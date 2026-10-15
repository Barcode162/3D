#!/usr/bin/env bash
# Downloads the CC0 Poly Haven textures the asset scripts use (1k JPEGs).
# Usage: bash tools/assets/fetch_textures.sh <dir>
set -euo pipefail
dir=${1:?usage: fetch_textures.sh <dir>}
mkdir -p "$dir"
for id in wood_table_001 dark_wood; do
  for map in diff rough nor_gl; do
    f="$dir/${id}_${map}_1k.jpg"
    [ -s "$f" ] || curl -fsSL -o "$f" "https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/$id/${id}_${map}_1k.jpg"
  done
done
echo "Textures in $dir"
