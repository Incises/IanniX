#!/usr/bin/env bash
# Build, bundle, sign, optionally notarize, and package IanniX as a macOS DMG.
#
# Supports two modes:
#   1. Ad-hoc signing (default when IANNIX_SIGN_IDENTITY is unset or "-"):
#      Builds IanniX.app via CMake, runs macdeployqt, applies ad-hoc signature
#      (required on Apple Silicon), and creates the DMG in dist/macos/dmg/.
#   2. Developer ID + Notarization (when IANNIX_SIGN_IDENTITY is set):
#      Signs all nested dylibs, frameworks, plugins, and IanniX.app with
#      Hardened Runtime (--options runtime) and deploy/macos/entitlements.plist,
#      notarizes & staples IanniX.app (if IANNIX_NOTARY_PROFILE is set), creates
#      the DMG, and signs + notarizes + staples the DMG.
#
# Environment variables (can also be placed in deploy/macos/signing.local.env):
#   IANNIX_SIGN_IDENTITY   e.g. "Developer ID Application: Your Name (TEAMID)" (default: "-")
#   IANNIX_NOTARY_PROFILE  xcrun notarytool keychain profile name (optional)
#   IANNIX_QT_DIR          Path to Qt5 prefix (default: "$(brew --prefix qt@5)")
#   IANNIX_BUILD_DIR       CMake build directory (default: "<repo>/build")
#   IANNIX_BUILD_CONFIG    CMake build configuration (default: "Release")
#   IANNIX_OUT_DIR         Output directory for DMG (default: "<repo>/dist/macos/dmg")

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
LOCAL_ENV="$REPO_ROOT/deploy/macos/signing.local.env"
if [[ -f "$LOCAL_ENV" ]]; then
    # shellcheck disable=SC1090
    source "$LOCAL_ENV"
fi

BREW_PREFIX="$(brew --prefix 2>/dev/null || echo /opt/homebrew)"
QT_DIR="${IANNIX_QT_DIR:-$(brew --prefix qt@5 2>/dev/null || echo "$BREW_PREFIX/opt/qt@5")}"
BUILD_DIR="${IANNIX_BUILD_DIR:-$REPO_ROOT/build}"
CONFIG="${IANNIX_BUILD_CONFIG:-Release}"
OUT_DIR="${IANNIX_OUT_DIR:-$REPO_ROOT/dist/macos/dmg}"
SIGN_IDENTITY="${IANNIX_SIGN_IDENTITY:--}"
NOTARY_PROFILE="${IANNIX_NOTARY_PROFILE:-}"
ARCH="${CMAKE_OSX_ARCHITECTURES:-$(uname -m)}"
VERSION="$(plutil -extract CFBundleShortVersionString raw "$REPO_ROOT/deploy/macos/Info.plist")"
ENTITLEMENTS="$REPO_ROOT/deploy/macos/entitlements.plist"
APP_BUNDLE="$BUILD_DIR/IanniX.app"

# Remove any previously deployed bundle so macdeployqt starts cleanly
rm -rf "$APP_BUNDLE"

echo "== Configuring & building IanniX $VERSION ($ARCH) =="
cmake -S "$REPO_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$CONFIG" \
    -DCMAKE_PREFIX_PATH="$QT_DIR;$BREW_PREFIX"
cmake --build "$BUILD_DIR" --config "$CONFIG" --parallel

echo "== Bundling Qt frameworks with macdeployqt =="
"$QT_DIR/bin/macdeployqt" "$APP_BUNDLE"

if [[ "$SIGN_IDENTITY" == "-" ]]; then
    echo "== Applying ad-hoc signature =="
    codesign --force --deep --entitlements "$ENTITLEMENTS" -s - "$APP_BUNDLE"
else
    echo "== Signing with Developer ID ($SIGN_IDENTITY) =="
    if [[ -d "$APP_BUNDLE/Contents/Frameworks" ]]; then
        find "$APP_BUNDLE/Contents/Frameworks" -name "*.dylib" -maxdepth 1 \
            -exec codesign --force --options runtime --timestamp -s "$SIGN_IDENTITY" {} \;
        for fw in "$APP_BUNDLE/Contents/Frameworks"/*.framework; do
            [[ -e "$fw" ]] || continue
            codesign --force --options runtime --timestamp -s "$SIGN_IDENTITY" "$fw"
        done
    fi
    if [[ -d "$APP_BUNDLE/Contents/PlugIns" ]]; then
        find "$APP_BUNDLE/Contents/PlugIns" -name "*.dylib" \
            -exec codesign --force --options runtime --timestamp -s "$SIGN_IDENTITY" {} \;
    fi
    codesign --force --options runtime --timestamp \
        --entitlements "$ENTITLEMENTS" -s "$SIGN_IDENTITY" "$APP_BUNDLE"
fi

codesign --verify --deep --strict "$APP_BUNDLE"

if [[ "$SIGN_IDENTITY" != "-" && -n "$NOTARY_PROFILE" ]]; then
    echo "== Notarizing IanniX.app =="
    NOTARIZE_ZIP="$BUILD_DIR/IanniX-notarize.zip"
    ditto -c -k --keepParent "$APP_BUNDLE" "$NOTARIZE_ZIP"
    xcrun notarytool submit "$NOTARIZE_ZIP" --keychain-profile "$NOTARY_PROFILE" --wait
    xcrun stapler staple "$APP_BUNDLE"
    rm -f "$NOTARIZE_ZIP"
fi

echo "== Creating DMG =="
mkdir -p "$OUT_DIR"
DMG_PATH="$OUT_DIR/IanniX-$VERSION-macOS-$ARCH.dmg"
STAGE_DIR="$(mktemp -d)"
trap 'rm -rf "$STAGE_DIR"' EXIT

cp -R "$APP_BUNDLE" "$STAGE_DIR/"
ln -s /Applications "$STAGE_DIR/Applications"
hdiutil create -volname "IanniX $VERSION" -srcfolder "$STAGE_DIR" -ov -format UDZO "$DMG_PATH"

if [[ "$SIGN_IDENTITY" != "-" ]]; then
    echo "== Signing DMG =="
    codesign --force --timestamp -s "$SIGN_IDENTITY" "$DMG_PATH"
    if [[ -n "$NOTARY_PROFILE" ]]; then
        echo "== Notarizing DMG =="
        xcrun notarytool submit "$DMG_PATH" --keychain-profile "$NOTARY_PROFILE" --wait
        xcrun stapler staple "$DMG_PATH"
        spctl -a -t open --context context:primary-signature -v "$DMG_PATH"
    fi
fi

echo "== Done: $DMG_PATH =="
