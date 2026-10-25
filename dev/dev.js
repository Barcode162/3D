// Dev (PLAN.md R4, J90): login, the five tabs, the nested pages under them
// and the trail at the top. A tab is {screen, fill(view)}: the screen file is
// drawn first (its example rows show while data loads), then fill() replaces
// the slots' contents with the real thing. A nested page (J90: a project, a
// chapter, a job…) is {title, parent, draw(view, arg)}, registered by the tab
// modules in D.pages; its address is the tab and its path: #plan/engine,
// #plan/J85, #projects/bar3d. Going deeper slides the page in from the right,
// going back from the left (dev.css; the View Transitions API where Safari
// has it).
(function () {
  'use strict';
  var $ = function (id) { return document.getElementById(id); };
  var ACCOUNT = '/bar/account.php';
  var D = window.Dev = { me: null, token: '', screens: {}, tabs: {}, pages: {} };
  function store(k, v) { try { if (v === undefined) return localStorage.getItem(k); if (v === null) localStorage.removeItem(k); else localStorage.setItem(k, v); } catch (e) { return null; } }
  D.store = store;

  // ---- Talking to the servers ----
  D.post = function (url, body) {
    body = Object.assign({ token: D.token }, body);
    return fetch(url, { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify(body) })
      .then(function (r) { return r.json().then(function (j) { j.status = r.status; return j; }); })
      .catch(function () { return { ok: 0, status: 0, error: 'No connection.' }; });
  };
  D.get = function (url) { return fetch(url, { cache: 'no-store' }).then(function (r) { if (!r.ok) throw new Error(r.status); return r.json(); }); };
  D.text = function (url) { return fetch(url, { cache: 'no-store' }).then(function (r) { if (!r.ok) throw new Error(r.status); return r.text(); }); };

  // ---- Slots ----
  // The slot with this id in the current view, its example children cleared.
  D.slot = function (id, keep) { var el = $('view').querySelector('[data-id="' + id + '"]'); if (el && !keep) el.textContent = ''; return el; };
  D.part = function (p) { return Looped.part(p); };
  D.fill = function (id, parts) { var el = D.slot(id); if (el) parts.forEach(function (p) { el.appendChild(D.part(p)); }); return el; };
  D.toast = function (t) { Looped.toast(t); };
  // A row (or any element) that opens a place in Dev when tapped.
  D.link = function (el, path) {
    var a = document.createElement('a'); a.className = 'dev-row-btn'; a.href = '#' + path; a.dataset.go = path;
    a.appendChild(el); return a;
  };
  D.go = function (path) { if (('#' + path) === location.hash) route(); else location.hash = path; };

  // ---- Tabs and pages ----
  var ORDER = [['overview', 'Overview', 'home'], ['projects', 'Projects', 'layers'], ['plan', 'Plan', 'rule'], ['publish', 'Publish', 'publish'], ['people', 'People', 'people']];
  D.TAB_NAMES = {};
  ORDER.forEach(function (t) { D.tabs[t[0]] = D.tabs[t[0]] || { screen: 'dev-' + t[0], fill: null }; D.TAB_NAMES[t[0]] = t[1]; });
  function screen(id) {
    if (D.screens[id]) return Promise.resolve(D.screens[id]);
    return Promise.all([D.get('/ui/system/screens/' + id + '.json'), Looped.loadParts()]).then(function (a) { D.screens[id] = a[0]; return a[0]; });
  }
  // "plan/J85" → {tab: 'plan', rest: 'J85', depth: 1}; "" or anything unknown → the Overview.
  function parse(hash) {
    var path = String(hash || '').replace(/^#\/?/, '');
    var parts = path.split('/'), tab = parts[0];
    if (!D.tabs[tab]) return { tab: 'overview', rest: '', path: 'overview', depth: 0 };
    var rest = parts.slice(1).join('/');
    return { tab: tab, rest: rest, path: rest ? tab + '/' + rest : tab, depth: rest ? rest.split('/').length : 0 };
  }
  D.where = function () { return parse(location.hash); };

  // The trail at the top: Dev › Plan › The engine › J85 (each a way back), and Back.
  D.setTrail = function (crumbs) {
    var t = $('trail'); if (!t) return;
    t.textContent = '';
    crumbs.forEach(function (c, i) {
      if (i) { var s = document.createElement('span'); s.className = 'dev-sep'; s.textContent = '›'; s.setAttribute('aria-hidden', 'true'); t.appendChild(s); }
      var last = i === crumbs.length - 1, b = document.createElement(last ? 'span' : 'a');
      b.className = 'dev-crumb' + (last ? ' here' : ''); b.textContent = c[0];
      if (!last) { b.href = '#' + c[1]; b.dataset.go = c[1]; } else b.setAttribute('aria-current', 'page');
      t.appendChild(b);
    });
    t.scrollLeft = t.scrollWidth;  // the deepest crumb in view
    $('back').hidden = crumbs.length < 3;  // Dev › Tab: nothing to go back to
    $('back').dataset.go = crumbs.length >= 3 ? crumbs[crumbs.length - 2][1] : '';
  };

  var current = '', currentDepth = 0, drawing = 0;
  // Draws the place the address names. `dir`: 'push' (deeper), 'pop' (back), 'swap' (another tab).
  function draw(w) {
    var my = ++drawing;
    [].forEach.call(document.querySelectorAll('#tabs button'), function (b) { if (b.dataset.tab === w.tab) b.setAttribute('aria-current', 'page'); else b.removeAttribute('aria-current'); });
    var page = w.rest ? pageFor(w) : null;
    if (w.rest && !page) { w = parse(w.tab); }  // an address that no longer leads anywhere: its tab
    if (page) {
      var view = $('view'); view.textContent = '';
      var root = D.part({ type: 'stack', gap: 14, pad: 16 }); view.appendChild(root);
      D.setTrail(page.trail);
      return Promise.resolve(page.draw(root, page.arg)).then(function () { if (my === drawing) D.afterDraw(view); });
    }
    D.setTrail([['Dev', 'overview'], [D.TAB_NAMES[w.tab], w.tab]]);
    return screen(D.tabs[w.tab].screen).then(function (s) {
      if (my !== drawing) return;
      Looped.screen(s, $('view'));
      var r = D.tabs[w.tab].fill ? D.tabs[w.tab].fill($('view')) : null;
      D.afterDraw($('view'));
      return r;
    }).catch(function () { $('view').innerHTML = '<p class="dev-wait">This page did not load. Pull down to try again.</p>'; });
  }
  // A nested page for {tab, rest}: the first registered page of that tab whose
  // pattern matches (D.pages[tab] = [{match: /re/, title(m), parent(m), draw(view, m)}]).
  function pageFor(w) {
    var list = D.pages[w.tab] || [];
    for (var i = 0; i < list.length; i++) {
      var m = list[i].match.exec(w.rest);
      if (!m) continue;
      var p = list[i], trail = [['Dev', 'overview'], [D.TAB_NAMES[w.tab], w.tab]];
      (p.crumbs ? p.crumbs(m) : []).forEach(function (c) { trail.push(c); });
      trail.push([p.title(m), w.path]);
      return { draw: p.draw, arg: m, trail: trail };
    }
    return null;
  }
  // After every draw: motion (dev/motion.js) staggers the new page in.
  D.afterDraw = function (view) { if (D.motion) D.motion.enter(view); };

  function route() {
    if (!D.me) return;
    var w = parse(location.hash);
    var dir = w.tab !== current.split('/')[0] ? 'swap' : w.depth > currentDepth ? 'push' : w.depth < currentDepth ? 'pop' : 'swap';
    if (w.path === current && dir === 'swap' && $('view').childNodes.length) dir = 'swap';
    current = w.path; currentDepth = w.depth;
    if (D.onLeave) D.onLeave();
    var run = function () { var r = draw(w); window.scrollTo(0, 0); return r; };
    if (D.motion) D.motion.transition(dir, run); else run();
  }
  D.route = route;
  // A tab by its name (the tab bar, the old addresses like #people).
  D.show = function (tab) {
    if (!D.tabs[tab]) tab = 'overview';
    try { history.replaceState(null, '', '#' + tab); } catch (e) {}
    route();
  };
  function drawTabs() {
    var nav = $('tabs'); nav.textContent = '';
    ORDER.forEach(function (t) {
      var b = document.createElement('button'); b.type = 'button'; b.dataset.tab = t[0];
      var ns = 'http://www.w3.org/2000/svg', sv = document.createElementNS(ns, 'svg'), u = document.createElementNS(ns, 'use');
      sv.setAttribute('class', 'ico'); sv.setAttribute('aria-hidden', 'true'); u.setAttribute('href', '/ui/icons.svg#i-' + t[2]); sv.appendChild(u);
      b.appendChild(sv); b.appendChild(document.createTextNode(t[1]));
      b.addEventListener('click', function () { D.show(t[0]); });
      nav.appendChild(b);
    });
    nav.hidden = false;
    $('bar').hidden = false;
  }
  // Links inside Dev (data-go) move without reloading; Back goes up one level.
  document.addEventListener('click', function (e) {
    var a = e.target.closest && e.target.closest('[data-go]');
    if (!a || !a.dataset.go) return;
    e.preventDefault();
    D.go(a.dataset.go);
  });

  // ---- Login: the bar's account; Dev is for its owner and admins ----
  function showLogin(why) {
    $('view').textContent = ''; $('tabs').hidden = true; $('bar').hidden = true; $('login').hidden = false;
    if (why) $('loginWhy').textContent = why;
  }
  function start() {
    // Pictures and tests on this computer only: ?demo skips the login (no PHP behind the page).
    if (/^(localhost|127\.)/.test(location.hostname) && /[?&]demo/.test(location.search)) {
      D.me = { user: 'demo', name: 'Demo', admin: 1 }; D.demo = true; $('login').hidden = true; drawTabs(); return route();
    }
    D.token = store('hubToken') || '';
    if (!D.token) return showLogin();
    D.post(ACCOUNT, { a: 'me' }).then(function (j) {
      if (!j.ok) return showLogin(j.status === 0 ? 'No connection. Try again in a moment.' : '');
      if (!j.me.admin) return showLogin('Dev is for the owner and admins. You are logged in as @' + j.me.user + '.');
      D.me = j.me; $('login').hidden = true; drawTabs();
      route();
    });
  }
  $('login').addEventListener('submit', function (e) {
    e.preventDefault(); $('loginMsg').textContent = 'Logging in…';
    D.post(ACCOUNT, { a: 'login', user: $('user').value.trim(), pass: $('pass').value }).then(function (j) {
      if (!j.ok) { $('loginMsg').textContent = j.error || 'That didn\'t work.'; return; }
      store('hubToken', j.token); $('loginMsg').textContent = ''; start();
    });
  });
  window.addEventListener('hashchange', function () { if (D.me) route(); });
  // The tab modules (projects.js, ...) load after this file and register their fill() before start runs.
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start); else setTimeout(start, 0);
})();
