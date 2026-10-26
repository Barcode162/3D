#!/usr/bin/env bash
# The piano's recorded notes (J100): the Salamander Grand Piano V3 by
# Alexander Holm (CC-BY 3.0; freepats.zenvoid.org), cut down for the phone:
# 30 notes (every third key, A0 to C8: A, C, D#, F#), 4 loudness layers
# (Salamander's 4, 8, 12 and 16 of 16), each shortened (long in the bass,
# short at the top) with a smooth fade, mono MP3 at 80 kbps (mono: the
# piano's 3D sound places it at the piano; J108.1).
#   bash tools/piano_samples.sh <scratch dir>    → assets/piano/<note>v<layer>.mp3
set -euo pipefail
cd "$(dirname "$0")/.."
scratch=${1:?usage: piano_samples.sh <scratch dir>}
src="$scratch/salamander/SalamanderGrandPianoV3_44.1khz16bit/44.1khz16bit"
if [ ! -d "$src" ]; then
  mkdir -p "$scratch/salamander"
  curl -fsSL -o "$scratch/salamander/sal.tar.xz" \
    "https://freepats.zenvoid.org/Piano/SalamanderGrandPiano/SalamanderGrandPianoV3+20161209_44khz16bit.tar.xz"
  tar -xJf "$scratch/salamander/sal.tar.xz" -C "$scratch/salamander"
fi
mkdir -p assets/piano
names=(A C D# F#)
for s in $(seq 0 29); do
  key=$((21 + 3 * s))                 # MIDI note
  name=${names[$((s % 4))]}
  octave=$(((key - 12) / 12))
  len=$(awk -v k=$key 'BEGIN { print (k < 48 ? 7 : (k < 72 ? 5.5 : 4)) }')
  fade=$(awk -v l=$len 'BEGIN { print l - 0.9 }')
  for v in 4 8 12 16; do
    file="${name/\#/s}"   # D# → Ds (a # in a web address starts its fragment)
    out="assets/piano/${file}${octave}v${v}.mp3"
    ffmpeg -loglevel error -y -i "$src/${name}${octave}v${v}.wav" -t "$len" -af "afade=t=out:st=${fade}:d=0.9" \
      -ac 1 -codec:a libmp3lame -b:a 80k "$out"
  done
done
du -sh assets/piano
