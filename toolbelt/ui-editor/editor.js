// The UI Editor (docs/UI_PLAN.md R1.4-R1.7): the theme and screen files of
// ui/system/, edited live. One state object; every change goes through
// change(), which keeps undo and redraws. The preview is a .lp frame that
// wears the edited theme (its tokens set inline, so edits show at once).
(function () {
  'use strict';
  var SYS = '/ui/system/';
  var $ = function (id) { return document.getElementById(id); };
  var S = {
    mode: 'screen', device: 'phone',
    tokens: null, parts: null, defs: {}, index: null,
    themes: {}, themeId: null,      // id → theme file (edited copies)
    screens: {}, screenId: null,    // id → screen file (edited copies)
    sel: null,                      // selected part's path ('' = root, '0.2' ...) or token id in theme mode
  };

  // ---- Loading ----
  function getJson(path) { return fetch(SYS + path, { cache: 'no-cache' }).then(function (r) { if (!r.ok) throw new Error(path + ': ' + r.status); return r.json(); }); }
  function load() {
    return Promise.all([getJson('tokens.json'), getJson('parts.json'), getJson('themes/index.json'), getJson('screens/index.json')]).then(function (a) {
      S.tokens = a[0]; S.parts = a[1].parts; S.index = a[2];
      S.parts.forEach(function (p) { S.defs[p.type] = p; });
      return Promise.all([
        Promise.all(S.index.order.map(function (id) { return getJson('themes/' + id + '.json'); })),
        Promise.all(a[3].order.map(function (id) { return getJson('screens/' + id + '.json'); })),
        Looped.loadParts(),
      ]);
    }).then(function (b) {
      b[0].forEach(function (t) { S.themes[t.id] = t; });
      b[1].forEach(function (s) { S.screens[s.id] = s; });
      S.themeId = S.index.default; S.screenId = b[1][0].id;
    });
  }

  // ---- Small helpers ----
  function h(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text != null) e.textContent = text; return e; }
  var toastT = 0;
  function toast(text) { var t = $('toast'); t.textContent = text; t.classList.add('show'); clearTimeout(toastT); toastT = setTimeout(function () { t.classList.remove('show'); }, 2000); }
  function theme() { return S.themes[S.themeId]; }
  function screen() { return S.screens[S.screenId]; }
  function partAt(path) {
    var p = screen().root;
    if (path === '' || path == null) return p;
    var bits = String(path).split('.');
    for (var i = 0; i < bits.length; i++) { p = p && p.children && p.children[+bits[i]]; }
    return p || null;
  }
  // A part's one-line summary for the list: its words, else its main option.
  function summary(p) {
    if (p.text) return '"' + p.text + '"';
    if (p.options) return p.options.join(' / ');
    if (p.name) return p.name;
    if (p.children) return p.children.length + ' inside';
    return '';
  }

  // ---- Drawing ----
  function applyTheme() {
    var th = theme(), f = $('frame');
    document.documentElement.setAttribute('data-look', S.index.order.indexOf(th.id) >= 0 ? th.id : (th.look || S.index.default));
    S.tokens.tokens.forEach(function (t) { f.style.setProperty('--' + t.id, th.tokens[t.id]); });
  }
  function drawPreview() {
    var f = $('frame');
    f.className = 'lp ed-frame ed-' + S.device;
    applyTheme();
    Looped.screen(screen(), f, { mark: true });
    markSel();
  }
  function markSel() {
    var f = $('frame'), old = f.querySelectorAll('.ed-sel');
    for (var i = 0; i < old.length; i++) old[i].classList.remove('ed-sel');
    if (S.mode !== 'screen' || S.sel == null) return;
    var el = f.querySelector('[data-path="' + S.sel + '"]');
    if (el) { el.classList.add('ed-sel'); if (el.scrollIntoView && !inView(el)) el.scrollIntoView({ block: 'nearest' }); }
  }
  function inView(el) { var v = $('viewport').getBoundingClientRect(), r = el.getBoundingClientRect(); return r.top >= v.top && r.bottom <= v.bottom; }

  function drawTree() {
    var ol = $('tree'), n = 0;
    ol.textContent = '';
    (function walk(p, path, depth) {
      var li = h('li'), b = h('button'), d = S.defs[p.type];
      b.type = 'button'; b.dataset.path = path; b.style.paddingLeft = (10 + depth * 14) + 'px';
      if (path === S.sel) b.setAttribute('aria-current', 'true');
      b.appendChild(h('span', 't', d ? d.name : p.type)); b.appendChild(h('span', 's', summary(p)));
      li.appendChild(b); ol.appendChild(li); n++;
      (p.children || []).forEach(function (c, i) { walk(c, path === '' ? String(i) : path + '.' + i, depth + 1); });
    })(screen().root, '', 0);
    $('count').textContent = n + ' parts';
  }

  function drawInspector() {
    var box = $('insp'); box.textContent = '';
    var p = S.sel == null ? null : partAt(S.sel);
    if (!p) { $('inspTitle').textContent = 'Nothing selected'; $('inspWhat').textContent = 'Tap a part in the preview or the list.'; return; }
    var d = S.defs[p.type];
    $('inspTitle').textContent = d ? d.name : p.type;
    $('inspWhat').textContent = d ? d.what : 'Unknown part.';
    if (!d) return;
    Object.keys(d.props).forEach(function (k) {
      var row = h('div', 'ed-row'), v = p[k] !== undefined ? p[k] : d.props[k].default;
      row.appendChild(h('span', null, k));
      row.appendChild(h('span', 'ed-val', Array.isArray(v) ? v.join(', ') : String(v)));
      box.appendChild(row);
    });
  }

  function drawPickers() {
    var pick = $('pick'); pick.textContent = '';
    Object.keys(S.screens).forEach(function (id) { var o = h('option', null, S.screens[id].name); o.value = id; pick.appendChild(o); });
    pick.value = S.screenId;
    var tp = $('themePick'); tp.textContent = '';
    Object.keys(S.themes).forEach(function (id) { var o = h('option', null, S.themes[id].name); o.value = id; tp.appendChild(o); });
    tp.value = S.themeId;
  }
  function drawAll() { drawPickers(); drawPreview(); drawTree(); drawInspector(); }

  // ---- Selecting ----
  function select(path) { S.sel = path; markSel(); drawTree(); drawInspector(); }
  // In the preview, a tap selects the part under the finger; nothing in the preview acts on it.
  function previewTap(e) {
    var el = e.target.closest && e.target.closest('[data-path]');
    e.preventDefault(); e.stopPropagation();
    if (el && $('frame').contains(el)) select(el.getAttribute('data-path'));
  }
  ['click', 'pointerdown'].forEach(function (ev) {
    $('frame').addEventListener(ev, function (e) { if (ev === 'click') previewTap(e); else e.stopPropagation(); }, true);
  });
  $('frame').addEventListener('mouseover', function (e) { var el = e.target.closest('[data-path]'); var o = $('frame').querySelector('.ed-hover'); if (o) o.classList.remove('ed-hover'); if (el) el.classList.add('ed-hover'); });
  $('frame').addEventListener('submit', function (e) { e.preventDefault(); }, true);
  $('tree').addEventListener('click', function (e) { var b = e.target.closest('button[data-path]'); if (b) select(b.dataset.path); });

  // ---- Top bar ----
  function pressed(sel, attr, val) { [].forEach.call(document.querySelectorAll(sel), function (b) { b.setAttribute('aria-pressed', String(b.dataset[attr] === val)); }); }
  document.addEventListener('click', function (e) {
    var b = e.target.closest && e.target.closest('[data-device]');
    if (b) { S.device = b.dataset.device; pressed('[data-device]', 'device', S.device); drawPreview(); }
    var m = e.target.closest && e.target.closest('[data-mode]');
    if (m) { S.mode = m.dataset.mode; pressed('[data-mode]', 'mode', S.mode); document.body.dataset.mode = S.mode; drawAll(); }
  });
  $('pick').addEventListener('change', function () { S.screenId = this.value; S.sel = null; drawAll(); });
  $('themePick').addEventListener('change', function () { S.themeId = this.value; drawAll(); });

  load().then(function () {
    document.body.dataset.mode = S.mode;
    drawAll();
    $('status').textContent = Object.keys(S.screens).length + ' screens · ' + Object.keys(S.themes).length + ' themes';
  }).catch(function (e) { $('status').textContent = 'Did not load: ' + e.message; });

  window.UIEditor = { state: S, select: select, partAt: partAt, toast: toast };
})();
