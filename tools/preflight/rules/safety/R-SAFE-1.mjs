// R-SAFE-1 · No secret in the repo.
export default {
  id: 'R-SAFE-1',
  area: 'safety',
  rule: 'No key, token or password is written into a changed file.',
  why: 'Charter: never put a secret, key or password in the repo, a commit, a log or a reply.',
  when: ['**'],
  check(ctx) {
    const looks = [/\bsk-[A-Za-z0-9]{20,}/, /\bghp_[A-Za-z0-9]{30,}/, /\bgithub_pat_[A-Za-z0-9_]{30,}/, /\bAKIA[0-9A-Z]{16}\b/, /-----BEGIN [A-Z ]*PRIVATE KEY-----/, /\bxox[bap]-[A-Za-z0-9-]{20,}/];
    const spots = [];
    for (const f of ctx.changed) {
      if (!ctx.exists(f) || ctx.isDir(f) || /\.(glb|png|jpg|mp3|m4a|wasm|gz|ktx2|woff2?)$/i.test(f)) continue;
      let text;
      try { text = ctx.read(f); } catch { continue; }
      if (text.length > 3e6) continue;
      const hit = looks.find((re) => re.test(text));
      if (hit) spots.push({ where: f, fix: 'something that looks like a key or token is in this file: take it out (secrets live in the environment, never the repo)' });
    }
    return spots;
  },
};
