#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
bin="${repo_root}/build/povray"
output_root="${repo_root}/output/panel"
reference_root="${repo_root}/../referenceTestImages"
width="${POV_PANEL_WIDTH:-320}"
height="${POV_PANEL_HEIGHT:-200}"
golden_width="${POV_GOLDEN_WIDTH:-1280}"
golden_height="${POV_GOLDEN_HEIGHT:-800}"
with_goldens=0

while (($# > 0)); do
    case "$1" in
        --with-goldens)
            with_goldens=1
            shift
            ;;
        *)
            echo "Unknown option: $1" >&2
            echo "Usage: scripts/benchmarkPanel.sh [--with-goldens]" >&2
            exit 1
            ;;
    esac
done

if [[ ! -x "${bin}" ]]; then
    echo "Missing renderer binary: ${bin}" >&2
    echo "Build first with ./scripts/compile.sh" >&2
    exit 1
fi

mkdir -p "${output_root}"

# Baseline column corrected 2026-07-04 (Plan 10 Phase 3): the original
# values here (1.989/1.292/4.112/5.316/0.382) were roughly 2x too slow -
# traced to a stale -pg (gprof) instrumentation flag baked into a baseline
# worktree's cmake cache at the time those numbers were first measured.
# A clean rebuild of 4af1a75 (no -pg) on the same machine gives the figures
# below; see doc/performanceReviewPlan10.md Phase 3 for the full comparison
# and the corrected panel verdict (4 of 5 scenes are actually SLOWER than
# baseline, not faster - only spline beats it).
scenes=(
    "level2|spline.pov|../include|1.045"
    "level3/ntreal|ntreal.pov|../../include|0.64"
    "level3/piece3|piece3.pov|../../include|2.015"
    "level2|iortest.pov|../include|2.115"
    "level1|shapes2.pov|../include|0.145"
)

golden_scenes=(
    "level3|kscope.pov|../include"
    "level3/pencil|pencil.pov|../../include"
)

run_scene() {
    local scene_dir="$1"
    local scene_file="$2"
    local include_dir="$3"
    local output_file="$4"
    local stderr_file="$5"
    local scene_width="$6"
    local scene_height="$7"

    (
        cd "${repo_root}/etc/${scene_dir}"
        /usr/bin/time -f '%e' \
            "${bin}" \
            "+l${include_dir}" \
            "+i${scene_file}" \
            "+o${output_file}" \
            "+w${scene_width}" "+h${scene_height}" -d +ft -x \
            >/dev/null 2>"${stderr_file}"
    )
}

extract_seconds() {
    local stderr_file="$1"
    tail -n 1 "${stderr_file}"
}

printf "| Scene | Baseline 4af1a75 (s) | Current worktree (s) | Factor |\n"
printf "| --- | ---: | ---: | ---: |\n"

for spec in "${scenes[@]}"; do
    IFS='|' read -r scene_dir scene_file include_dir baseline <<<"${spec}"
    scene_slug="${scene_dir}/${scene_file%.pov}"
    scene_slug="${scene_slug//\//_}"
    output_file="${output_root}/${scene_slug}.tga"
    stderr_file="${output_root}/${scene_slug}.stderr.log"

    run_scene "${scene_dir}" "${scene_file}" "${include_dir}" "${output_file}" "${stderr_file}" "${width}" "${height}"

    seconds="$(extract_seconds "${stderr_file}")"
    factor="$(awk -v current="${seconds}" -v base="${baseline}" 'BEGIN { printf "%.3f", current / base }')"
    printf '| `%s/%s` | %s | %s | %s |\n' "${scene_dir}" "${scene_file%.pov}" "${baseline}" "${seconds}" "${factor}"
done

if ((with_goldens == 0)); then
    exit 0
fi

if ! command -v compare >/dev/null 2>&1; then
    echo "ImageMagick compare is required for --with-goldens" >&2
    exit 1
fi

if [[ ! -d "${reference_root}" ]]; then
    echo "Missing reference image directory: ${reference_root}" >&2
    exit 1
fi

printf "\n| Golden Scene | AE | RMSE |\n"
printf "| --- | ---: | ---: |\n"

for spec in "${golden_scenes[@]}"; do
    IFS='|' read -r scene_dir scene_file include_dir <<<"${spec}"
    scene_slug="${scene_dir}/${scene_file%.pov}"
    scene_slug="${scene_slug//\//_}"
    output_file="${output_root}/${scene_slug}.tga"
    stderr_file="${output_root}/${scene_slug}.stderr.log"
    relative_ref="${scene_dir%%/*}/${scene_file%.pov}.tga"
    reference_file="${reference_root}/${relative_ref}"

    run_scene "${scene_dir}" "${scene_file}" "${include_dir}" "${output_file}" "${stderr_file}" "${golden_width}" "${golden_height}"

    ae="$(compare -metric AE "${output_file}" "${reference_file}" null: 2>&1 || true)"
    rmse="$(compare -metric RMSE "${output_file}" "${reference_file}" null: 2>&1 || true)"
    printf '| `%s/%s` | %s | %s |\n' "${scene_dir}" "${scene_file%.pov}" "${ae}" "${rmse}"
done
