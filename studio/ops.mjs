// Edits as operations (docs/STUDIO.md 7.4). The draft is a bundle (its
// files: {"places/bar.json": {...}, "kinds/bar.json": {...}}); an op changes
// one spot in one file and gives back its inverse, so undo is applying the
// inverse. Paths name array entries by id ({id: "table-3"}), never by index,
// so ops stay right when other things are added or removed.
//   const undo = apply(files, op)      changes files, returns the inverse op
//   set(file, path, value)             an op that sets one value
//   add(file, path, item) / remove(file, path, id)   things in a list
//   group(ops, label)                  several ops as one (one undo)
// Shared by the Studio (browser) and tools/ops_test.mjs (node).

const clone = (v) => (v === undefined ? undefined : JSON.parse(JSON.stringify(v)));

// The container a path points into, and the last step.
function locate(root, path, make) {
  let node = root;
  for (let i = 0; i < path.length - 1; i++) {
    const step = path[i];
    let next;
    if (typeof step === 'object') next = Array.isArray(node) ? node.find((x) => x && x.id === step.id) : undefined;
    else next = node[step];
    if (next === undefined) {
      if (!make) throw new Error(`no ${typeof step === 'object' ? step.id : step} in ${path.slice(0, i).map(name).join('.') || 'the file'}`);
      next = typeof path[i + 1] === 'number' || typeof path[i + 1] === 'object' ? [] : {};
      node[step] = next;
    }
    node = next;
  }
  return [node, path[path.length - 1]];
}
const name = (s) => (typeof s === 'object' ? `[${s.id}]` : String(s));

export function set(file, path, value) { return { op: 'set', file, path, value }; }
export function add(file, path, item, index) { return { op: 'add', file, path, item, index }; }
export function remove(file, path, id) { return { op: 'remove', file, path, id }; }
export function group(ops, label = '') { return { op: 'group', ops, label }; }

export function apply(files, op) {
  if (op.op === 'group') {
    const undo = [];
    for (const o of op.ops) undo.unshift(apply(files, o));
    return group(undo, op.label);
  }
  const root = files[op.file];
  if (!root) throw new Error(`no file ${op.file}`);
  if (op.op === 'set') {
    const [node, last] = locate(root, op.path, true);
    const key = typeof last === 'object' ? node.findIndex((x) => x && x.id === last.id) : last;
    const before = clone(node[key]);
    if (op.value === undefined) {
      if (Array.isArray(node)) node.splice(key, 1); else delete node[key];
    } else node[key] = clone(op.value);
    return set(op.file, op.path, before);
  }
  if (op.op === 'add') {
    const [node, last] = locate(root, op.path, true);
    if (!Array.isArray(node[last])) node[last] = [];
    const list = node[last];
    if (op.item && op.item.id && list.some((x) => x && x.id === op.item.id)) throw new Error(`${op.item.id} is already there`);
    const at = op.index === undefined || op.index > list.length ? list.length : op.index;
    list.splice(at, 0, clone(op.item));
    return remove(op.file, op.path, op.item.id);
  }
  if (op.op === 'remove') {
    const [node, last] = locate(root, op.path, false);
    const list = node[last];
    const i = Array.isArray(list) ? list.findIndex((x) => x && x.id === op.id) : -1;
    if (i < 0) throw new Error(`no ${op.id} to remove`);
    const [item] = list.splice(i, 1);
    return add(op.file, op.path, item, i);
  }
  throw new Error(`unknown op ${op.op}`);
}

// The value a path points at now (undefined if none).
export function get(files, file, path) {
  try {
    const [node, last] = locate(files[file], path, false);
    if (typeof last === 'object') return Array.isArray(node) ? node.find((x) => x && x.id === last.id) : undefined;
    return node[last];
  } catch (e) { return undefined; }
}

// An undo/redo history of op groups (one per gesture).
export class History {
  constructor(files) { this.files = files; this.done = []; this.undone = []; }
  do(op) { const inv = apply(this.files, op); this.done.push({ op, inv }); this.undone = []; return inv; }
  undo() { const e = this.done.pop(); if (!e) return null; apply(this.files, e.inv); this.undone.push(e); return e; }
  redo() { const e = this.undone.pop(); if (!e) return null; apply(this.files, e.op); this.done.push(e); return e; }
  get canUndo() { return this.done.length > 0; }
  get canRedo() { return this.undone.length > 0; }
}
