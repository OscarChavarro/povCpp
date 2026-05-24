#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
POVRAY_BIN="${ROOT_DIR}/build/povray"
SCENE_ROOT="${ROOT_DIR}/etc"
INCLUDE_PATH="${SCENE_ROOT}/include"

if [[ ! -x "${POVRAY_BIN}" ]]; then
  echo "Missing binary: ${POVRAY_BIN}" >&2
  echo "Build first: cmake --build build -j2" >&2
  exit 1
fi

if [[ ! -d "${SCENE_ROOT}" ]]; then
  echo "Missing scene directory: ${SCENE_ROOT}" >&2
  exit 1
fi

total=0
ok=0
failed=0

tmp_log="$(mktemp)"
fail_log="$(mktemp)"
trap 'rm -f "${tmp_log}"' EXIT
trap 'rm -f "${tmp_log}" "${fail_log}"' EXIT

echo "Scanning scenes under ${SCENE_ROOT}"

while IFS= read -r scene; do
  total=$((total + 1))
  rel="${scene#${ROOT_DIR}/}"

  set +e
  POVCPP_PARSE_ONLY=1 \
    "${POVRAY_BIN}" "+i${scene}" "+l${INCLUDE_PATH}" "+w64" "+h64" \
    >"${tmp_log}" 2>&1
  rc=$?
  set -e

  if [[ ${rc} -eq 0 ]]; then
    ok=$((ok + 1))
    echo "AST_OK   ${rel}"
  else
    failed=$((failed + 1))
    printf "%s\n" "${rel}" >> "${fail_log}"
    echo "FAIL     ${rel}"
    tail -n 3 "${tmp_log}" | sed 's/^/  /'
  fi
done < <(find "${SCENE_ROOT}" -type f -name '*.pov' | sort)

echo
echo "Summary"
echo "  total:    ${total}"
echo "  ok:       ${ok}"
echo "  failed:   ${failed}"

if [[ -s "${fail_log}" ]]; then
  echo
  echo "Failed Files"
  sort "${fail_log}" | sed 's/^/  /'
fi

if [[ ${failed} -gt 0 ]]; then
  exit 2
fi
