#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")/..")"
BIN="${ROOT_DIR}/povrayAntlr"

if [[ ! -x "${BIN}" ]]; then
  echo "Missing binary: ${BIN}. Run ./scripts/compileAntrl.sh first." >&2
  exit 1
fi

STAMP="$(date +%Y%m%d_%H%M%S)"
export GMON_OUT_PREFIX="/tmp/gmon_antlr_${STAMP}"

echo "Using GMON_OUT_PREFIX=${GMON_OUT_PREFIX}" >&2

render_scene() {
  local output_dir="$1"
  local scene_file="$2"
  local include_path="$3"
  local base_name="${scene_file%.pov}"

  mkdir -p "${output_dir}"
  "${BIN}" \
    "+l${include_path}" \
    "+i${scene_file}" \
    "+o${output_dir}/${base_name}.tga" \
    +w1280 +h800 -d -v +x +ft \
    >"${output_dir}/${base_name}_stdout.log" \
    2>"${output_dir}/${base_name}_stderr.log" &
}

run_all_scenes() {
  local output_root="${ROOT_DIR}/output"

  mkdir -p "${output_root}/level1" "${output_root}/level2" "${output_root}/level3" "${output_root}/math"
  pushd "${ROOT_DIR}/etc" >/dev/null

  pushd level1 >/dev/null
  pushd bumpmap >/dev/null
  render_scene ../../../output/level1 bumpmap.pov ../../include
  popd >/dev/null
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
  popd >/dev/null

  pushd level2 >/dev/null
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
  popd >/dev/null

  pushd level3 >/dev/null
  pushd car >/dev/null
  render_scene ../../../output/level3 car.pov ../../include
  popd >/dev/null
  render_scene ../../output/level3 chess.pov ../include
  render_scene ../../output/level3 desk.pov ../include
  render_scene ../../output/level3 dfwood.pov ../include
  pushd drums2 >/dev/null
  render_scene ../../../output/level3 drums.pov ../../include
  popd >/dev/null
  pushd fish13 >/dev/null
  render_scene ../../../output/level3 fish13.pov ../../include
  popd >/dev/null
  render_scene ../../output/level3 fishbowl.pov ../include
  pushd ionic5 >/dev/null
  render_scene ../../../output/level3 ionic5.pov ../../include
  popd >/dev/null
  render_scene ../../output/level3 kscope.pov ../include
  render_scene ../../output/level3 lamp.pov ../include
  pushd ntreal >/dev/null
  render_scene ../../../output/level3 ntreal.pov ../../include
  popd >/dev/null
  render_scene ../../output/level3 oak2.pov ../include
  render_scene ../../output/level3 palace.pov ../include
  pushd pencil >/dev/null
  render_scene ../../../output/level3 pencil.pov ../../include
  popd >/dev/null
  render_scene ../../output/level3 piece1.pov ../include
  pushd piece2 >/dev/null
  render_scene ../../../output/level3 piece2.pov ../../include
  popd >/dev/null
  pushd piece3 >/dev/null
  render_scene ../../../output/level3 piece3.pov ../../include
  popd >/dev/null
  render_scene ../../output/level3 pool.pov ../include
  render_scene ../../output/level3 roman.pov ../include
  render_scene ../../output/level3 snack.pov ../include
  pushd snail >/dev/null
  render_scene ../../../output/level3 snail.pov ../../include
  popd >/dev/null
  render_scene ../../output/level3 takeoff.pov ../include
  pushd teapot >/dev/null
  render_scene ../../../output/level3 teapot.pov ../../include
  popd >/dev/null
  render_scene ../../output/level3 tomb.pov ../include
  render_scene ../../output/level3 wealth.pov ../include
  render_scene ../../output/level3 wg5.pov ../include
  popd >/dev/null

  pushd math >/dev/null
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
  popd >/dev/null

  popd >/dev/null

  local pids=($(jobs -p))
  local failed_jobs=0
  local completed_jobs=0
  local pid
  for pid in "${pids[@]}"; do
    if ! wait "${pid}"; then
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
  echo "Rendered ${completed_jobs} scene(s), failures: ${failed_jobs}" >&2
  ((failed_jobs == 0))
}

if (( $# == 0 )); then
  run_all_scenes
else
  "${BIN}" "$@"
fi

PROFILE_FILE="$(ls -1 ${GMON_OUT_PREFIX}.* 2>/dev/null | head -n 1 || true)"
if [[ -n "${PROFILE_FILE}" ]]; then
  echo "Profile: ${PROFILE_FILE}" >&2
else
  echo "Warning: no gprof profile file found for prefix ${GMON_OUT_PREFIX}" >&2
fi
