#!/usr/bin/env bash
set -euo pipefail

start_time=$(date +%s)

POVRAY_BIN="$(pwd)/build/povray"

mkdir -p output/level1 output/level2 output/level3 output/math

render_scene() {
    local output_dir="$1"
    local scene_file="$2"
    local include_path="$3"
    local base_name="${scene_file%.pov}"

    "$POVRAY_BIN" \
        "+l${include_path}" \
        "+i${scene_file}" \
        "+o${output_dir}/${base_name}.tga" \
        +w1280 +h800 -d -v +x +ft \
        >"${output_dir}/${base_name}_stdout.log" \
        2>"${output_dir}/${base_name}_stderr.log" &
}

cd etc
cd level1
cd bumpmap
render_scene ../../../output/level1 bumpmap.pov ../../include
cd ..
render_scene ../../output/level1 alphafun.pov ../include
render_scene ../../output/level1 ballbox1.pov ../include
render_scene ../../output/level1 basicvue.pov ../include
render_scene ../../output/level1 blob.pov ../include
render_scene ../../output/level1 box.pov ../include
render_scene ../../output/level1 cantelop.pov ../include
render_scene ../../output/level1 checker2.pov ../include
render_scene ../../output/level1 cliptst2.pov ../include
render_scene ../../output/level1 colors.pov ../include
render_scene ../../output/level1 dish.pov ../include
render_scene ../../output/level1 dodec2.pov ../include
render_scene ../../output/level1 fogtst.pov ../include
render_scene ../../output/level1 glasdish.pov ../include
render_scene ../../output/level1 glass.pov ../include
render_scene ../../output/level1 imagetst.pov ../include
render_scene ../../output/level1 intee1.pov ../include
render_scene ../../output/level1 laser.pov ../include
render_scene ../../output/level1 mapper.pov ../include
render_scene ../../output/level1 mappr2.pov ../include
render_scene ../../output/level1 matmap.pov ../include
render_scene ../../output/level1 pvinterp.pov ../include
render_scene ../../output/level1 shapes2.pov ../include
render_scene ../../output/level1 shapes.pov ../include
render_scene ../../output/level1 spotlite.pov ../include
render_scene ../../output/level1 stone1.pov ../include
render_scene ../../output/level1 stone2.pov ../include
render_scene ../../output/level1 stone3.pov ../include
render_scene ../../output/level1 stone4.pov ../include
render_scene ../../output/level1 sunset1.pov ../include
render_scene ../../output/level1 sunset.pov ../include
render_scene ../../output/level1 texture1.pov ../include
render_scene ../../output/level1 texture2.pov ../include
render_scene ../../output/level1 texture3.pov ../include
render_scene ../../output/level1 window.pov ../include
cd ..
cd level2
render_scene ../../output/level2 arches.pov ../include
render_scene ../../output/level2 cluster.pov ../include
render_scene ../../output/level2 crystal.pov ../include
render_scene ../../output/level2 eight.pov ../include
render_scene ../../output/level2 esp01.pov ../include
render_scene ../../output/level2 hfclip.pov ../include
render_scene ../../output/level2 illum1.pov ../include
render_scene ../../output/level2 illum2.pov ../include
render_scene ../../output/level2 iortest.pov ../include
render_scene ../../output/level2 lpops1.pov ../include
render_scene ../../output/level2 lpops2.pov ../include
render_scene ../../output/level2 magglass.pov ../include
render_scene ../../output/level2 mtmand.pov ../include
render_scene ../../output/level2 pacman.pov ../include
render_scene ../../output/level2 pawns.pov ../include
render_scene ../../output/level2 planet.pov ../include
render_scene ../../output/level2 poolball.pov ../include
render_scene ../../output/level2 romo.pov ../include
render_scene ../../output/level2 room.pov ../include
render_scene ../../output/level2 skyvase.pov ../include
render_scene ../../output/level2 smoke.pov ../include
render_scene ../../output/level2 spline.pov ../include
render_scene ../../output/level2 stonewal.pov ../include
render_scene ../../output/level2 sunsethf.pov ../include
render_scene ../../output/level2 tetra.pov ../include
render_scene ../../output/level2 waterbow.pov ../include
render_scene ../../output/level2 wtorus.pov ../include
cd ..
cd level3
cd car
render_scene ../../../output/level3 car.pov ../../include
cd ..
render_scene ../../output/level3 chess.pov ../include
render_scene ../../output/level3 desk.pov ../include
render_scene ../../output/level3 dfwood.pov ../include
cd drums2
render_scene ../../../output/level3 drums.pov ../../include
cd ..
cd fish13
render_scene ../../../output/level3 fish13.pov ../../include
cd ..
render_scene ../../output/level3 fishbowl.pov ../include
cd ionic5
render_scene ../../../output/level3 ionic5.pov ../../include
cd ..
render_scene ../../output/level3 kscope.pov ../include
render_scene ../../output/level3 lamp.pov ../include
cd ntreal
render_scene ../../../output/level3 ntreal.pov ../../include
cd ..
render_scene ../../output/level3 oak2.pov ../include
render_scene ../../output/level3 palace.pov ../include
cd pencil
render_scene ../../../output/level3 pencil.pov ../../include
cd ..
render_scene ../../output/level3 piece1.pov ../include
cd piece2
render_scene ../../../output/level3 piece2.pov ../../include
cd ..
cd piece3
render_scene ../../../output/level3 piece3.pov ../../include
cd ..
render_scene ../../output/level3 pool.pov ../include
render_scene ../../output/level3 roman.pov ../include
render_scene ../../output/level3 snack.pov ../include
cd snail
render_scene ../../../output/level3 snail.pov ../../include
cd ..
render_scene ../../output/level3 takeoff.pov ../include
cd teapot
render_scene ../../../output/level3 teapot.pov ../../include
cd ..
render_scene ../../output/level3 tomb.pov ../include
render_scene ../../output/level3 wealth.pov ../include
render_scene ../../output/level3 wg5.pov ../include
cd ..
cd math
render_scene ../../output/math bezier0.pov ../include
render_scene ../../output/math bezier.pov ../include
render_scene ../../output/math bicube.pov ../include
render_scene ../../output/math folium.pov ../include
render_scene ../../output/math grafbic.pov ../include
render_scene ../../output/math helix.pov ../include
render_scene ../../output/math hyptorus.pov ../include
render_scene ../../output/math lemnisc2.pov ../include
render_scene ../../output/math lemnisca.pov ../include
render_scene ../../output/math monkey.pov ../include
render_scene ../../output/math partorus.pov ../include
render_scene ../../output/math piriform.pov ../include
render_scene ../../output/math quarcyl.pov ../include
render_scene ../../output/math quarpara.pov ../include
render_scene ../../output/math steiner.pov ../include
render_scene ../../output/math tcubic.pov ../include
render_scene ../../output/math teardrop.pov ../include
render_scene ../../output/math torus.pov ../include
render_scene ../../output/math trough.pov ../include
render_scene ../../output/math witch.pov ../include
cd ..

pids=($(jobs -p))
failed_jobs=0
completed_jobs=0

for pid in "${pids[@]}"; do
    if ! wait "$pid"; then
        failed_jobs=$((failed_jobs + 1))
    fi

    completed_jobs=$((completed_jobs + 1))
    printf "." >&2

    if ((completed_jobs % 50 == 0)); then
        printf "\n" >&2
    elif ((completed_jobs % 10 == 0)); then
        printf " " >&2
    fi
done

if ((completed_jobs % 50 != 0)); then
    printf "\n" >&2
fi

end_time=$(date +%s)
elapsed_seconds=$((end_time - start_time))

if [ "$failed_jobs" -eq 0 ]; then
    echo "All render processes have finished. Total execution time: ${elapsed_seconds} seconds."
else
    echo "All render processes have finished. ${failed_jobs} process(es) failed. Total execution time: ${elapsed_seconds} seconds."
fi
