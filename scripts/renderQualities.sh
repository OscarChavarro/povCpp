#!/usr/bin/env bash
#
# Renders the quality golden gate's fixtures into output/qualities/.
#
# Two coverage tiers, both consumed by scripts/testQualities.sh:
#
# 1. etc/level2/iortest.pov at every POV-Ray quality level (+q0 .. +q9),
#    flat into output/qualities/iortest_q{0..9}.tga. iortest.pov is chosen
#    because it exercises every feature the quality knob gates: flat
#    colours, diffuse/specular lighting, shadows, full procedural/checker
#    textures, refraction (ior.inc) and reflection - so each of its ten
#    images is a tight, full-resolution-of-detail witness that the quality
#    band mapping (doc/vitralNormalizationAnalysis.md §7) stays bit-identical.
#
# 2. All 108 gate scenes (same set as scripts/renderQualityMatrix.sh /
#    scripts/renderAll.sh) at one representative quality per band -
#    q0, q2, q4, q6, q8 - into output/qualities/<level>/<scene>_q<N>.tga,
#    mirroring renderAll.sh's per-level (not per-scene-subdirectory)
#    output layout. This is the broader, shallower check: it does not
#    re-verify every one of iortest's ten levels on every scene, but it
#    confirms the band mapping holds across the whole scene corpus, not
#    just the one spanning fixture (so iortest is not a single point of
#    failure for the band-fidelity check).
#
# A small resolution is used on purpose: the gate only needs to be
# deterministic, not high-resolution.
set -euo pipefail

start_time=$(date +%s)

POVRAY_BIN="$(pwd)/build/povray"
OUT_DIR="$(pwd)/output/qualities"
WIDTH=320
HEIGHT=200

if [[ ! -x "${POVRAY_BIN}" ]]; then
    echo "Missing build/povray binary; run ./scripts/compile.sh first." >&2
    exit 1
fi

mkdir -p "${OUT_DIR}"

# --- Tier 1: iortest.pov at all ten quality levels ---
for quality in 0 1 2 3 4 5 6 7 8 9; do
    base_name="iortest_q${quality}"
    ( cd etc/level2 && "${POVRAY_BIN}" \
        "+l../include" \
        "+iiortest.pov" \
        "+o${OUT_DIR}/${base_name}.tga" \
        "+w${WIDTH}" "+h${HEIGHT}" -d +x +ft "+q${quality}" \
        >"${OUT_DIR}/${base_name}_stdout.log" \
        2>"${OUT_DIR}/${base_name}_stderr.log" )
    printf "q%d " "${quality}" >&2
done
printf "\n" >&2

# --- Tier 2: all 108 gate scenes at one representative quality per band ---
# Each entry: "<scene_dir under etc>|<scene_file>|<include path from
# scene_dir>". Mirrors the exact scene set and include layout of
# scripts/renderAll.sh / scripts/renderQualityMatrix.sh.
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

for level in level1 level2 level3 math; do
    mkdir -p "${OUT_DIR}/${level}"
done

for entry in "${scenes[@]}"; do
    IFS='|' read -r scene_dir scene_file include_path <<<"${entry}"
    base_name="${scene_file%.pov}"
    level="${scene_dir%%/*}"

    for quality in 0 2 4 6 8; do
        out_name="${base_name}_q${quality}"
        ( cd "etc/${scene_dir}" && "${POVRAY_BIN}" \
            "+l${include_path}" \
            "+i${scene_file}" \
            "+o${OUT_DIR}/${level}/${out_name}.tga" \
            "+w${WIDTH}" "+h${HEIGHT}" -d +x +ft "+q${quality}" \
            >"${OUT_DIR}/${level}/${out_name}_stdout.log" \
            2>"${OUT_DIR}/${level}/${out_name}_stderr.log" ) &
    done
done

failed_jobs=0
completed_jobs=0
for pid in $(jobs -p); do
    if ! wait "${pid}"; then
        failed_jobs=$((failed_jobs + 1))
    fi
    completed_jobs=$((completed_jobs + 1))
    printf "." >&2
    if ((completed_jobs % 50 == 0)); then
        printf "\n" >&2
    fi
done
printf "\n" >&2

end_time=$(date +%s)
if ((failed_jobs == 0)); then
    echo "renderQualities finished in $((end_time - start_time)) seconds."
else
    echo "renderQualities finished in $((end_time - start_time)) seconds. ${failed_jobs} render(s) failed." >&2
    exit 1
fi
