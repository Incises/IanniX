#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
BUILD_DIR="${IANNIX_BUILD_DIR:-$REPO_ROOT/build}"
OUT_DIR="${IANNIX_OUT_DIR:-$REPO_ROOT/dist/linux/deb}"

"$REPO_ROOT/deploy/shared/stage.sh" "$BUILD_DIR" "$OUT_DIR/stage"
cpack --config "$BUILD_DIR/CPackConfig.cmake" -G DEB -B "$OUT_DIR"
