// The jukebox, run by the room: when a song ends, the room starts the next
// one for everyone (the phones don't each decide). The list must match the
// built-in songs in bar/jukebox.cpp (kBuiltIn), in the same order; lengths in
// seconds, from ffprobe on assets/music/*.m4a (tools/native/run.sh checks the
// names still match).
export const SONGS = [
  { file: 'smooth-lovin.m4a', seconds: 259.40 },
  { file: 'bossa-antigua.m4a', seconds: 283.35 },
  { file: 'night-on-the-docks-sax.m4a', seconds: 174.05 },
  { file: 'lobby-time.m4a', seconds: 193.20 },
];
const SLACK_S = 1.0;  // a moment after the song's last note, so phones finish it

// When the song in the jukebox entry [song, startS] ends (server ms), or 0
// if nothing the room knows is playing. `testSeconds` replaces every song's
// length (tests only).
export function endsAtMs(values, testSeconds = 0) {
  if (!Array.isArray(values) || values.length < 2) return 0;
  const song = values[0], start = values[1];
  if (!Number.isInteger(song) || song < 0 || song >= SONGS.length || !(start > 0)) return 0;
  return (start + (testSeconds || SONGS[song].seconds) + SLACK_S) * 1000;
}

// The entry for the song after it, starting now.
export function next(values, nowMs) {
  return [(values[0] + 1) % SONGS.length, Math.round(nowMs) / 1000];
}
