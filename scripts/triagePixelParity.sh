#!/bin/bash
# triagePixelParity.sh
# Para cada escena imprime 3 RMSE:
# legacy_vs_golden, antlr_vs_golden, antlr_vs_legacy
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GOLDEN_DIR="$(readlink -f "$ROOT/../referenceTestImages")"
ANTLR_DIR="$ROOT/output"
LEGACY_DIR="$ROOT/output_legacy"

printf "%-26s %-10s %-10s %-10s\n" "scene" "L/G" "A/G" "A/L"
for ref in $(find "$GOLDEN_DIR" -name '*.tga' | sort); do
  rel=${ref#$GOLDEN_DIR/}
  rel=${rel%.tga}

  lg="-"
  ag="-"
  al="-"

  if [ -f "$LEGACY_DIR/$rel.tga" ]; then
    lg=$(compare -metric RMSE "$LEGACY_DIR/$rel.tga" "$ref" null: 2>&1 | sed -E 's/.*\(([0-9.]+)\).*/\1/')
  fi
  if [ -f "$ANTLR_DIR/$rel.tga" ]; then
    ag=$(compare -metric RMSE "$ANTLR_DIR/$rel.tga" "$ref" null: 2>&1 | sed -E 's/.*\(([0-9.]+)\).*/\1/')
    if [ -f "$LEGACY_DIR/$rel.tga" ]; then
      al=$(compare -metric RMSE "$ANTLR_DIR/$rel.tga" "$LEGACY_DIR/$rel.tga" null: 2>&1 | sed -E 's/.*\(([0-9.]+)\).*/\1/')
    fi
  fi

  printf "%-26s %-10s %-10s %-10s\n" "$rel" "$lg" "$ag" "$al"
done | sort -t= -k4 -rn
