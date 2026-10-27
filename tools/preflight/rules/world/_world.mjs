// Shared by the world rules: every place and kind file, kinds resolved through "is".
export function worldFiles(ctx) {
  const places = ctx.list('assets/world/places').filter((f) => f.endsWith('.json')).map((f) => `assets/world/places/${f}`);
  const kindFiles = ctx.list('assets/world/kinds').filter((f) => f.endsWith('.json')).map((f) => `assets/world/kinds/${f}`);
  const kinds = new Map();
  for (const f of kindFiles) {
    const j = ctx.json(f);
    for (const k of j.kinds || [j]) if (k && k.id) kinds.set(k.id, { ...k, file: f });
  }
  // A kind with everything it inherits ("is"), its own values winning.
  const resolve = (id, seen = new Set()) => {
    const k = kinds.get(id);
    if (!k || seen.has(id)) return k || null;
    seen.add(id);
    const base = k.is ? resolve(k.is, seen) : null;
    if (!base) return k;
    return { ...base, ...k, parts: { ...(base.parts || {}), ...(k.parts || {}) },
             states: { ...(base.states || {}), ...(k.states || {}) }, loops: { ...(base.loops || {}), ...(k.loops || {}) } };
  };
  return { places, kindFiles, kinds, resolve };
}
