#!/bin/bash
# triageRmse.sh — compute RMSE for all test scenes and report bucket distribution

set -e
ROOT_DIR="$(dirname "${BASH_SOURCE[0]}")/.."
cd "$ROOT_DIR"

echo "=== Computing RMSE triage for all test scenes ==="

GOLDEN_DIR="$(readlink -f ../referenceTestImages)"
OUTPUT_DIR="$ROOT_DIR/output"

results_file="/tmp/rmse_results_$$.txt"
> "$results_file"

for ref in $(find "$GOLDEN_DIR" -name '*.tga' -type f | sort); do
    rel="${ref#$GOLDEN_DIR/}"
    out="$OUTPUT_DIR/$rel"

    if [[ ! -f "$out" ]]; then
        echo "MISSING $rel" >> "$results_file"
        continue
    fi

    if diff -q "$ref" "$out" >/dev/null 2>&1; then
        echo "PASS $rel" >> "$results_file"
        continue
    fi

    # Compute RMSE
    rmse=$(compare -metric RMSE "$out" "$ref" null: 2>&1 | sed -E 's/.*\(([0-9.]+)\).*/\1/')
    if [[ -z "$rmse" ]]; then
        rmse="ERROR"
    fi
    printf "FAIL %-26s %s\n" "$rel" "$rmse" >> "$results_file"
done

# Report results, sorted by RMSE
echo ""
echo "=== Results (sorted by RMSE) ==="
sort -t' ' -k3 -n "$results_file" | awk '{print $0}'

# Bucket distribution
echo ""
echo "=== RMSE Bucket Distribution ==="
awk '$2!="PASS" && $2!="MISSING" && $3!="" {
    val = $3
    if (val < 0.02) { n++; }
    else if (val < 0.05) { s++; }
    else if (val < 0.10) { m++; }
    else if (val < 0.20) { l++; }
    else { h++; }
} END {
    printf "  <0.02 (casi perfecto):  %d\n", (n ? n : 0)
    printf "  0.02-0.05 (leve):       %d\n", (s ? s : 0)
    printf "  0.05-0.10 (moderado):   %d\n", (m ? m : 0)
    printf "  0.10-0.20 (grande):     %d\n", (l ? l : 0)
    printf "  >0.20 (severo):         %d\n", (h ? h : 0)
}' "$results_file"

echo ""
echo "=== Summary ==="
grep -c "^PASS" "$results_file" || true | awk '{print $1 " PASS (pixel-perfect)"}'
grep -c "^FAIL" "$results_file" || true | awk '{print $1 " FAIL"}'
grep -c "^MISSING" "$results_file" || true | awk '{print $1 " MISSING"}'

cat "$results_file"
