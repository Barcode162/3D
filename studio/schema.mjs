// The world schema (assets/world/schema.json) in code: walks a world file and
// says which fields it uses that the schema doesn't describe, and which
// choices it uses that aren't in the lists. Shared by the Studio, the hub's
// Dials and tools/schema_test.mjs (an ES module: browser and node).
//   problems(schema, 'place' | 'kinds' | 'catalog' | 'songs' | 'materials', json, extra?) → ["where: what", ...]
//   fieldAt(schema, owner, key)  → the field's description (for panels)

// The description of a thing that holds fields: 'place', 'thing', 'kind', 'loop', ...
function holder(schema, type) { return schema[type] && schema[type].fields; }

function isVec(v, n) { return Array.isArray(v) && v.length === n && v.every((x) => typeof x === 'number'); }

function checkValue(schema, field, v, where, out) {
  const t = field.type;
  if (t === 'object') { checkFields(schema, field.fields || {}, v, where, out); return; }
  if (t === 'list') {
    if (!Array.isArray(v)) { out.push(`${where}: should be a list`); return; }
    v.forEach((x, i) => checkValue(schema, field.item || {}, x, `${where}[${i}]`, out));
    return;
  }
  if (t === 'map') {
    if (!v || typeof v !== 'object' || Array.isArray(v)) { out.push(`${where}: should be named entries`); return; }
    for (const [k, x] of Object.entries(v)) checkValue(schema, field.item || {}, x, `${where}.${k}`, out);
    return;
  }
  if (t === 'parts') { checkParts(schema, v, where, out); return; }
  if (holder(schema, t)) { checkFields(schema, holder(schema, t), v, where, out); return; }
  if (t === 'number' || t === 'int') { if (typeof v !== 'number') out.push(`${where}: should be a number`); return; }
  if (t === 'bool') { if (typeof v !== 'boolean') out.push(`${where}: should be true or false`); return; }
  if (t === 'vec3' || t === 'color') { if (!isVec(v, 3)) out.push(`${where}: should be [x, y, z]`); return; }
  if (t === 'rect') { if (!isVec(v, 4)) out.push(`${where}: should be [x0, z0, x1, z1]`); return; }
  if (t === 'box') { if (!(Array.isArray(v) && v.length === 2 && isVec(v[0], 3) && isVec(v[1], 3))) out.push(`${where}: should be two corners`); return; }
  if (t === 'quad') { if (!(Array.isArray(v) && v.length === 4 && v.every((c) => isVec(c, 3)))) out.push(`${where}: should be four corners`); return; }
  if (t === 'range') { if (!(typeof v === 'number' || (Array.isArray(v) && v.every((x) => typeof x === 'number')))) out.push(`${where}: should be a number or numbers`); return; }
  if (t === 'keys') { if (!(Array.isArray(v) && v.every((k) => Array.isArray(k) && k.length >= 2))) out.push(`${where}: should be [[time, value], ...]`); return; }
  if (t === 'states') { if (!(Array.isArray(v) && v.every((x) => typeof x === 'string'))) out.push(`${where}: should be state names`); return; }
  if (t === 'choice') {
    const list = [...(schema.choices[field.choices] || []), ...((extraChoices || {})[field.choices] || [])];
    if (!list.includes(v)) out.push(`${where}: "${v}" isn't one of the ${field.choices} choices`);
    return;
  }
  // text, id, kind, model, thingRef, stateRef, loopRef: text
  if (typeof v !== 'string') out.push(`${where}: should be text`);
}

function checkFields(schema, fields, obj, where, out) {
  if (!obj || typeof obj !== 'object' || Array.isArray(obj)) { out.push(`${where}: should be an object`); return; }
  for (const [k, v] of Object.entries(obj)) {
    const f = fields[k];
    if (!f) { out.push(`${where}.${k}: not in the schema`); continue; }
    checkValue(schema, f, v, `${where}.${k}`, out);
  }
}

function checkParts(schema, parts, where, out) {
  if (!parts || typeof parts !== 'object') { out.push(`${where}: should be parts`); return; }
  for (const [name, v] of Object.entries(parts)) {
    const p = schema.parts[name];
    if (!p) { out.push(`${where}.${name}: no such part in the schema`); continue; }
    if (p.type) { checkValue(schema, p, v, `${where}.${name}`, out); continue; }
    if (p.list) {
      if (!Array.isArray(v)) { out.push(`${where}.${name}: should be a list`); continue; }
      v.forEach((x, i) => checkFields(schema, p.fields, x, `${where}.${name}[${i}]`, out));
    } else checkFields(schema, p.fields, v, `${where}.${name}`, out);
  }
}

// extra: more choices the world itself adds ({material: ['blue-velvet']}, from materials.json).
let extraChoices = null;
export function problems(schema, what, json, extra = null) {
  extraChoices = extra;
  const out = [];
  if (what === 'place') checkFields(schema, schema.place.fields, json, 'place', out);
  else if (what === 'kinds') {
    for (const [k, v] of Object.entries(json)) {
      if (k === 'about') continue;
      if (k !== 'kinds') { out.push(`kinds.${k}: not in the schema`); continue; }
      v.forEach((kind, i) => checkFields(schema, schema.kind.fields, kind, `kind ${kind.id || i}`, out));
    }
  } else if (what === 'catalog') checkFields(schema, schema.catalog.fields, json, 'catalog', out);
  else if (what === 'materials') checkFields(schema, schema.materials.fields, json, 'materials', out);
  else if (what === 'songs') {
    for (const [k, v] of Object.entries(json)) if (k !== 'about') checkValue(schema, schema.songs.item, v, `songs.${k}`, out);
  }
  return out;
}

// A field's description, for drawing its control. owner: 'thing', 'kind',
// 'loop', 'piece', 'state', 'place', or 'part:<name>'.
export function fieldAt(schema, owner, key) {
  if (owner.startsWith('part:')) {
    const p = schema.parts[owner.slice(5)];
    return p && p.fields ? p.fields[key] : p;
  }
  const h = holder(schema, owner);
  return h ? h[key] : undefined;
}
