#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")/..")"
cd "${ROOT_DIR}"

./scripts/compile.sh \
  -DENABLE_ANTLR_CPP_RUNTIME=OFF \
  -DENABLE_ANTLR_GENERATION=OFF \
  -DCMAKE_CXX_FLAGS=-pg \
  -DCMAKE_EXE_LINKER_FLAGS=-pg \
  "$@"

cp build/povray "${ROOT_DIR}/povrayLegacy"
echo "Generated ${ROOT_DIR}/povrayLegacy"
