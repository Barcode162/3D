// L1.2: every light gets real units: lumens (from its old intensity, about
// 180 lm per unit) and, for the warm white bulbs, a colour temperature in
// Kelvin with a white filter (the colour now comes from the temperature).
// Coloured lights (neon, signs, the jukebox) keep their colour as a filter.
const near = (c, r) => Array.isArray(c) && c.length === 3 && c.every((v, i) => Math.abs(v - r[i]) < 0.04);
const WARM = [[[1, 0.68, 0.38], 2400], [[1, 0.55, 0.2], 1900], [[1, 0.45, 0.15], 1700], [[1, 0.8, 0.6], 3000],
              [[1, 0.92, 0.75], 3800], [[1, 0.45, 0.25], 0]];

function light(l, where, out) {
  if (!l || typeof l !== 'object' || l.lumens !== undefined) return;
  if (l.intensity === undefined && l.color === undefined) return;
  l.lumens = Math.round((l.intensity ?? 1) * 180);
  for (const [c, k] of WARM) {
    if (k && near(l.color, c)) { l.kelvin = k; l.color = [1, 1, 1]; break; }
  }
  out.push(`${where}: ${l.lumens} lm${l.kelvin ? `, ${l.kelvin} K` : ''}`);
}

export function migrate(file, json) {
  const out = [];
  for (const k of json.kinds ?? []) light(k.parts?.light, `kind ${k.id}`, out);
  for (const t of json.things ?? []) light(t.parts?.light, `thing ${t.id}`, out);
  for (const [mood, m] of Object.entries(json.moods ?? {})) {
    for (const [id, l] of Object.entries(m.lights ?? {})) {
      // A mood's light: lumens from its intensity; its colour stays (moods tint).
      if (l.intensity !== undefined && l.lumens === undefined) { l.lumens = Math.round(l.intensity * 180); out.push(`mood ${mood} ${id}: ${l.lumens} lm`); }
    }
  }
  return out;
}
