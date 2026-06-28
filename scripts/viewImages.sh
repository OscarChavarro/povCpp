#!/usr/bin/env bash
set -euo pipefail

OUTPUT_DIR="${OUTPUT_DIR:-output}"
REFERENCE_DIR="${REFERENCE_DIR:-../referenceTestImages}"
TARGET_DIR="${TARGET_DIR:-/tmp/i}"

# Difference visualization controls.
# DIFF_GAMMA > 1 brightens the magnitude before the heat-map lookup so that
# small (but non-zero) colour distances become visible while exact matches
# stay black. DIFF_GRAYSCALE picks how the per-pixel colour distance is
# collapsed to a single magnitude (RMS ~ Euclidean colour distance).
DIFF_GAMMA="${DIFF_GAMMA:-2.2}"
DIFF_GRAYSCALE="${DIFF_GRAYSCALE:-RMS}"

# With -skip, scenes whose AE metric is below the threshold are omitted from
# the final listing: neither their name line nor their img2sixel preview is
# shown, so only the scenes at or above the threshold are displayed.
# `-skip` alone is shorthand for `-skip 1`, i.e. skip only pixel-perfect
# matches (AE == 0).
SKIP_THRESHOLD=0
if [[ "${1:-}" == "-skip" ]]; then
  SKIP_THRESHOLD=1
  if [[ -n "${2:-}" ]]; then
    if [[ ! "${2}" =~ ^[0-9]+$ ]]; then
      echo "Invalid -skip threshold: ${2} (expected non-negative integer)" >&2
      exit 1
    fi
    SKIP_THRESHOLD="${2}"
  fi
fi

if [[ ! -d "${OUTPUT_DIR}" ]]; then
  echo "Missing output directory: ${OUTPUT_DIR}" >&2
  exit 1
fi

if [[ ! -d "${REFERENCE_DIR}" ]]; then
  echo "Missing reference directory: ${REFERENCE_DIR}" >&2
  exit 1
fi

# Prefer ImageMagick v7 (magick), fallback to convert. COMPARE_BIN is the
# matching difference-metric tool: 'magick compare' on v7, the standalone
# 'compare' on v6.
if command -v magick >/dev/null 2>&1; then
  CONVERTER_BIN="magick"
  COMPARE_BIN="magick compare"
elif command -v convert >/dev/null 2>&1; then
  CONVERTER_BIN="convert"
  COMPARE_BIN="compare"
else
  echo "Neither 'magick' nor 'convert' is available in PATH" >&2
  exit 1
fi

rm -rf "${TARGET_DIR}"
mkdir -p "${TARGET_DIR}"

work_dir="$(mktemp -d /tmp/viewImages.XXXXXX)"
trap 'rm -rf "${work_dir}"' EXIT

# Build the heat-map colour lookup table once: a 256x1 strip mapping
# magnitude 0 (left, exact match) to maximum (right). Stops:
#   black -> blue -> cyan -> yellow -> red
# Four 64-wide segments are appended vertically then rotated so that index 0
# (black, "no difference") lands on the left, as -clut expects.
clut_file="${work_dir}/heat.clut.png"
"${CONVERTER_BIN}" \
  \( -size 1x64 gradient:black-blue \) \
  \( -size 1x64 gradient:blue-cyan \) \
  \( -size 1x64 gradient:cyan-yellow \) \
  \( -size 1x64 gradient:yellow-red \) \
  -append -rotate -90 "${clut_file}"

