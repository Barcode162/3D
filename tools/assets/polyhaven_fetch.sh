#!/usr/bin/env bash
# Downloads one Poly Haven model (CC0) as glTF with its textures at one
# size (default 1k) into <dir>/<id>/, and its page facts (name, authors,
# size in mm) into <dir>/<id>/info.json, for polyhaven.py and CREDITS.md.
#   bash tools/assets/polyhaven_fetch.sh <asset id> <dir> [1k|2k|4k]
set -euo pipefail
id=${1:?usage: polyhaven_fetch.sh <asset id> <dir> [res]}
dir=${2:?usage: polyhaven_fetch.sh <asset id> <dir> [res]}
res=${3:-1k}
out="$dir/$id"
mkdir -p "$out"
curl -fsSL "https://api.polyhaven.com/info/$id" -o "$out/info.json"
curl -fsSL "https://api.polyhaven.com/files/$id" -o "$out/files.json"
# The glTF and every file it includes (the .bin and the textures).
python3 -I - "$out/files.json" "$res" > "$out/urls.txt" <<'PY'
import json, sys
f = json.load(open(sys.argv[1]))["gltf"][sys.argv[2]]["gltf"]
print(f["url"], "model.gltf")
for path, v in f["include"].items():
    print(v["url"], path)
PY
while read -r url path; do
  mkdir -p "$out/$(dirname "$path")"
  curl -fsSL "$url" -o "$out/$path"
done < "$out/urls.txt"
echo "FETCHED $id ($res) → $out"
