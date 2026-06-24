#!/usr/bin/env bash
#
# Renders a single reference scene (etc/level2/iortest.pov) once per POV-Ray
# quality level (+q0 .. +q9) into output/qualities/. iortest.pov is chosen
# because it exercises every feature the quality knob gates: flat colours,
# diffuse/specular lighting, shadows, full procedural/checker textures,
# refraction (ior.inc) and reflection. So each quality band produces a
# visibly distinct image, which makes this scene a good spanning fixture for
# the quality golden gate (scripts/testQualities.sh) and for the lazy-quality
# work described in doc/lazyQualitySelectionPlan.md.
#
# A small resolution is used on purpose: the gate runs 10 renders and only
# needs to be deterministic, not high-resolution.
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

end_time=$(date +%s)
echo "renderQualities finished in $((end_time - start_time)) seconds."
