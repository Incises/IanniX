#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD_DIR="${IANNIX_BUILD_DIR:-$REPO_ROOT/build}"
OUT_DIR="${IANNIX_OUT_DIR:-$REPO_ROOT/dist/macos/pkg}"
STAGE_DIR="$OUT_DIR/stage"
APP_BUNDLE="$STAGE_DIR/IanniX.app"

"$REPO_ROOT/deploy/shared/stage.sh" "$BUILD_DIR" "$STAGE_DIR"

if [[ ! -d "$APP_BUNDLE" ]]; then
    printf 'Expected app bundle at %s\n' "$APP_BUNDLE" >&2
    exit 1
fi

pkgbuild --component "$APP_BUNDLE" --install-location /Applications "$OUT_DIR/IanniX.pkg"