process_image() {
  local reference_file="$1"
  local rel_path output_file comparison_file reference_size output_size safe_name diff_file metric

  rel_path="${reference_file#${REFERENCE_DIR}/}"
  output_file="${OUTPUT_DIR}/${rel_path}"
  if [[ ! -f "${output_file}" ]]; then
    echo "Missing output image: ${output_file}" >&2
    exit 1
  fi

  comparison_file="${TARGET_DIR}/${rel_path%.tga}.png"
  mkdir -p "$(dirname "${comparison_file}")"

  reference_size="$("${CONVERTER_BIN}" "${reference_file}" -format '%wx%h' info:)"
  output_size="$("${CONVERTER_BIN}" "${output_file}" -format '%wx%h' info:)"
  if [[ "${reference_size}" != "${output_size}" ]]; then
    echo "Image size mismatch for ${rel_path}: ${reference_size} vs ${output_size}" >&2
    exit 1
  fi

  safe_name="$(printf '%s' "${rel_path}" | tr '/.' '__')"
  diff_file="${work_dir}/${safe_name}.diff.png"

  # Per-pixel colour distance -> single magnitude -> gamma lift -> heat-map.
  # Exact matches stay black; the colour ramps blue/cyan/yellow/red with the
  # size of the colour distance, so small texture shifts and missing objects
  # are visually distinguishable instead of both being flat red.
  "${CONVERTER_BIN}" "${reference_file}" "${output_file}" \
    -alpha off -compose difference -composite \
    -grayscale "${DIFF_GRAYSCALE}" \
    -gamma "${DIFF_GAMMA}" \
    "${clut_file}" -clut \
    "${diff_file}"
  "${CONVERTER_BIN}" "${reference_file}" "${output_file}" "${diff_file}" +append "${comparison_file}"

  # Absolute-error count: number of pixels that differ at all. 0 means an exact
  # pixel match. Stored in a sidecar so the display loop can label the scene and
  # honour -skip/-skip N without recomputing. COMPARE_BIN is left unquoted on
  # purpose so the two-word 'magick compare' form word-splits correctly.
  metric="$(${COMPARE_BIN} -metric AE "${reference_file}" "${output_file}" null: 2>&1 || true)"
  metric="${metric%% *}"
  printf '%s' "${metric}" > "${work_dir}/${safe_name}.metric"
}

export OUTPUT_DIR REFERENCE_DIR TARGET_DIR work_dir CONVERTER_BIN COMPARE_BIN
export clut_file DIFF_GAMMA DIFF_GRAYSCALE
export -f process_image

reference_count="$(find "${REFERENCE_DIR}" -type f -name '*.tga' | wc -l)"
if [[ "${reference_count}" -eq 0 ]]; then
  echo "No .tga files found under ${REFERENCE_DIR}" >&2
  exit 1
fi

max_jobs="$(nproc)"
active_jobs=0
job_failed=0
while IFS= read -r -d '' reference_file; do
  bash -c 'set -euo pipefail; process_image "$1"' _ "${reference_file}" &
  active_jobs=$((active_jobs + 1))

  if [[ "${active_jobs}" -ge "${max_jobs}" ]]; then
    if ! wait -n; then
      job_failed=1
    fi
    active_jobs=$((active_jobs - 1))
  fi
done < <(find "${REFERENCE_DIR}" -type f -name '*.tga' -print0 | sort -z)

while [[ "${active_jobs}" -gt 0 ]]; do
  if ! wait; then
    job_failed=1
  fi
  active_jobs=0
done

if [[ "${reference_count}" -ne 108 ]]; then
  echo "Expected 108 reference images, found ${reference_count}" >&2
  exit 1
fi

while IFS= read -r file_path; do
  rel_png="${file_path#${TARGET_DIR}/}"
  safe_name="$(printf '%s' "${rel_png%.png}.tga" | tr '/.' '__')"
  metric="$(cat "${work_dir}/${safe_name}.metric" 2>/dev/null || true)"

  if [[ -n "${metric}" && "${metric}" =~ ^[0-9]+$ && "${metric}" -lt "${SKIP_THRESHOLD}" ]]; then
    continue
  fi

  if [[ "${metric}" == "0" ]]; then
    label="(pixel match)"
  else
    label="(AE=${metric:-?})"
  fi

  echo ""
  echo "${file_path} ${label}"
  img2sixel "${file_path}"
done < <(find "${TARGET_DIR}" -type f -name '*.png' | sort)

if [[ "${job_failed}" -ne 0 ]]; then
  exit 1
fi

echo ""
