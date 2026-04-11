#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT_DIR="${IANNIX_OUT_DIR:-$REPO_ROOT/dist/linux/deb}"

"$REPO_ROOT/deploy/shared/cpack.sh" DEB "$OUT_DIR"
