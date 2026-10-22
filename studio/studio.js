// The Studio's page (docs/STUDIO.md 5): logs you in, frames the bar's engine
// in studio mode (/bar/?studio=1, bar/studio_mode.cpp) and draws the
// Studio's controls around it. Talks to the engine with
// bar3dBridge.command('st.*', a, b, text) and hears it through
// window.bar3dStudioSink (the "studio" page operation).
const $ = (id) => document.getElementById(id);
const API = 'api.php';
const local = /^(localhost|127\.0\.0\.1)$/.test(location.hostname);
const store = {
  get: (k) => { try { return localStorage.getItem(k); } catch (e) { return null; } },
  set: (k, v) => { try { localStorage.setItem(k, v); } catch (e) {} },
};

// What the Studio knows (shared by its parts).
export const S = {
  token: store.get('hubToken') || '',
  you: '', role: '', live: { version: 0 },
  engine: null,          // the frame's window, once its bridge is up
  things: [],            // [{i, id, kind, parent, at}] from the engine
  byId: new Map(),
  place: 'bar',
  selected: -1,
  layers: 0,             // X-ray layers shown (monitor.js)
  listeners: {},
};
export function on(ev, f) { (S.listeners[ev] = S.listeners[ev] || []).push(f); }
export function emitEvent(ev, d) { emit(ev, d); }
function emit(ev, d) { (S.listeners[ev] || []).forEach((f) => { try { f(d); } catch (e) { console.error(e); } }); }

export function send(name, a = 0, b = 0, text = '', text2 = '') {
  if (S.engine && S.engine.bar3dBridge) S.engine.bar3dBridge.command(name, a, b, text, text2);
}
export async function api(body) {
  const r = await fetch(API, { method: 'POST', body: JSON.stringify({ ...body, token: S.token }), headers: { 'Content-Type': 'text/plain' } });
  let j = {};
  try { j = await r.json(); } catch (e) {}
  return { status: r.status, ...j };
}
export function setSaved(text) { $('savedText').textContent = text; }

// ---- The engine in its frame ----
function onEngine(e) {
  if (!e || !e.ev) return;
  if (e.ev === 'ready') {
    S.place = e.place;
    S.things = e.things.map((t, i) => ({ i, id: t[0], kind: t[1], parent: t[2], at: [t[3], t[4], t[5]] }));
    S.byId = new Map(S.things.map((t) => [t.id, t]));
    $('placeName').textContent = S.place === 'bar' ? 'Bar' : S.place;
    setSaved(S.live.version ? `Live v${S.live.version}` : 'Built-in world');
    emit('ready', S);
  }
  emit(e.ev, e);
}
function startEngine() {
  const frame = $('world');
  frame.src = '/bar/?studio=1&skipintro&net=off' + (local ? '&server=off' : '');
  // Hook the sink as soon as the frame's page exists (before the engine starts).
  const hook = setInterval(() => {
    const w = frame.contentWindow;
    if (!w || !w.document || w.location.href === 'about:blank') return;
    w.bar3dStudioSink = onEngine;
    if (w.bar3dBridge) { S.engine = w; clearInterval(hook); }
  }, 30);
}

// ---- Login ----
async function who() {
  if (local && /[?&]nologin/.test(location.search)) return { ok: 1, you: 'Test', role: 'owner', live: { version: 0 } };
  if (!S.token) return { status: 401 };
  return api({ a: 'me' });
}
async function start() {
  const me = await who().catch(() => ({ status: 0 }));
  if (!me.ok) return showLogin(me.status === 403 ? 'The Studio is for the bar\'s owner and admins.' : '');
  S.you = me.you; S.role = me.role; S.live = me.live || { version: 0 };
  $('login').hidden = true;
  $('app').hidden = false;
  startEngine();
  emit('start', S);
}
function showLogin(msg) {
  $('app').hidden = true;
  $('login').hidden = false;
  $('loginError').textContent = msg || '';
}
$('loginForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  $('loginError').textContent = 'Logging in…';
  try {
    const r = await fetch('/bar/account.php', { method: 'POST', headers: { 'Content-Type': 'text/plain' },
      body: JSON.stringify({ a: 'login', user: $('user').value.trim(), pass: $('pass').value }) });
    const j = await r.json();
    if (!j.ok) { $('loginError').textContent = j.error || 'That didn\'t work.'; return; }
    S.token = j.token; store.set('hubToken', j.token);
    start();
  } catch (err) { $('loginError').textContent = 'No connection.'; }
});

// ---- The bars ----
$('camModes').addEventListener('click', (e) => {
  const b = e.target.closest('[data-cam]');
  if (!b) return;
  [...$('camModes').children].forEach((x) => x.classList.toggle('on', x === b));
  send('st.cam', +b.dataset.cam);
});

