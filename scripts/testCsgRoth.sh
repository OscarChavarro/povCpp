#!/usr/bin/env bash
# RMSE-tolerant comparator for the -csgRoth output (see doc/CSGByRaySegments.md,
# Q4): Roth's boolean ray-segment classification is not byte-identical to the
# default point-membership CSG at boolean edges, so this reuses the same
# golden images as testAgainstGoldenImages.sh but accepts a normalized RMSE
# below RMSE_THRESHOLD instead of requiring AE=0.
set -euo pipefail

output_dir="outputCsgByRaySegments"
reference_dir="../referenceTestImages"
RMSE_THRESHOLD="${RMSE_THRESHOLD:-0.02}"

if [[ ! -d "${output_dir}" ]]; then
    echo "Missing output directory: ${output_dir}" >&2
    exit 1
fi

if [[ ! -d "${reference_dir}" ]]; then
    echo "Missing reference directory: ${reference_dir}" >&2
    exit 1
fi

failures=0

while IFS= read -r generated; do
    rel_path="${generated#${output_dir}/}"
    reference="${reference_dir}/${rel_path}"

    if [[ ! -f "${reference}" ]]; then
        echo "Missing reference image: ${reference}" >&2
        failures=$((failures + 1))
        continue
    fi

    compare_output=""
    compare_output=$(compare -metric RMSE "${generated}" "${reference}" null: 2>&1) || true
    # compare prints "<absolute> (<normalized 0..1>)" on stderr.
    normalized_rmse=$(printf '%s' "${compare_output}" | sed -n 's/.*(\(.*\))/\1/p')

    if [[ -z "${normalized_rmse}" ]]; then
        echo "compare failed for ${rel_path}: ${compare_output}" >&2
        failures=$((failures + 1))
        continue
    fi

    if (( $(echo "${normalized_rmse} > ${RMSE_THRESHOLD}" | bc -l) )); then
        echo "RMSE above threshold: ${rel_path} (RMSE=${normalized_rmse}, threshold=${RMSE_THRESHOLD})" >&2
        failures=$((failures + 1))
    fi
done < <(find "${output_dir}" -type f -name '*.tga' | sort)

while IFS= read -r reference; do
    rel_path="${reference#${reference_dir}/}"
    generated="${output_dir}/${rel_path}"

    if [[ ! -f "${generated}" ]]; then
        echo "Missing generated image: ${generated}" >&2
        failures=$((failures + 1))
    fi
done < <(find "${reference_dir}" -type f -name '*.tga' | sort)

if ((failures == 0)); then
    echo "Test passed (RMSE threshold ${RMSE_THRESHOLD})."
else
    echo "Test failed with ${failures} issue(s)." >&2
    exit 1
fi
