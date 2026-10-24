// Dev (PLAN.md R4): login, the five tabs, and each tab's live data poured into
// its screen file's slots (parts with an "id"). A tab is {screen, fill(view)}:
// the screen file is drawn first (its example rows show while data loads),
// then fill() replaces the slots' contents with the real thing.
(function () {
  'use strict';
  var $ = function (id) { return document.getElementById(id); };
  var ACCOUNT = '/bar/account.php';
  var D = window.Dev = { me: null, token: '', screens: {}, tabs: {} };
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

  // ---- Tabs ----
  var ORDER = [['overview', 'Overview', 'home'], ['projects', 'Projects', 'layers'], ['plan', 'Plan', 'rule'], ['publish', 'Publish', 'publish'], ['people', 'People', 'people']];
  ORDER.forEach(function (t) { D.tabs[t[0]] = D.tabs[t[0]] || { screen: 'dev-' + t[0], fill: null }; });
  function screen(id) {
    if (D.screens[id]) return Promise.resolve(D.screens[id]);
    return Promise.all([D.get('/ui/system/screens/' + id + '.json'), Looped.loadParts()]).then(function (a) { D.screens[id] = a[0]; return a[0]; });
  }
  var current = '';
  D.show = function (tab) {
    if (!D.tabs[tab]) tab = 'overview';
    current = tab;
    [].forEach.call(document.querySelectorAll('#tabs button'), function (b) { if (b.dataset.tab === tab) b.setAttribute('aria-current', 'page'); else b.removeAttribute('aria-current'); });
    try { history.replaceState(null, '', '#' + tab); } catch (e) {}
    return screen(D.tabs[tab].screen).then(function (s) {
      if (current !== tab) return;
      Looped.screen(s, $('view'));
      window.scrollTo(0, 0);
      if (D.tabs[tab].fill) return D.tabs[tab].fill($('view'));
    }).catch(function () { $('view').innerHTML = '<p class="dev-wait">This tab did not load. Pull down to try again.</p>'; });
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
  }

  // ---- Login: the bar's account; Dev is for its owner and admins ----
  function showLogin(why) {
    $('view').textContent = ''; $('tabs').hidden = true; $('login').hidden = false;
    if (why) $('loginWhy').textContent = why;
  }
  function start() {
    // Pictures and tests on this computer only: ?demo skips the login (no PHP behind the page).
    if (/^(localhost|127\.)/.test(location.hostname) && /[?&]demo/.test(location.search)) {
      D.me = { user: 'demo', name: 'Demo', admin: 1 }; D.demo = true; $('login').hidden = true; drawTabs(); return D.show((location.hash || '').slice(1));
    }
    D.token = store('hubToken') || '';
    if (!D.token) return showLogin();
    D.post(ACCOUNT, { a: 'me' }).then(function (j) {
      if (!j.ok) return showLogin(j.status === 0 ? 'No connection. Try again in a moment.' : '');
      if (!j.me.admin) return showLogin('Dev is for the owner and admins. You are logged in as @' + j.me.user + '.');
      D.me = j.me; $('login').hidden = true; drawTabs();
      D.show((location.hash || '').slice(1));
    });
  }
  $('login').addEventListener('submit', function (e) {
    e.preventDefault(); $('loginMsg').textContent = 'Logging in…';
    D.post(ACCOUNT, { a: 'login', user: $('user').value.trim(), pass: $('pass').value }).then(function (j) {
      if (!j.ok) { $('loginMsg').textContent = j.error || 'That didn\'t work.'; return; }
      store('hubToken', j.token); $('loginMsg').textContent = ''; start();
    });
  });
  window.addEventListener('hashchange', function () { var t = location.hash.slice(1); if (t && t !== current && D.me) D.show(t); });
  // The tab modules (projects.js, ...) load after this file and register their fill() before start runs.
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start); else setTimeout(start, 0);
})();
