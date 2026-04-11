#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    printf 'Usage: %s <generator> [output-dir]\n' "${BASH_SOURCE[0]}" >&2
    exit 1
fi

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
GENERATOR="$1"
OUT_DIR="${2:-}"
BUILD_DIR="${IANNIX_BUILD_DIR:-$REPO_ROOT/build}"
CONFIG="${IANNIX_BUILD_CONFIG:-Release}"

if [[ -z "$OUT_DIR" ]]; then
    OUT_DIR="$REPO_ROOT/dist/${GENERATOR,,}"
fi

cmake -S "$REPO_ROOT" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" --config "$CONFIG"
cpack --config "$BUILD_DIR/CPackConfig.cmake" -G "$GENERATOR" -B "$OUT_DIR" -C "$CONFIG"

printf 'Generated %s package artifacts in %s\n' "$GENERATOR" "$OUT_DIR"