// ---- Selection (S2.3) ----
export function select(i, fly = false) {
  S.selected = i;
  send('st.select', i, fly ? 1 : 0);
  drawCrumb();
  emit('select', i);
}
function pathOf(i) {
  const out = [];
  for (let t = S.things[i]; t; t = S.things[t.parent]) out.unshift(t);
  return out;
}
function drawCrumb() {
  const c = $('crumb');
  const t = S.things[S.selected];
  c.hidden = !t;
  if (!t) return;
  const place = S.place === 'bar' ? 'Bar' : S.place;
  const parts = pathOf(S.selected).map((x) => x.id);
  const last = parts.pop();
  $('crumbPath').innerHTML = '';
  $('crumbPath').append(place + ' › ' + parts.map((p) => p + ' › ').join(''));
  const b = document.createElement('b');
  b.textContent = last;
  $('crumbPath').append(b);
}
let hitsTimer = 0;
on('pick', (e) => {
  if (window.bar3dTools && window.bar3dTools.putOn) return;  // tools.mjs: this tap picks a new parent
  const hits = e.hits || [];
  const box = $('hits');
  box.hidden = true;
  if (!hits.length) { if (!S.locked) select(-1); return; }
  if (!S.locked && !e.twice) select(hits[0][0]);
  if (hits.length > 1 && !e.twice) {
    box.textContent = '';
    const head = document.createElement('span');
    head.className = 'label';
    head.textContent = 'Under your finger';
    box.appendChild(head);
    hits.slice(0, 6).forEach(([i, d]) => {
      const t = S.things[i];
      if (!t) return;
      const b = document.createElement('button');
      b.innerHTML = `<span></span><small>${t.kind} · ${(+d).toFixed(1)} m</small>`;
      b.firstChild.textContent = t.id;
      if (i === S.selected) b.style.color = 'var(--accent)';
      b.onclick = () => { select(i); box.hidden = true; };
      box.appendChild(b);
    });
    box.hidden = false;
    clearTimeout(hitsTimer);
    hitsTimer = setTimeout(() => { box.hidden = true; }, 4000);
  }
});
$('deselect').addEventListener('click', () => select(-1));
$('lockKey').addEventListener('click', () => {
  S.locked = !S.locked;
  $('lockKey').classList.toggle('lit', S.locked);
  Looped.toast(S.locked ? 'Selection locked: move around freely' : 'Selection unlocked');
});

// ---- Undo / redo (S3.4): the keys, and two- or three-finger taps on the world ----
async function undoRedo(which) {
  const d = await import('./draft.mjs');
  const e = which === 'undo' ? d.undo() : d.redo();
  if (e) Looped.toast((which === 'undo' ? 'Undid: ' : 'Redid: ') + (e.op.label || 'a change'));
  else Looped.toast(which === 'undo' ? 'Nothing to undo' : 'Nothing to redo');
}
$('undo').addEventListener('click', () => undoRedo('undo'));
$('redo').addEventListener('click', () => undoRedo('redo'));
on('undo', () => undoRedo('undo'));
on('redo', () => undoRedo('redo'));
on('changed', () => { $('undo').disabled = !S.history.canUndo; $('redo').disabled = !S.history.canRedo; });
on('world', () => { $('undo').disabled = true; $('redo').disabled = true; openFromLink(); });
// A link straight to a thing (the hub's "Open in Studio"): /studio/#thing=<id>.
function openFromLink() {
  const m = location.hash.match(/thing=([^&]+)/);
  const t = m && S.byId && S.byId.get(decodeURIComponent(m[1]));
  if (t) select(t.i, true);
}
window.addEventListener('hashchange', () => { if (S.draft) openFromLink(); });

// ---- Saved views (per place, on this device) ----
const viewsKey = () => 'studioViews-' + S.place;
const loadViews = () => { try { return JSON.parse(store.get(viewsKey()) || '[]'); } catch (e) { return []; } };
function drawViews() {
  const box = $('views');
  box.textContent = '';
  const add = document.createElement('button');
  add.innerHTML = '<span>Save this view</span><small>+</small>';
  add.onclick = () => { S.wantView = true; send('st.getview'); };
  box.appendChild(add);
  loadViews().forEach((v, i) => {
    const b = document.createElement('button');
    b.innerHTML = `<span>${v.name}</span><small>${['orbit', 'fly', 'top', 'walk'][v.view[5]] || ''}</small>`;
    b.onclick = () => { send('st.view', 0, 0, v.view.join(',')); setCamButton(v.view[5]); box.hidden = true; };
    box.appendChild(b);
  });
}
on('view', (e) => {
  if (!S.wantView) return;
  S.wantView = false;
  const views = loadViews();
  views.push({ name: 'View ' + (views.length + 1), view: e.view });
  store.set(viewsKey(), JSON.stringify(views.slice(-12)));
  drawViews();
  Looped.toast('View saved');
});
$('viewsKey').addEventListener('click', () => { const box = $('views'); box.hidden = !box.hidden; if (!box.hidden) drawViews(); });
function setCamButton(mode) { [...$('camModes').children].forEach((x) => x.classList.toggle('on', +x.dataset.cam === +mode)); }

window.bar3dStudio = { S, send, on, select };  // for tests and the console
// The Monitor (monitor.js) imports this module, so it's loaded after it.
Promise.all([import('./draft.mjs'), import('./monitor.js'), import('./tools.mjs')]).then(start, (e) => { console.error(e); start(); });
