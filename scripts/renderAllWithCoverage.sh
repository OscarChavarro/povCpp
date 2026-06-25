#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd -P)"
BUILD_DIR="${ROOT_DIR}/build"
COVERAGE_DIR="${BUILD_DIR}/coverage"

cd "${ROOT_DIR}"

echo "[1/4] Cleaning build directory"
rm -rf "${BUILD_DIR}"

echo "[2/4] Compiling with gcov instrumentation"
"${ROOT_DIR}/scripts/compile.sh" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS="--coverage -O0 -g" \
  -DCMAKE_CXX_FLAGS="--coverage -O0 -g" \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage" \
  -DCMAKE_SHARED_LINKER_FLAGS="--coverage"

echo "[3/4] Running all renders (accumulative coverage)"
"${ROOT_DIR}/scripts/renderAll.sh"
"${ROOT_DIR}/scripts/renderQualities.sh"

mkdir -p "${COVERAGE_DIR}"

echo "[4/4] Generating HTML coverage report"
if command -v gcovr >/dev/null 2>&1; then
  gcovr \
    --root "${ROOT_DIR}" \
    --object-directory "${BUILD_DIR}" \
    --filter "${ROOT_DIR}/src/.*" \
    --exclude-directories ".*/CMakeFiles/[0-9.]+/CompilerId.*" \
    --gcov-exclude ".*/CompilerId[^/]+/.*" \
    --exclude ".*CMakeCXXCompilerId\\.cpp$" \
    --exclude ".*CMakeCCompilerId\\.c$" \
    --gcov-executable gcov \
    --exclude-unreachable-branches \
    --exclude-throw-branches \
    --html-details "${COVERAGE_DIR}/index.html" \
    --print-summary
elif command -v lcov >/dev/null 2>&1 && command -v genhtml >/dev/null 2>&1; then
  lcov \
    --capture \
    --directory "${BUILD_DIR}" \
    --base-directory "${ROOT_DIR}" \
    --output-file "${COVERAGE_DIR}/coverage.info"
  lcov \
    --remove "${COVERAGE_DIR}/coverage.info" \
    '*/CMakeFiles/*' \
    '*/CMakeCXXCompilerId.cpp' \
    '*/CMakeCCompilerId.c' \
    --output-file "${COVERAGE_DIR}/coverage.info"
  genhtml "${COVERAGE_DIR}/coverage.info" --output-directory "${COVERAGE_DIR}"
else
  echo "Missing coverage tools. Install gcovr (preferred) or lcov+genhtml." >&2
  exit 1
fi

echo "Coverage report ready at: ${COVERAGE_DIR}/index.html"
