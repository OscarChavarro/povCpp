#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")/..")"
BIN="${ROOT_DIR}/povrayLegacy"

if [[ ! -x "${BIN}" ]]; then
  echo "Missing binary: ${BIN}. Run ./scripts/compileLegacy.sh first." >&2
  exit 1
fi

STAMP="$(date +%Y%m%d_%H%M%S)"
export GMON_OUT_PREFIX="/tmp/gmon_legacy_${STAMP}"

echo "Using GMON_OUT_PREFIX=${GMON_OUT_PREFIX}" >&2
"${BIN}" "$@"

PROFILE_FILE="$(ls -1 ${GMON_OUT_PREFIX}.* 2>/dev/null | head -n 1 || true)"
if [[ -n "${PROFILE_FILE}" ]]; then
  echo "Profile: ${PROFILE_FILE}" >&2
else
  echo "Warning: no gprof profile file found for prefix ${GMON_OUT_PREFIX}" >&2
fi
