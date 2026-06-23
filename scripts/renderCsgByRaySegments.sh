#!/usr/bin/env bash
# Renders every scene with -csgRoth (boolean ray-segment CSG classification,
# see doc/CSGByRaySegments.md), into outputCsgByRaySegments/ so it never clobbers the
# default-algorithm output/ directory. Modeled on renderAll.sh, but scenes run
# one at a time (in the foreground) instead of all backgrounded at once;
# each render instead parallelizes internally via -parallel. This keeps
# resource usage predictable and per-scene logs easy to follow while still
# using all cores. Renders ALL scenes (not just CSG ones, Q5): the non-CSG
# scenes must come out byte-identical to the default golden, which is what
# proves the flag does not leak outside CSG evaluation.
set -euo pipefail

start_time=$(date +%s)

POVRAY_BIN="$(pwd)/build/povray"

mkdir -p outputCsgByRaySegments/level1 outputCsgByRaySegments/level2 outputCsgByRaySegments/level3 outputCsgByRaySegments/math

total_scenes=0
failed_scenes=0

render_scene() {
    local output_dir="$1"
    local scene_file="$2"
    local include_path="$3"
    local base_name="${scene_file%.pov}"

    total_scenes=$((total_scenes + 1))
    printf "[%d] %s ... " "$total_scenes" "$scene_file" >&2

    local scene_start_time scene_end_time scene_elapsed_seconds
    scene_start_time=$(date +%s)
    if "$POVRAY_BIN" \
        "+l${include_path}" \
        "+i${scene_file}" \
        "+o${output_dir}/${base_name}.tga" \
        +w1280 +h800 -d -v +x +ft -csgRoth -parallel \
        >"${output_dir}/${base_name}_stdout.log" \
        2>"${output_dir}/${base_name}_stderr.log"; then
        scene_end_time=$(date +%s)
        scene_elapsed_seconds=$((scene_end_time - scene_start_time))
        printf "%ds ... ok\n" "$scene_elapsed_seconds" >&2
    else
        scene_end_time=$(date +%s)
        scene_elapsed_seconds=$((scene_end_time - scene_start_time))
        failed_scenes=$((failed_scenes + 1))
        printf "%ds ... FAILED\n" "$scene_elapsed_seconds" >&2
    fi
}

