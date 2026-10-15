#!/usr/bin/env bash
# Downloads CC0 Poly Haven textures (1k JPEGs), saved as <id>_diff_1k.jpg,
# <id>_rough_1k.jpg and <id>_nor_gl_1k.jpg whatever Poly Haven calls them
# (some sets name the color map "albedo"). Uses the files API for the URLs.
# Usage: bash tools/assets/fetch_textures.sh <dir> [polyhaven ids...]
# With no ids it fetches every set the asset scripts use.
set -euo pipefail
dir=${1:?usage: fetch_textures.sh <dir> [ids...]}
shift
ids=("$@")
[ ${#ids[@]} -gt 0 ] || ids=(wood_table_001 dark_wood red_brick_03 plastered_wall_02
  wood_floor leather_red_02 fabric_leather_02)
mkdir -p "$dir"
for id in "${ids[@]}"; do
  [ -s "$dir/${id}_diff_1k.jpg" ] && [ -s "$dir/${id}_rough_1k.jpg" ] && [ -s "$dir/${id}_nor_gl_1k.jpg" ] && continue
  curl -fsSL "https://api.polyhaven.com/files/$id" | python3 -c '
import json, sys
d = json.load(sys.stdin)
color = "Diffuse" if "Diffuse" in d else "coll1"  # color variants are named coll1, coll2...
for key, short in ((color, "diff"), ("Rough", "rough"), ("nor_gl", "nor_gl")):
    print(short, d[key]["1k"]["jpg"]["url"])' | while read -r short url; do
    curl -fsSL -o "$dir/${id}_${short}_1k.jpg" "$url"
  done
done
echo "Textures in $dir"
