// L1.6: lights with size, from the light tool's fit (node tools/light.mjs fit
// bar tools/migrations/l1_6_light_fit.json: each lamp fitted to the glowing
// object beside it). Tubes and panels found with enough glowing surface get
// their shape; balls (bulbs) get their glass's radius. Lamps with nothing
// glowing near them stay as they are. Kinds' own lights (the fire, the
// jukebox, the sign) are changed in the kind.
import { readFileSync } from 'node:fs';

const fit = JSON.parse(readFileSync(new URL('./l1_6_light_fit.json', import.meta.url), 'utf8'));
const round = (v) => v.map((x) => Math.round(x * 1000) / 1000);

function apply(light, f, where, out) {
  if (!light || !f || light.shape) return;
  if (f.shape === 'ball') {
    if (!(f.size > 0.02)) return;
    light.radius = Math.round(Math.min(0.3, Math.max(0.03, f.size / 2)) * 100) / 100;
    out.push(`${where}: a ball of ${light.radius} m`);
    return;
  }
  if ((f.corners ?? 0) < 18) return;
  light.shape = f.shape;
  light.center = round(f.center);
  light.axis = round(f.axis);
  light.length = Math.round(f.length * 100) / 100;
  if (f.shape === 'panel') {
    light.axis2 = round(f.axis2);
    light.width = Math.round(f.width * 100) / 100;
  }
  out.push(`${where}: a ${f.shape} ${light.length}${f.shape === 'panel' ? ' x ' + light.width : ''} m`);
}

export function migrate(file, json) {
  const out = [];
  const kindIds = new Set();
  for (const t of json.things ?? []) {
    const f = fit[t.id];
    if (!f) continue;
    if (t.parts?.light) apply(t.parts.light, f, `thing ${t.id}`, out);
    else kindIds.add(t.kind + '\u0000' + t.id);
  }
  // Lights that come from their kind (one thing of that kind in the bar).
  if (json.kinds) {
    for (const k of json.kinds) {
      const f = fit[k.id];
      if (f && k.parts?.light) apply(k.parts.light, f, `kind ${k.id}`, out);
    }
  }
  return out;
}
