#!/usr/bin/env bash
set -euo pipefail

# This tree predates CMake: build with its Makefile (objects in tmp/, binary in
# bin/) and expose the binary at build/povray, where renderAll.sh expects it.
mkdir -p tmp bin build

if command -v nproc >/dev/null 2>&1; then
    jobs="$(nproc)"
else
    jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
fi

make -j"${jobs}" all
cp bin/povray build/povray
