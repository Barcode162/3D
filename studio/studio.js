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
  listeners: {},
};
export function on(ev, f) { (S.listeners[ev] = S.listeners[ev] || []).push(f); }
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

window.bar3dStudio = { S, send, on };  // for tests and the console
start();
