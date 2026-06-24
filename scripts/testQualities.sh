#!/usr/bin/env bash
#
# Golden-image gate for the per-quality renders produced by
# scripts/renderQualities.sh: byte-for-pixel comparison of everything under
# output/qualities/ (recursively) against the matching path under
# ../referenceTestImagesQualities/. That currently covers two tiers -
# output/qualities/iortest_q{0..9}.tga (all ten quality levels on one
# feature-spanning scene) and output/qualities/<level>/<scene>_q{0,2,4,6,8}.tga
# (one representative quality per band, on all 108 gate scenes, mirroring
# renderAll.sh's per-level output layout) - but this script does not know or
# care about that structure: it just diffs whatever .tga files exist on both
# sides by relative path, so widening renderQualities.sh's coverage needs no
# change here.
#
# This protects the meaning of the +qN quality knob (see
# doc/vitralNormalizationAnalysis.md §7): any change to the quality model must
# keep each level producing exactly the image it produces today, unless the
# baseline is deliberately regenerated. Run after ./scripts/renderQualities.sh.
set -euo pipefail

output_dir="output/qualities"
reference_dir="../referenceTestImagesQualities"

if [[ ! -d "${output_dir}" ]]; then
    echo "Missing output directory: ${output_dir} (run ./scripts/renderQualities.sh first)" >&2
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

    ae_error=""
    if ! ae_error=$(compare -metric AE "${generated}" "${reference}" null: 2>&1); then
        if [[ "${ae_error}" =~ ^[0-9]+$ ]]; then
            echo "Different image: ${rel_path} (AE=${ae_error})" >&2
        else
            echo "compare failed for ${rel_path}: ${ae_error}" >&2
        fi
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
    echo "Test passed."
else
    echo "Test failed with ${failures} issue(s)." >&2
    exit 1
fi
