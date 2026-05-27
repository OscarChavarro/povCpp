#!/usr/bin/env bash
#set -euo pipefail

# Test ANTLR rendering against golden images (pixel-perfect comparison)

ROOT_DIR="$(readlink -f "$(dirname "${BASH_SOURCE[0]}")/..")"
BIN="${ROOT_DIR}/povrayAntlr"
INCLUDE_PATH="${ROOT_DIR}/etc/include"
REF_IMAGES="${ROOT_DIR}/../referenceTestImages"
OUTPUT_DIR="${ROOT_DIR}/output_antlr_test"
THREADS=72

mkdir -p "${OUTPUT_DIR}"

# Create a list of all tests with their paths and directories
# Format: scene_dir scene_file level
declare -a TESTS

# Level 1 tests
pushd "${ROOT_DIR}/etc/level1" >/dev/null
for f in *.pov; do
  TESTS+=("level1 $f level1")
done
popd >/dev/null

pushd "${ROOT_DIR}/etc/level1/bumpmap" >/dev/null
TESTS+=("level1/bumpmap bumpmap.pov level1")
popd >/dev/null

# Level 2 tests
pushd "${ROOT_DIR}/etc/level2" >/dev/null
for f in *.pov; do
  TESTS+=("level2 $f level2")
done
popd >/dev/null

# Level 3 tests
pushd "${ROOT_DIR}/etc/level3" >/dev/null
for f in *.pov; do
  [[ "$f" == "car.pov" ]] && continue
  TESTS+=("level3 $f level3")
done
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/car" >/dev/null
TESTS+=("level3/car car.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/drums2" >/dev/null
TESTS+=("level3/drums2 drums.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/fish13" >/dev/null
TESTS+=("level3/fish13 fish13.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/ionic5" >/dev/null
TESTS+=("level3/ionic5 ionic5.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/ntreal" >/dev/null
TESTS+=("level3/ntreal ntreal.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/pencil" >/dev/null
TESTS+=("level3/pencil pencil.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/piece2" >/dev/null
TESTS+=("level3/piece2 piece2.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/piece3" >/dev/null
TESTS+=("level3/piece3 piece3.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/snail" >/dev/null
TESTS+=("level3/snail snail.pov level3")
popd >/dev/null

pushd "${ROOT_DIR}/etc/level3/teapot" >/dev/null
TESTS+=("level3/teapot teapot.pov level3")
popd >/dev/null

# Math tests
pushd "${ROOT_DIR}/etc/math" >/dev/null
for f in *.pov; do
  TESTS+=("math $f math")
done
popd >/dev/null

echo "Total tests: ${#TESTS[@]}"

# Function to run a single test
run_test() {
  local scene_dir="$1"
  local scene_file="$2"
  local level="$3"
  local base_name="${scene_file%.pov}"
  local test_dir="${ROOT_DIR}/etc/${scene_dir}"

  # Create output directory
  mkdir -p "${OUTPUT_DIR}/${level}"

  # Run test from its own directory
  (
    cd "${test_dir}"
    "${BIN}" \
      "+l${INCLUDE_PATH}" \
      "+i${scene_file}" \
      "+o${OUTPUT_DIR}/${level}/${base_name}.tga" \
      +w1280 +h800 -d -v +x +ft \
      >"${OUTPUT_DIR}/${level}/${base_name}_stdout.log" \
      2>"${OUTPUT_DIR}/${level}/${base_name}_stderr.log"
  ) && echo "RENDER: ${level}/${base_name}" || echo "CRASH: ${level}/${base_name}"
}

export -f run_test
export BIN INCLUDE_PATH OUTPUT_DIR ROOT_DIR

# Run tests in parallel
printf '%s\n' "${TESTS[@]}" | xargs -P "${THREADS}" -I{} bash -c 'IFS=" " read -r scene_dir scene_file level <<< "{}"; run_test "$scene_dir" "$scene_file" "$level"'

echo ""
echo "=== GOLDEN IMAGE COMPARISON ==="

PASS=0
FAIL=0
FAILED_TESTS=()

for ref in $(find "${REF_IMAGES}" -name "*.tga" -type f | sort); do
  rel_path="${ref#${REF_IMAGES}/}"
  output_file="${OUTPUT_DIR}/${rel_path}"

  if [[ -f "$output_file" ]]; then
    if diff -q "$ref" "$output_file" >/dev/null 2>&1; then
      ((PASS++))
      echo "PASS: ${rel_path}"
    else
      ((FAIL++))
      FAILED_TESTS+=("DIFF: ${rel_path}")
    fi
  else
    ((FAIL++))
    FAILED_TESTS+=("MISSING: ${rel_path}")
  fi
done

echo ""
echo "=== RESULTS ==="
echo "PASS:  $PASS/108"
echo "FAIL:  $FAIL/108"

if ((FAIL > 0)); then
  echo ""
  echo "Failed tests:"
  printf '%s\n' "${FAILED_TESTS[@]}"
  exit 1
else
  echo "All tests passed!"
  exit 0
fi
