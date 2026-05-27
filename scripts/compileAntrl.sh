#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")/..")"
cd "${ROOT_DIR}"

./scripts/compile.sh \
  -DENABLE_ANTLR_CPP_RUNTIME=ON \
  -DENABLE_ANTLR_GENERATION=ON \
  -DENABLE_ANTLR_DOWNLOAD=ON \
  -DANTLR4_RUNTIME_INCLUDE_DIR=/usr/include/antlr4-runtime \
  -DANTLR4_RUNTIME_LIBRARY=/usr/lib/x86_64-linux-gnu/libantlr4-runtime.so \
  -DCMAKE_CXX_FLAGS=-pg \
  -DCMAKE_EXE_LINKER_FLAGS=-pg \
  "$@"

cp build/povray "${ROOT_DIR}/povrayAntlr"
echo "Generated ${ROOT_DIR}/povrayAntlr"
