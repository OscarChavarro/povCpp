#!/usr/bin/env bash
#
# Builds the performance baseline for doc/lazyQualitySelectionPlan.md: renders
# all 108 gate scenes at each quality level 0..9 and records the wall-clock
# render time of every (scene, quality) pair. The result is a 108 x 10 matrix
# of seconds, written as a TSV to output/qualityMatrix.tsv.
#
# This is the Step 0 baseline: run it BEFORE changing the quality model so the
# reinterpreted (VITRAL-style flag/lazy-mask) implementation can be compared
# cell-by-cell against it.
#
# Resolution defaults to a moderate 320x200 so the full 1080-render sweep is
# cheap and repeatable; the *relative* quality-vs-time curve is essentially
# resolution-independent (per-pixel work x pixel count). Override with:
#   WIDTH=1280 HEIGHT=800 ./scripts/renderQualityMatrix.sh
set -euo pipefail

POVRAY_BIN="$(pwd)/build/povray"
OUT_DIR="$(pwd)/output/qualityMatrix"
TSV="$(pwd)/output/qualityMatrix.tsv"
WIDTH="${WIDTH:-320}"
HEIGHT="${HEIGHT:-200}"

if [[ ! -x "${POVRAY_BIN}" ]]; then
    echo "Missing build/povray binary; run ./scripts/compile.sh first." >&2
    exit 1
fi

mkdir -p "${OUT_DIR}"

# Each entry: "<scene_dir under etc>|<scene_file>|<include path from scene_dir>"
# Mirrors the exact scene set and include layout of scripts/renderAll.sh.
scenes=(
    "level1/bumpmap|bumpmap.pov|../../include"
    "level1|alphafun.pov|../include"
    "level1|ballbox1.pov|../include"
    "level1|basicvue.pov|../include"
    "level1|blob.pov|../include"
    "level1|box.pov|../include"
    "level1|cantelop.pov|../include"
    "level1|checker2.pov|../include"
    "level1|cliptst2.pov|../include"
    "level1|colors.pov|../include"
    "level1|dish.pov|../include"
    "level1|dodec2.pov|../include"
    "level1|fogtst.pov|../include"
    "level1|glasdish.pov|../include"
    "level1|glass.pov|../include"
    "level1|imagetst.pov|../include"
    "level1|intee1.pov|../include"
    "level1|laser.pov|../include"
    "level1|mapper.pov|../include"
    "level1|mappr2.pov|../include"
    "level1|matmap.pov|../include"
    "level1|pvinterp.pov|../include"
    "level1|shapes2.pov|../include"
    "level1|shapes.pov|../include"
    "level1|spotlite.pov|../include"
    "level1|stone1.pov|../include"
    "level1|stone2.pov|../include"
    "level1|stone3.pov|../include"
    "level1|stone4.pov|../include"
    "level1|sunset1.pov|../include"
    "level1|sunset.pov|../include"
    "level1|texture1.pov|../include"
    "level1|texture2.pov|../include"
    "level1|texture3.pov|../include"
    "level1|window.pov|../include"
    "level2|arches.pov|../include"
    "level2|cluster.pov|../include"
    "level2|crystal.pov|../include"
    "level2|eight.pov|../include"
    "level2|esp01.pov|../include"
    "level2|hfclip.pov|../include"
    "level2|illum1.pov|../include"
    "level2|illum2.pov|../include"
    "level2|iortest.pov|../include"
    "level2|lpops1.pov|../include"
    "level2|lpops2.pov|../include"
    "level2|magglass.pov|../include"
    "level2|mtmand.pov|../include"
    "level2|pacman.pov|../include"
    "level2|pawns.pov|../include"
    "level2|planet.pov|../include"
    "level2|poolball.pov|../include"
    "level2|romo.pov|../include"
    "level2|room.pov|../include"
    "level2|skyvase.pov|../include"
    "level2|smoke.pov|../include"
    "level2|spline.pov|../include"
    "level2|stonewal.pov|../include"
    "level2|sunsethf.pov|../include"
    "level2|tetra.pov|../include"
    "level2|waterbow.pov|../include"
    "level2|wtorus.pov|../include"
    "level3/car|car.pov|../../include"
    "level3|chess.pov|../include"
    "level3|desk.pov|../include"
    "level3|dfwood.pov|../include"
    "level3/drums2|drums.pov|../../include"
    "level3/fish13|fish13.pov|../../include"
    "level3|fishbowl.pov|../include"
    "level3/ionic5|ionic5.pov|../../include"
    "level3|kscope.pov|../include"
    "level3|lamp.pov|../include"
    "level3/ntreal|ntreal.pov|../../include"
    "level3|oak2.pov|../include"
    "level3|palace.pov|../include"
    "level3/pencil|pencil.pov|../../include"
    "level3|piece1.pov|../include"
    "level3/piece2|piece2.pov|../../include"
    "level3/piece3|piece3.pov|../../include"
    "level3|pool.pov|../include"
    "level3|roman.pov|../include"
    "level3|snack.pov|../include"
    "level3/snail|snail.pov|../../include"
    "level3|takeoff.pov|../include"
    "level3/teapot|teapot.pov|../../include"
    "level3|tomb.pov|../include"
    "level3|wealth.pov|../include"
    "level3|wg5.pov|../include"
    "math|bezier0.pov|../include"
    "math|bezier.pov|../include"
    "math|bicube.pov|../include"
    "math|folium.pov|../include"
    "math|grafbic.pov|../include"
    "math|helix.pov|../include"
    "math|hyptorus.pov|../include"
    "math|lemnisc2.pov|../include"
    "math|lemnisca.pov|../include"
    "math|monkey.pov|../include"
    "math|partorus.pov|../include"
    "math|piriform.pov|../include"
    "math|quarcyl.pov|../include"
    "math|quarpara.pov|../include"
    "math|steiner.pov|../include"
    "math|tcubic.pov|../include"
    "math|teardrop.pov|../include"
    "math|torus.pov|../include"
    "math|trough.pov|../include"
    "math|witch.pov|../include"
)

# Header.
{
    printf "scene"
    for q in 0 1 2 3 4 5 6 7 8 9; do printf "\tq%d" "${q}"; done
    printf "\n"
} >"${TSV}"

start_time=$(date +%s)
scene_index=0

for entry in "${scenes[@]}"; do
    IFS='|' read -r scene_dir scene_file include_path <<<"${entry}"
    base_name="${scene_file%.pov}"
    scene_index=$((scene_index + 1))

    printf "%s" "${base_name}" >>"${TSV}"
    for q in 0 1 2 3 4 5 6 7 8 9; do
        t0=$(date +%s.%N)
        ( cd "etc/${scene_dir}" && "${POVRAY_BIN}" \
            "+l${include_path}" \
            "+i${scene_file}" \
            "+o${OUT_DIR}/${base_name}_q${q}.tga" \
            "+w${WIDTH}" "+h${HEIGHT}" -d +x +ft "+q${q}" \
            >/dev/null 2>&1 ) || true
        t1=$(date +%s.%N)
        printf "\t%.3f" "$(echo "${t1} - ${t0}" | bc)" >>"${TSV}"
    done
    printf "\n" >>"${TSV}"
    printf "[%3d/108] %s\n" "${scene_index}" "${base_name}" >&2
done

end_time=$(date +%s)
echo "renderQualityMatrix finished in $((end_time - start_time)) seconds." >&2
echo "Matrix written to ${TSV}" >&2
