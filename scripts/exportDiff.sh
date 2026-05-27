#!/bin/bash
# exportDiff.sh [<level>/<scene> ...]   (sin args = todas las escenas)
# Genera en /tmp/i, por escena:
#   <scene>.png          salida ANTLR
#   <scene>_golden.png   referencia golden
#   <scene>_legacy.png   salida legacy (si existe)
#   <scene>_diff.png     ANTLR vs GOLDEN: fondo blanco, diferencias en ROJO
#   <scene>_difflegacy.png ANTLR vs LEGACY: diferencias en ROJO (señal de parseo)
#   <scene>_compare.png  montage [ANTLR | golden | diff] lado a lado
# Imprime el conteo de píxeles distintos (AE) por escena.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT=/tmp/i ; mkdir -p "$OUT"
GOLDEN_DIR="$(readlink -f "$ROOT/../referenceTestImages")"
ANTLR_DIR="$ROOT/output"
LEGACY_DIR="$ROOT/output_legacy"

targets=("$@")
if [ ${#targets[@]} -eq 0 ]; then
  mapfile -t targets < <(cd "$GOLDEN_DIR" && find . -name '*.tga' | sed 's#^\./##; s#\.tga$##')
fi

printf "%-26s %12s %12s\n" "scene" "AE_vs_golden" "AE_vs_legacy"
for rel in "${targets[@]}"; do
  scene="${rel##*/}"
  g="$GOLDEN_DIR/$rel.tga" ; a="$ANTLR_DIR/$rel.tga" ; l="$LEGACY_DIR/$rel.tga"
  [ -f "$a" ] || { printf "%-26s %12s\n" "$rel" "NO_ANTLR"; continue; }
  convert "$a" "$OUT/${scene}.png" 2>/dev/null
  [ -f "$g" ] && convert "$g" "$OUT/${scene}_golden.png" 2>/dev/null
  [ -f "$l" ] && convert "$l" "$OUT/${scene}_legacy.png" 2>/dev/null
  ae_g="-" ; ae_l="-"
  if [ -f "$g" ]; then
    ae_g=$(compare -metric AE -fuzz 0 -highlight-color red -lowlight-color white \
           "$OUT/${scene}.png" "$OUT/${scene}_golden.png" "$OUT/${scene}_diff.png" 2>&1)
    montage "$OUT/${scene}.png" "$OUT/${scene}_golden.png" "$OUT/${scene}_diff.png" \
            -tile 3x1 -geometry +2+2 "$OUT/${scene}_compare.png" 2>/dev/null
  fi
  if [ -f "$l" ]; then
    ae_l=$(compare -metric AE -fuzz 0 -highlight-color red -lowlight-color white \
           "$OUT/${scene}.png" "$OUT/${scene}_legacy.png" "$OUT/${scene}_difflegacy.png" 2>&1)
  fi
  printf "%-26s %12s %12s\n" "$rel" "$ae_g" "$ae_l"
done
