#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD_DIR="${IANNIX_BUILD_DIR:-$REPO_ROOT/build}"
OUT_DIR="${IANNIX_OUT_DIR:-$REPO_ROOT/dist/linux/appimage}"
APPDIR="$OUT_DIR/AppDir"

"$REPO_ROOT/deploy/shared/stage.sh" "$BUILD_DIR" "$APPDIR/usr"
install -Dm755 "$REPO_ROOT/deploy/linux/appimage/AppRun" "$APPDIR/AppRun"
install -Dm644 "$REPO_ROOT/deploy/linux/iannix.desktop" "$APPDIR/iannix.desktop"
install -Dm644 "$REPO_ROOT/deploy/linux/iannix.png" "$APPDIR/iannix.png"

if command -v appimagetool >/dev/null 2>&1; then
    appimagetool "$APPDIR" "$OUT_DIR/IanniX.AppImage"
else
    printf 'appimagetool not found; AppDir prepared at %s\n' "$APPDIR"
fi
