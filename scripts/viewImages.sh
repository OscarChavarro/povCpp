#!/usr/bin/env bash
set -euo pipefail

OUTPUT_DIR="${OUTPUT_DIR:-output}"
REFERENCE_DIR="${REFERENCE_DIR:-../referenceTestImages}"
TARGET_DIR="${TARGET_DIR:-/tmp/i}"

if [[ ! -d "${OUTPUT_DIR}" ]]; then
  echo "Missing output directory: ${OUTPUT_DIR}" >&2
  exit 1
fi

if [[ ! -d "${REFERENCE_DIR}" ]]; then
  echo "Missing reference directory: ${REFERENCE_DIR}" >&2
  exit 1
fi

# Prefer ImageMagick v7 (magick), fallback to convert.
if command -v magick >/dev/null 2>&1; then
  CONVERTER_BIN="magick"
elif command -v convert >/dev/null 2>&1; then
  CONVERTER_BIN="convert"
else
  echo "Neither 'magick' nor 'convert' is available in PATH" >&2
  exit 1
fi

rm -rf "${TARGET_DIR}"
mkdir -p "${TARGET_DIR}"

work_dir="$(mktemp -d /tmp/viewImages.XXXXXX)"
trap 'rm -rf "${work_dir}"' EXIT

process_image() {
  local reference_file="$1"
  local rel_path output_file comparison_file reference_size output_size safe_name mask_file diff_file

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
  mask_file="${work_dir}/${safe_name}.mask.png"
  diff_file="${work_dir}/${safe_name}.diff.png"

  "${CONVERTER_BIN}" "${reference_file}" "${output_file}" -compose difference -composite -alpha off -threshold 0 "${mask_file}"
  "${CONVERTER_BIN}" "${mask_file}" -alpha off -fill red -opaque white -fill white -opaque black "${diff_file}"
  "${CONVERTER_BIN}" "${reference_file}" "${output_file}" "${diff_file}" +append "${comparison_file}"
}

export OUTPUT_DIR REFERENCE_DIR TARGET_DIR work_dir CONVERTER_BIN
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
  echo ""
  echo "${file_path}"
  img2sixel "${file_path}"
done < <(find "${TARGET_DIR}" -type f | sort)

if [[ "${job_failed}" -ne 0 ]]; then
  exit 1
fi
