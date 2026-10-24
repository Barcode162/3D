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
    sel: null,                      // selected part's path ('' = root, '0.2' ...)
    group: 'theme',                 // theme mode: the token group shown ('theme' = name, scheme, copy)
    undo: [], redo: [], lastKey: '', lastAt: 0,
  };
  var FONTS = ['"Archivo", "Helvetica Neue", Arial, sans-serif', '"Limelight", "Didot", Georgia, serif', '"Unbounded", "Archivo", Arial, sans-serif',
    '"Righteous", "Archivo", Arial, sans-serif', '"Martian Mono", ui-monospace, monospace', '-apple-system, BlinkMacSystemFont, "SF Pro Display", "Helvetica Neue", Arial, sans-serif',
    'Georgia, "Times New Roman", serif'];

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
      S.base = JSON.parse(JSON.stringify(S.themes));
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
    if (S.mode === 'theme') return drawGroups();
    $('treeTitle').textContent = 'Parts';
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
    if (S.mode === 'theme') return drawThemeInspector();
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

  // ---- Changes and undo ----
  // Every edit goes through change(): a snapshot for undo first. Edits with the
  // same key in quick succession (a slider being dragged) share one undo step.
  function snap() { return JSON.stringify({ themes: S.themes, screens: S.screens, themeId: S.themeId, screenId: S.screenId, sel: S.sel }); }
  function restore(js) { var o = JSON.parse(js); S.themes = o.themes; S.screens = o.screens; S.themeId = o.themeId; S.screenId = o.screenId; S.sel = o.sel; }
  function change(key, fn, redraw) {
    var now = Date.now();
    if (!(key && key === S.lastKey && now - S.lastAt < 900)) { S.undo.push(snap()); if (S.undo.length > 200) S.undo.shift(); }
    S.lastKey = key || ''; S.lastAt = now; S.redo = [];
    fn();
    edited();
    (redraw || drawAll)();
  }
  function edited() {
    $('undo').disabled = !S.undo.length; $('redo').disabled = !S.redo.length;
    if (window.UIEditor && UIEditor.onEdit) UIEditor.onEdit();
  }
  function undo() { if (!S.undo.length) return; S.redo.push(snap()); restore(S.undo.pop()); S.lastKey = ''; edited(); drawAll(); toast('Undone'); }
  function redo() { if (!S.redo.length) return; S.undo.push(snap()); restore(S.redo.pop()); S.lastKey = ''; edited(); drawAll(); toast('Redone'); }
  $('undo').addEventListener('click', undo);
  $('redo').addEventListener('click', redo);
  document.addEventListener('keydown', function (e) {
    if (!(e.metaKey || e.ctrlKey) || e.key.toLowerCase() !== 'z' || /INPUT|TEXTAREA|SELECT/.test(e.target.tagName)) return;
    e.preventDefault(); if (e.shiftKey) redo(); else undo();
  });

  // ---- Theme editing ----
  // Colours: a swatch (the browser's picker) plus the exact text. A colour
  // with see-through (rgba or #RRGGBBAA) keeps its see-through when picked.
  function parseColor(v) {
    var m = /^#([0-9a-f]{3,8})$/i.exec(v), r, g, b, a = 1;
    if (m) {
      var x = m[1]; if (x.length <= 4) x = x.split('').map(function (c) { return c + c; }).join('');
      r = parseInt(x.slice(0, 2), 16); g = parseInt(x.slice(2, 4), 16); b = parseInt(x.slice(4, 6), 16); if (x.length === 8) a = parseInt(x.slice(6, 8), 16) / 255;
    } else if ((m = /^rgba?\(([^)]*)\)$/.exec(v))) {
      var p = m[1].split(',').map(function (s) { return parseFloat(s); }); r = p[0]; g = p[1]; b = p[2]; if (p.length > 3) a = p[3];
    } else return null;
    return { r: r, g: g, b: b, a: a };
  }
  function hex2(n) { return ('0' + Math.round(n).toString(16)).slice(-2); }
  function toHex(c) { return '#' + hex2(c.r) + hex2(c.g) + hex2(c.b); }
  function withPick(old, picked) {
    var c = parseColor(old), n = parseColor(picked);
    if (!c || c.a >= 1) return picked.toUpperCase();
    return 'rgba(' + n.r + ',' + n.g + ',' + n.b + ',' + (+c.a.toFixed(3)) + ')';
  }
  function setToken(id, v, key) {
    change('tok:' + id + (key || ''), function () { theme().tokens[id] = v; }, function () { applyTheme(); drawInspectorRowState(); });
  }
  function drawInspectorRowState() {
    var base = S.base[S.themeId];
    [].forEach.call(document.querySelectorAll('#insp [data-token]'), function (row) {
      var id = row.dataset.token; row.classList.toggle('ed-changed', !!base && base.tokens[id] !== theme().tokens[id]);
    });
  }
  function tokenRow(t) {
    var v = theme().tokens[t.id], row = h('div', 'ed-row'), lab = h('label', null, t.id), box;
    row.dataset.token = t.id; lab.title = t.what; row.appendChild(lab);
    var text = h(t.kind === 'paint' ? 'textarea' : 'input', 'ed-in'); text.value = v; text.setAttribute('aria-label', t.id); text.spellcheck = false;
    if (t.kind === 'paint') text.rows = 2;
    text.addEventListener('change', function () { var nv = text.value.trim(); if (nv && nv !== theme().tokens[t.id]) setToken(t.id, nv); });
    if (t.kind === 'color') {
      box = h('div', 'ed-pair'); var sw = h('input', 'ed-swatch'); sw.type = 'color'; var c = parseColor(v); if (c) sw.value = toHex(c); sw.setAttribute('aria-label', t.id + ' colour');
      sw.addEventListener('input', function () { var nv = withPick(theme().tokens[t.id], sw.value); text.value = nv; setToken(t.id, nv, ':pick'); });
      box.appendChild(sw); box.appendChild(text);
    } else if ((t.kind === 'length' || t.kind === 'number') && t.min !== undefined) {
      box = h('div', 'ed-pair ed-wide'); var rg = h('input', 'ed-range'); rg.type = 'range'; rg.min = t.min; rg.max = t.max; rg.step = t.step; rg.setAttribute('aria-label', t.id);
      var unit = (/[a-z%]+$/.exec(v) || [''])[0]; rg.value = parseFloat(v);
      rg.addEventListener('input', function () { var nv = (+(+rg.value).toFixed(3)) + unit; text.value = nv; setToken(t.id, nv, ':drag'); });
      text.addEventListener('change', function () { rg.value = parseFloat(text.value); });
      box.appendChild(rg); box.appendChild(text);
    } else if (t.kind === 'font') {
      box = h('div', 'ed-stackin'); var fs = h('select', 'ed-in'); fs.setAttribute('aria-label', t.id + ' font');
      var list = FONTS.indexOf(v) >= 0 ? FONTS : [v].concat(FONTS);
      list.forEach(function (f) { var o = h('option', null, f.split(',')[0].replace(/"/g, '')); o.value = f; fs.appendChild(o); }); fs.value = v;
      fs.addEventListener('change', function () { text.value = fs.value; setToken(t.id, fs.value); });
      box.appendChild(fs);
    } else if (t.kind === 'choice') {
      box = h('select', 'ed-in'); box.setAttribute('aria-label', t.id);
      t.choices.forEach(function (c) { var o = h('option', null, c); o.value = c; box.appendChild(o); }); box.value = v;
      box.addEventListener('change', function () { setToken(t.id, box.value); });
    } else { box = text; if (t.kind === 'paint') row.classList.add('ed-tall'); }
    row.appendChild(box);
    var note = h('div', 'ed-note', t.what); row.appendChild(note);
    return row;
  }
  function themeMeta(box) {
    var th = theme();
    var nm = h('input', 'ed-in'); nm.value = th.name; nm.setAttribute('aria-label', 'Theme name');
    nm.addEventListener('change', function () { var v = nm.value.trim(); if (v) change('name', function () { theme().name = v; }); });
    var row = h('div', 'ed-row'); row.appendChild(h('label', null, 'name')); row.appendChild(nm); box.appendChild(row);
    var ab = h('input', 'ed-in'); ab.value = th.about; ab.setAttribute('aria-label', 'About the theme');
    ab.addEventListener('change', function () { change('about', function () { theme().about = ab.value.trim(); }); });
    row = h('div', 'ed-row'); row.appendChild(h('label', null, 'about')); row.appendChild(ab); box.appendChild(row);
    var sc = h('select', 'ed-in'); ['dark', 'light'].forEach(function (k) { var o = h('option', null, k); o.value = k; sc.appendChild(o); }); sc.value = th.scheme; sc.setAttribute('aria-label', 'Light or dark');
    sc.addEventListener('change', function () { change('scheme', function () { theme().scheme = sc.value; }); });
    row = h('div', 'ed-row'); row.appendChild(h('label', null, 'light or dark')); row.appendChild(sc); box.appendChild(row);
    var acts = h('div', 'ed-actions'), cp = h('button', 'ed-btn', 'Copy as a new theme');
    cp.type = 'button'; cp.addEventListener('click', copyTheme); acts.appendChild(cp);
    var rs = h('button', 'ed-btn', 'Back to as loaded'); rs.type = 'button';
    rs.disabled = !S.base[th.id] || JSON.stringify(S.base[th.id]) === JSON.stringify(th);
    rs.addEventListener('click', function () { change('', function () { S.themes[th.id] = JSON.parse(JSON.stringify(S.base[th.id])); }); });
    if (S.base[th.id]) acts.appendChild(rs);
    box.appendChild(acts);
  }
  function copyTheme() {
    var th = theme(), n = 2, id;
    do { id = th.id.replace(/-\d+$/, '') + '-' + n++; } while (S.themes[id]);
    change('', function () {
      var c = JSON.parse(JSON.stringify(th)); c.id = id; c.name = th.name + ' ' + (n - 1); c.look = S.index.order.indexOf(th.id) >= 0 ? th.id : (th.look || S.index.default);
      S.themes[id] = c; S.themeId = id;
    });
    toast('Copied as "' + S.themes[id].name + '"');
  }
  function drawGroups() {
    var ol = $('tree'); ol.textContent = '';
    $('treeTitle').textContent = 'Look';
    var groups = [{ id: 'theme', name: 'This theme' }].concat(S.tokens.groups);
    groups.forEach(function (g) {
      var li = h('li'), b = h('button'); b.type = 'button'; b.dataset.group = g.id;
      if (g.id === S.group) b.setAttribute('aria-current', 'true');
      var n = g.id === 'theme' ? '' : S.tokens.tokens.filter(function (t) { return t.group === g.id; }).length + ' tokens';
      b.appendChild(h('span', 't', g.name)); b.appendChild(h('span', 's', n)); li.appendChild(b); ol.appendChild(li);
    });
    $('count').textContent = S.tokens.tokens.length + ' tokens';
  }
  function drawThemeInspector() {
    var box = $('insp'); box.textContent = '';
    var g = S.tokens.groups.filter(function (x) { return x.id === S.group; })[0];
    $('inspTitle').textContent = g ? g.name : theme().name;
    $('inspWhat').textContent = g ? 'Every change shows in the preview at once.' : 'Its name, light or dark, and copies.';
    if (!g) { themeMeta(box); return; }
    S.tokens.tokens.forEach(function (t) { if (t.group === g.id) box.appendChild(tokenRow(t)); });
    drawInspectorRowState();
  }
  $('tree').addEventListener('click', function (e) { var b = e.target.closest('button[data-group]'); if (b) { S.group = b.dataset.group; drawTree(); drawInspector(); } });

  // ---- Selecting ----
  function select(path) { S.sel = path; markSel(); drawTree(); drawInspector(); }
  // In the preview, a tap selects the part under the finger; nothing in the preview acts on it.
  function previewTap(e) {
    var el = e.target.closest && e.target.closest('[data-path]');
    e.preventDefault(); e.stopPropagation();
    if (S.mode !== 'screen') return;
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

  // Deep links: ?mode=theme&group=shape&theme=jukebox&screen=sampler&device=computer
  function fromAddress() {
    var q = {}; location.search.replace(/[?&]([a-z]+)=([a-z0-9-]+)/g, function (_, k, v) { q[k] = v; });
    if (q.mode === 'theme' || q.mode === 'screen') S.mode = q.mode;
    if (q.group) S.group = q.group;
    if (q.theme && S.themes[q.theme]) S.themeId = q.theme;
    if (q.screen && S.screens[q.screen]) S.screenId = q.screen;
    if (/^(phone|computer|game)$/.test(q.device || '')) S.device = q.device;
    pressed('[data-mode]', 'mode', S.mode); pressed('[data-device]', 'device', S.device);
  }
  load().then(function () {
    fromAddress();
    document.body.dataset.mode = S.mode;
    drawAll();
    $('status').textContent = Object.keys(S.screens).length + ' screens · ' + Object.keys(S.themes).length + ' themes';
  }).catch(function (e) { $('status').textContent = 'Did not load: ' + e.message; });

  window.UIEditor = { state: S, select: select, partAt: partAt, toast: toast, change: change, undo: undo, redo: redo, drawAll: drawAll };
})();
