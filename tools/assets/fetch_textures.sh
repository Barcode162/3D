#!/usr/bin/env bash
# Downloads CC0 Poly Haven textures (1k JPEGs: _diff, _rough, _nor_gl).
# Usage: bash tools/assets/fetch_textures.sh <dir> [polyhaven ids...]
# With no ids it fetches the ones the existing asset scripts use.
set -euo pipefail
dir=${1:?usage: fetch_textures.sh <dir> [ids...]}
shift
ids=("$@")
[ ${#ids[@]} -gt 0 ] || ids=(wood_table_001 dark_wood)
mkdir -p "$dir"
for id in "${ids[@]}"; do
  for map in diff rough nor_gl; do
    f="$dir/${id}_${map}_1k.jpg"
    [ -s "$f" ] || curl -fsSL -o "$f" "https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/$id/${id}_${map}_1k.jpg"
  done
done
echo "Textures in $dir"
