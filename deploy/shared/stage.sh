#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${1:-$REPO_ROOT/build}"
STAGE_DIR="${2:-$REPO_ROOT/dist/stage}"
CONFIG="${IANNIX_BUILD_CONFIG:-Release}"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --config "$CONFIG"
rm -rf "$STAGE_DIR"
cmake --install "$BUILD_DIR" --prefix "$STAGE_DIR" --config "$CONFIG"

printf 'Staged install tree at %s\n' "$STAGE_DIR"