cd etc
cd level1
cd bumpmap
render_scene ../../../outputCsgByRaySegments/level1 bumpmap.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level1 alphafun.pov ../include
render_scene ../../outputCsgByRaySegments/level1 ballbox1.pov ../include
render_scene ../../outputCsgByRaySegments/level1 basicvue.pov ../include
render_scene ../../outputCsgByRaySegments/level1 blob.pov ../include
render_scene ../../outputCsgByRaySegments/level1 box.pov ../include
render_scene ../../outputCsgByRaySegments/level1 cantelop.pov ../include
render_scene ../../outputCsgByRaySegments/level1 checker2.pov ../include
render_scene ../../outputCsgByRaySegments/level1 cliptst2.pov ../include
render_scene ../../outputCsgByRaySegments/level1 colors.pov ../include
render_scene ../../outputCsgByRaySegments/level1 dish.pov ../include
render_scene ../../outputCsgByRaySegments/level1 dodec2.pov ../include
render_scene ../../outputCsgByRaySegments/level1 fogtst.pov ../include
render_scene ../../outputCsgByRaySegments/level1 glasdish.pov ../include
render_scene ../../outputCsgByRaySegments/level1 glass.pov ../include
render_scene ../../outputCsgByRaySegments/level1 imagetst.pov ../include
render_scene ../../outputCsgByRaySegments/level1 intee1.pov ../include
render_scene ../../outputCsgByRaySegments/level1 laser.pov ../include
render_scene ../../outputCsgByRaySegments/level1 mapper.pov ../include
render_scene ../../outputCsgByRaySegments/level1 mappr2.pov ../include
render_scene ../../outputCsgByRaySegments/level1 matmap.pov ../include
render_scene ../../outputCsgByRaySegments/level1 pvinterp.pov ../include
render_scene ../../outputCsgByRaySegments/level1 shapes2.pov ../include
render_scene ../../outputCsgByRaySegments/level1 shapes.pov ../include
render_scene ../../outputCsgByRaySegments/level1 spotlite.pov ../include
render_scene ../../outputCsgByRaySegments/level1 stone1.pov ../include
render_scene ../../outputCsgByRaySegments/level1 stone2.pov ../include
render_scene ../../outputCsgByRaySegments/level1 stone3.pov ../include
render_scene ../../outputCsgByRaySegments/level1 stone4.pov ../include
render_scene ../../outputCsgByRaySegments/level1 sunset1.pov ../include
render_scene ../../outputCsgByRaySegments/level1 sunset.pov ../include
render_scene ../../outputCsgByRaySegments/level1 texture1.pov ../include
render_scene ../../outputCsgByRaySegments/level1 texture2.pov ../include
render_scene ../../outputCsgByRaySegments/level1 texture3.pov ../include
render_scene ../../outputCsgByRaySegments/level1 window.pov ../include
cd ..
cd level2
render_scene ../../outputCsgByRaySegments/level2 arches.pov ../include
render_scene ../../outputCsgByRaySegments/level2 cluster.pov ../include
render_scene ../../outputCsgByRaySegments/level2 crystal.pov ../include
render_scene ../../outputCsgByRaySegments/level2 eight.pov ../include
render_scene ../../outputCsgByRaySegments/level2 esp01.pov ../include
render_scene ../../outputCsgByRaySegments/level2 hfclip.pov ../include
render_scene ../../outputCsgByRaySegments/level2 illum1.pov ../include
render_scene ../../outputCsgByRaySegments/level2 illum2.pov ../include
render_scene ../../outputCsgByRaySegments/level2 iortest.pov ../include
render_scene ../../outputCsgByRaySegments/level2 lpops1.pov ../include
render_scene ../../outputCsgByRaySegments/level2 lpops2.pov ../include
render_scene ../../outputCsgByRaySegments/level2 magglass.pov ../include
render_scene ../../outputCsgByRaySegments/level2 mtmand.pov ../include
render_scene ../../outputCsgByRaySegments/level2 pacman.pov ../include
render_scene ../../outputCsgByRaySegments/level2 pawns.pov ../include
render_scene ../../outputCsgByRaySegments/level2 planet.pov ../include
render_scene ../../outputCsgByRaySegments/level2 poolball.pov ../include
render_scene ../../outputCsgByRaySegments/level2 romo.pov ../include
render_scene ../../outputCsgByRaySegments/level2 room.pov ../include
render_scene ../../outputCsgByRaySegments/level2 skyvase.pov ../include
render_scene ../../outputCsgByRaySegments/level2 smoke.pov ../include
render_scene ../../outputCsgByRaySegments/level2 spline.pov ../include
render_scene ../../outputCsgByRaySegments/level2 stonewal.pov ../include
render_scene ../../outputCsgByRaySegments/level2 sunsethf.pov ../include
render_scene ../../outputCsgByRaySegments/level2 tetra.pov ../include
render_scene ../../outputCsgByRaySegments/level2 waterbow.pov ../include
render_scene ../../outputCsgByRaySegments/level2 wtorus.pov ../include
cd ..
cd level3
cd car
render_scene ../../../outputCsgByRaySegments/level3 car.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level3 chess.pov ../include
render_scene ../../outputCsgByRaySegments/level3 desk.pov ../include
render_scene ../../outputCsgByRaySegments/level3 dfwood.pov ../include
cd drums2
render_scene ../../../outputCsgByRaySegments/level3 drums.pov ../../include
cd ..
cd fish13
render_scene ../../../outputCsgByRaySegments/level3 fish13.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level3 fishbowl.pov ../include
cd ionic5
render_scene ../../../outputCsgByRaySegments/level3 ionic5.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level3 kscope.pov ../include
render_scene ../../outputCsgByRaySegments/level3 lamp.pov ../include
cd ntreal
render_scene ../../../outputCsgByRaySegments/level3 ntreal.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level3 oak2.pov ../include
render_scene ../../outputCsgByRaySegments/level3 palace.pov ../include
cd pencil
render_scene ../../../outputCsgByRaySegments/level3 pencil.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level3 piece1.pov ../include
cd piece2
render_scene ../../../outputCsgByRaySegments/level3 piece2.pov ../../include
cd ..
cd piece3
render_scene ../../../outputCsgByRaySegments/level3 piece3.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level3 pool.pov ../include
render_scene ../../outputCsgByRaySegments/level3 roman.pov ../include
render_scene ../../outputCsgByRaySegments/level3 snack.pov ../include
cd snail
render_scene ../../../outputCsgByRaySegments/level3 snail.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level3 takeoff.pov ../include
cd teapot
render_scene ../../../outputCsgByRaySegments/level3 teapot.pov ../../include
cd ..
render_scene ../../outputCsgByRaySegments/level3 tomb.pov ../include
render_scene ../../outputCsgByRaySegments/level3 wealth.pov ../include
render_scene ../../outputCsgByRaySegments/level3 wg5.pov ../include
cd ..
cd math
render_scene ../../outputCsgByRaySegments/math bezier0.pov ../include
render_scene ../../outputCsgByRaySegments/math bezier.pov ../include
render_scene ../../outputCsgByRaySegments/math bicube.pov ../include
render_scene ../../outputCsgByRaySegments/math folium.pov ../include
render_scene ../../outputCsgByRaySegments/math grafbic.pov ../include
render_scene ../../outputCsgByRaySegments/math helix.pov ../include
render_scene ../../outputCsgByRaySegments/math hyptorus.pov ../include
render_scene ../../outputCsgByRaySegments/math lemnisc2.pov ../include
render_scene ../../outputCsgByRaySegments/math lemnisca.pov ../include
render_scene ../../outputCsgByRaySegments/math monkey.pov ../include
render_scene ../../outputCsgByRaySegments/math partorus.pov ../include
render_scene ../../outputCsgByRaySegments/math piriform.pov ../include
render_scene ../../outputCsgByRaySegments/math quarcyl.pov ../include
render_scene ../../outputCsgByRaySegments/math quarpara.pov ../include
render_scene ../../outputCsgByRaySegments/math steiner.pov ../include
render_scene ../../outputCsgByRaySegments/math tcubic.pov ../include
render_scene ../../outputCsgByRaySegments/math teardrop.pov ../include
render_scene ../../outputCsgByRaySegments/math torus.pov ../include
render_scene ../../outputCsgByRaySegments/math trough.pov ../include
render_scene ../../outputCsgByRaySegments/math witch.pov ../include
cd ..

end_time=$(date +%s)
elapsed_seconds=$((end_time - start_time))

if [ "$failed_scenes" -eq 0 ]; then
    echo "All ${total_scenes} scenes rendered. Total execution time: ${elapsed_seconds} seconds."
else
    echo "${total_scenes} scenes processed, ${failed_scenes} failed. Total execution time: ${elapsed_seconds} seconds."
fi
