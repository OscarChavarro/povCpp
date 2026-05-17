#!/usr/bin/env bash
set -euo pipefail

output_dir="output"
reference_dir="../referenceTestImages"

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

    if ! diff -q "${generated}" "${reference}" >/dev/null; then
        echo "Different image: ${rel_path}" >&2
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
