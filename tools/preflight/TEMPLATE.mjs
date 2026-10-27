// R-AREA-N · One plain sentence: what passing means.
// Copy this file to tools/preflight/rules/<area>/R-AREA-N.mjs (a new number
// in its area), fill it in, and run node tools/preflight.mjs all.
export default {
  id: 'R-AREA-N',
  area: 'area',
  rule: 'One plain sentence: what passing means.',
  why: 'Where it came from: an incident, a request, a Charter line.',
  // The files it watches (globs from the repo root): it runs when one of
  // them changed (and always with "all").
  when: ['path/**'],
  // Returns the broken spots, [] when it passes: where (a file, a thing's id)
  // and how to fix it, in plain words. ctx: changed, touched (the watched
  // files that changed), read(path), json(path), exists(path), list(dir),
  // isDir(path), git(command).
  check(ctx) {
    const spots = [];
    return spots;
  },
};
