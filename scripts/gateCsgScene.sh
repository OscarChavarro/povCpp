#!/usr/bin/env bash
# Single-scene gate for the -csgRoth path. Renders ONE scene with -csgRoth and
# compares it (RMSE-tolerant) against its golden reference image.
#   usage: scripts/gateCsgScene.sh <level>/<scene> [RMSE_THRESHOLD]
#   e.g.   scripts/gateCsgScene.sh level1/cantelop 0.02
set -euo pipefail

rel="$1"                                   # e.g. level1/cantelop
threshold="${2:-0.02}"
scene="$(basename "$rel")"                 # cantelop
level="$(dirname "$rel")"                  # level1
povray="$(pwd)/build/povray"
reference="../referenceTestImages/${rel}.tga"
out_dir="$(mktemp -d)"
out="${out_dir}/${scene}.tga"

# Most scenes live in etc/<level>/<scene>.pov with include at ../include; a
# few live one directory deeper (their own subdirectory under <level>), with
# include at ../../include instead.
nested_scenes="bumpmap car drums fish13 ionic5 ntreal pencil piece2 piece3 snail teapot"
scene_dir="etc/${level}"
include="../include"
for nested in ${nested_scenes}; do
    if [[ "${scene}" == "${nested}" ]]; then
        nested_subdir="${nested}"
        if [[ "${nested}" == "drums" ]]; then
            nested_subdir="drums2"
        fi
        scene_dir="etc/${level}/${nested_subdir}"
        include="../../include"
        break
    fi
done

( cd "${scene_dir}" && "${povray}" \
    "+l${include}" "+i${scene}.pov" "+o${out}" \
    +w1280 +h800 -d -v +x +ft -csgRoth -parallel >/dev/null 2>&1 )

rmse="$(compare -metric RMSE "${out}" "${reference}" null: 2>&1 | sed -n 's/.*(\(.*\))/\1/p')" || true
echo "RMSE(${rel}) = ${rmse}  (threshold ${threshold})"
awk -v r="${rmse}" -v t="${threshold}" 'BEGIN{ exit !(r+0 <= t+0) }' \
  && echo "GATE PASS" \
  || { echo "GATE FAIL"; exit 1; }
