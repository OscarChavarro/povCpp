#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if command -v nproc >/dev/null 2>&1; then
    jobs="$(nproc)"
else
    jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
fi

cmake --build build -j"${jobs}"

if [[ -f build/compile_commands.json ]]; then
    cp build/compile_commands.json compile_commands.json
fi
