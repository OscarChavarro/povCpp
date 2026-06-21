#!/usr/bin/env bash
set -euo pipefail

start_time=$(date +%s)

POVRAY_BIN="$(pwd)/build/povray"

mkdir -p output/level2 output/level3

# Same output paths renderAll.sh uses for these two scenes, so a subsequent
# testAgainstGoldenImages.sh run still finds exactly the expected files.
OUT_LEVEL2="$(pwd)/output/level2"
OUT_LEVEL3="$(pwd)/output/level3"

run_scene_parallel() {
    local output_dir="$1"
    local scene_dir="$2"
    local scene_file="$3"
    local include_path="$4"
    local base_name="${scene_file%.pov}"

    ( cd "${scene_dir}" && "${POVRAY_BIN}" \
        "+l${include_path}" \
        "+i${scene_file}" \
        "+o${output_dir}/${base_name}.tga" \
        +w1280 +h800 -d -v +x +ft -parallel )
}

# iortest.pov lives in etc/level2 (include at ../include);
# drums.pov lives in etc/level3/drums2 (include at ../../include).
# The two scenes are launched concurrently as background jobs; each one's own
# render is also parallelized internally via -parallel.
run_scene_parallel "${OUT_LEVEL2}" etc/level2 iortest.pov ../include &
pid_level2=$!
run_scene_parallel "${OUT_LEVEL3}" etc/level3/drums2 drums.pov ../../include &
pid_level3=$!

wait "${pid_level2}"
wait "${pid_level3}"

end_time=$(date +%s)
echo "renderParallel finished in $((end_time - start_time)) seconds."
