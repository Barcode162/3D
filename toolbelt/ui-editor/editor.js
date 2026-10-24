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
    group: 'theme',
    scheme: null,                   // the half of the theme shown: its scheme, or its other's ('dark' | 'light')                 // theme mode: the token group shown ('theme' = name, scheme, copy)
    undo: [], redo: [], lastKey: '', lastAt: 0,
  };
  var FONTS = ['"Archivo", "Helvetica Neue", Arial, sans-serif', '"Limelight", "Didot", Georgia, serif', '"Unbounded", "Archivo", Arial, sans-serif',
    '"Righteous", "Archivo", Arial, sans-serif', '"Martian Mono", ui-monospace, monospace', '-apple-system, BlinkMacSystemFont, "SF Pro Display", "Helvetica Neue", Arial, sans-serif',
    'Georgia, "Times New Roman", serif',
    '-apple-system, BlinkMacSystemFont, "SF Pro Text", "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif',
    'ui-rounded, "SF Pro Rounded", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif'];

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
        fetch('/ui/icons.svg').then(function (r) { return r.text(); }),
      ]);
    }).then(function (b) {
      b[0].forEach(function (t) { S.themes[t.id] = t; });
      S.icons = (b[3].match(/id="i-[a-z-]+"/g) || []).map(function (m) { return m.slice(6, -1); });
      b[1].forEach(function (s) { S.screens[s.id] = s; });
      return liveVersion();
    }).then(function () {
      S.base = JSON.parse(JSON.stringify(S.themes)); S.baseScreens = JSON.parse(JSON.stringify(S.screens));
      S.themeId = S.index.default; S.screenId = Object.keys(S.screens)[0];
      restoreDraft();
    });
  }
  // The site is built from the repo's files; the store may hold a newer
  // published version Claude hasn't pulled yet: start from that one.
  function liveVersion() {
    return Promise.all([getJson('live-version.json').catch(function () { return { version: 0 }; }), apiGet('?live')]).then(function (a) {
      S.repoVersion = a[0].version || 0; S.live = a[1] && a[1].ok ? a[1] : { version: S.repoVersion };
      if (!(S.live.version > S.repoVersion)) return;
      return apiGet('?bundle=' + S.live.version).then(function (b) {
        if (!b || !b.files) return;
        Object.keys(b.files).forEach(function (path) {
          var f = b.files[path], m = /^(themes|screens)\/([a-z0-9-]+)\.json$/.exec(path);
          if (m && m[1] === 'themes') S.themes[f.id] = f; else if (m) S.screens[f.id] = f;
        });
      });
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

  // ---- Light and dark ----
  // A theme's main half is its scheme; theme.other holds the other half's
  // differences. S.scheme says which half the preview shows and edits change.
  function shown() { var th = theme(); return S.scheme === th.scheme || !th.other ? 'main' : 'other'; }
  function tok(id, th) { th = th || theme(); var o = shown() === 'other' && th.other && th.other.tokens; return o && id in o ? o[id] : th.tokens[id]; }
  var STARTER = {
    light: { bg: '#F4F4F2', 'bg-glow': 'none', text: '#1A1A1A', 'text-2': '#5A5A5A', 'text-3': '#8C8C8C', rule: 'rgba(0,0,0,.12)', panel: '#FFFFFF', over: 'rgba(255,255,255,.86)',
      field: '#ECECEA', 'key-bg': '#FFFFFF', 'key-text': '#1A1A1A', 'key-gloss': 'none', off: '#DCDCDA', 'panel-edge': '0 1px 3px rgba(0,0,0,.08), 0 0 0 1px rgba(0,0,0,.06)',
      'field-edge': 'inset 0 0 0 1px rgba(0,0,0,.06)', 'key-shadow': '0 1px 2px rgba(0,0,0,.12), 0 0 0 1px rgba(0,0,0,.08)', 'key-shadow-down': '0 0 0 1px rgba(0,0,0,.1)' },
    dark: { bg: '#0E0E10', 'bg-glow': 'none', text: '#F2F2F4', 'text-2': '#A0A0A8', 'text-3': '#6A6A72', rule: 'rgba(255,255,255,.1)', panel: '#1A1A1E', over: 'rgba(20,20,24,.86)',
      field: '#111114', 'key-bg': '#26262B', 'key-text': '#F2F2F4', 'key-gloss': 'none', off: '#2A2A30', 'panel-edge': '0 0 0 1px rgba(255,255,255,.07)',
      'field-edge': 'inset 0 0 0 1px rgba(255,255,255,.07)', 'key-shadow': '0 0 0 1px rgba(255,255,255,.08)', 'key-shadow-down': 'none' },
  };
  function setScheme(sc) {
    var th = theme();
    if (sc !== th.scheme && !th.other) {
      change('', function () { theme().other = { scheme: sc, tokens: JSON.parse(JSON.stringify(STARTER[sc])) }; S.scheme = sc; });
      toast('A ' + sc + ' half for ' + th.name + ': a plain start, change it in Theme');
      return;
    }
    S.scheme = sc; drawAll();
  }
  document.addEventListener('click', function (e) { var b = e.target.closest && e.target.closest('[data-scheme]'); if (b && b.closest('#schemeSeg')) setScheme(b.dataset.scheme); });

  // ---- Drawing ----
  function applyTheme() {
    var th = theme(), f = $('frame');
    if (S.scheme !== 'dark' && S.scheme !== 'light') S.scheme = th.scheme;
    if (S.scheme !== th.scheme && !th.other) S.scheme = th.scheme;
    document.documentElement.setAttribute('data-look', S.index.order.indexOf(th.id) >= 0 ? th.id : (th.look || S.index.default));
    document.documentElement.setAttribute('data-scheme', S.scheme);
    S.tokens.tokens.forEach(function (t) { f.style.setProperty('--' + t.id, tok(t.id)); });
    f.style.colorScheme = S.scheme;
    pressed('#schemeSeg [data-scheme]', 'scheme', S.scheme);
    [].forEach.call(document.querySelectorAll('#schemeSeg [data-scheme]'), function (b) { b.textContent = (b.dataset.scheme === 'dark' ? 'Dark' : 'Light') + (b.dataset.scheme !== th.scheme && !th.other ? ' +' : ''); });
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
    var cur = ol.querySelector('[aria-current="true"]');
    if (cur && cur.scrollIntoView) { var r = cur.getBoundingClientRect(), v = ol.getBoundingClientRect(); if (r.top < v.top || r.bottom > v.bottom) cur.scrollIntoView({ block: 'nearest' }); }
  }

  function drawInspector() {
    if (S.mode === 'theme') return drawThemeInspector();
    var box = $('insp'); box.textContent = '';
    var p = S.sel == null ? null : partAt(S.sel);
    if (!p) { $('inspTitle').textContent = 'Nothing selected'; $('inspWhat').textContent = 'Tap a part in the preview or the list.'; return; }
    var d = S.defs[p.type];
    $('inspTitle').textContent = d ? d.name : p.type;
    $('inspWhat').textContent = d ? d.what : 'Unknown part.';
    box.appendChild(partActions(p, d));
    if (S.sel === '') screenMeta(box);
    if (!d) return;
    Object.keys(d.props).forEach(function (k) { box.appendChild(propRow(p, k, d.props[k])); });
  }

  // ---- Screen editing ----
  function parentOf(path) { var i = path.lastIndexOf('.'); return i < 0 ? '' : path.slice(0, i); }
  function indexOf(path) { return +path.slice(path.lastIndexOf('.') + 1); }
  function join(parent, i) { return parent === '' ? String(i) : parent + '.' + i; }
  function fresh(type) {
    var d = S.defs[type], p = { type: type };
    if (d.props.text) p.text = d.props.text.default;
    if (d.children) p.children = [];
    return p;
  }
  function setProp(k, v, key) {
    var path = S.sel;
    change('prop:' + path + ':' + k + (key || ''), function () {
      var p = partAt(path), d = S.defs[p.type];
      if (d && JSON.stringify(d.props[k].default) === JSON.stringify(v)) delete p[k]; else p[k] = v;
    }, function () { drawPreview(); drawTree(); });
  }
  function propRow(p, k, def) {
    var v = p[k] !== undefined ? p[k] : def.default, row = h('div', 'ed-row'), lab = h('label', null, k), c;
    if (def.what) lab.title = def.what;
    row.appendChild(lab);
    if (def.kind === 'bool') {
      c = h('input'); c.type = 'checkbox'; c.checked = !!v; c.className = 'ed-check';
      c.addEventListener('change', function () { setProp(k, c.checked); });
    } else if (def.kind === 'choice' || def.kind === 'icon') {
      c = h('select', 'ed-in');
      var opts = def.kind === 'icon' ? [''].concat(S.icons) : def.choices;
      opts.forEach(function (o) { var e = h('option', null, o === '' ? '(none)' : o); e.value = o; c.appendChild(e); });
      c.value = v;
      c.addEventListener('change', function () { setProp(k, c.value); });
    } else if (def.kind === 'number') {
      c = h('input', 'ed-in'); c.type = 'number'; c.value = v; c.step = 'any'; c.inputMode = 'decimal';
      c.addEventListener('input', function () { var n = parseFloat(c.value); if (isFinite(n)) setProp(k, n, ':type'); });
    } else if (def.kind === 'list') {
      c = h('textarea', 'ed-in'); c.value = (v || []).join('\n'); c.rows = Math.max(2, (v || []).length);
      c.addEventListener('change', function () { setProp(k, c.value.split('\n').map(function (x) { return x.trim(); }).filter(Boolean)); });
      row.classList.add('ed-tall');
    } else {
      c = h('input', 'ed-in'); c.value = v; c.spellcheck = false;
      c.addEventListener('input', function () { setProp(k, c.value, ':type'); });
    }
    c.setAttribute('aria-label', k);
    row.appendChild(c);
    if (def.what) row.appendChild(h('div', 'ed-note', def.what));
    return row;
  }
  function actBtn(label, icon, fn, disabled) {
    var b = h('button', 'ed-btn'); b.type = 'button'; b.setAttribute('aria-label', label); b.title = label; b.disabled = !!disabled;
    if (icon) { var ns = 'http://www.w3.org/2000/svg', sv = document.createElementNS(ns, 'svg'), u = document.createElementNS(ns, 'use'); u.setAttribute('href', '/ui/icons.svg#i-' + icon); sv.appendChild(u); b.appendChild(sv); b.classList.add('ed-icon'); }
    else b.textContent = label;
    b.addEventListener('click', fn); return b;
  }
  function partActions(p, d) {
    var bar = h('div', 'ed-actions'), path = S.sel, root = path === '';
    var sib = root ? 0 : partAt(parentOf(path)).children.length, i = root ? 0 : indexOf(path);
    bar.appendChild(actBtn('Move up', 'up', function () { move(-1); }, root || i === 0));
    bar.appendChild(actBtn('Move down', 'down', function () { move(1); }, root || i === sib - 1));
    bar.appendChild(actBtn('Duplicate', 'duplicate', duplicate, root));
    bar.appendChild(actBtn('Delete', 'delete', remove, root));
    var add = h('select', 'ed-in ed-add'); add.setAttribute('aria-label', 'Add a part');
    var first = h('option', null, d && d.children ? '+ Add inside…' : '+ Add after…'); first.value = ''; add.appendChild(first);
    S.parts.forEach(function (x) { var o = h('option', null, x.name); o.value = x.type; add.appendChild(o); });
    add.addEventListener('change', function () { if (add.value) addPart(add.value); });
    bar.appendChild(add);
    return bar;
  }
  function move(dir) {
    var path = S.sel, par = parentOf(path), i = indexOf(path), j = i + dir;
    change('', function () { var list = partAt(par).children, x = list.splice(i, 1)[0]; list.splice(j, 0, x); S.sel = join(par, j); });
  }
  function duplicate() {
    var path = S.sel, par = parentOf(path), i = indexOf(path);
    change('', function () { var list = partAt(par).children; list.splice(i + 1, 0, JSON.parse(JSON.stringify(list[i]))); S.sel = join(par, i + 1); });
    toast('Duplicated');
  }
  function remove() {
    var path = S.sel, par = parentOf(path), i = indexOf(path);
    change('', function () { var list = partAt(par).children; list.splice(i, 1); S.sel = list.length ? join(par, Math.min(i, list.length - 1)) : par; });
    toast('Deleted. Undo brings it back.');
  }
  function addPart(type) {
    var path = S.sel, p = partAt(path), d = S.defs[p.type];
    change('', function () {
      if (d && d.children) { p.children = p.children || []; p.children.push(fresh(type)); S.sel = join(path, p.children.length - 1); }
      else { var par = parentOf(path), i = indexOf(path); partAt(par).children.splice(i + 1, 0, fresh(type)); S.sel = join(par, i + 1); }
    });
    toast('Added a ' + S.defs[type].name.toLowerCase());
  }
  function screenMeta(box) {
    var sc = screen();
    [['name', 'Screen name'], ['about', 'What the screen is for']].forEach(function (f) {
      var row = h('div', 'ed-row'), c = h('input', 'ed-in'); c.value = sc[f[0]] || ''; c.setAttribute('aria-label', f[1]);
      c.addEventListener('change', function () { var v = c.value.trim(); if (f[0] === 'name' && !v) return; change('', function () { screen()[f[0]] = v; }); });
      row.appendChild(h('label', null, 'screen ' + f[0])); row.appendChild(c); box.appendChild(row);
    });
  }
  function newScreen() {
    var n = 1, id; do { id = 'screen-' + n++; } while (S.screens[id]);
    change('', function () {
      S.screens[id] = { format: 1, id: id, name: 'New screen', about: '', root: { type: 'stack', gap: 16, pad: 16, children: [{ type: 'title', text: 'New screen', size: 'l' }] } };
      S.screenId = id; S.sel = '';
    });
    toast('A new screen');
  }

  function drawPickers() {
    var pick = $('pick'); pick.textContent = '';
    Object.keys(S.screens).forEach(function (id) { var o = h('option', null, S.screens[id].name); o.value = id; pick.appendChild(o); });
    var nw = h('option', null, '+ New screen'); nw.value = '+new'; pick.appendChild(nw);
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
    saveDraft();
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
    var half = shown();
    change('tok:' + half + id + (key || ''), function () {
      var th = theme();
      if (half === 'main') th.tokens[id] = v;
      else if (v === th.tokens[id]) delete th.other.tokens[id];
      else th.other.tokens[id] = v;
    }, function () { applyTheme(); drawInspectorRowState(); });
  }
  function drawInspectorRowState() {
    var base = S.base && S.base[S.themeId];
    [].forEach.call(document.querySelectorAll('#insp [data-token]'), function (row) {
      var id = row.dataset.token; row.classList.toggle('ed-changed', !!base && tok(id, base) !== tok(id));
    });
  }
  function tokenRow(t) {
    var v = tok(t.id), row = h('div', 'ed-row'), lab = h('label', null, t.id), box;
    row.dataset.token = t.id; lab.title = t.what; row.appendChild(lab);
    var text = h(t.kind === 'paint' ? 'textarea' : 'input', 'ed-in'); text.value = v; text.setAttribute('aria-label', t.id); text.spellcheck = false;
    if (t.kind === 'paint') text.rows = 2;
    text.addEventListener('change', function () { var nv = text.value.trim(); if (nv && nv !== tok(t.id)) setToken(t.id, nv); });
    if (t.kind === 'color') {
      box = h('div', 'ed-pair'); var sw = h('input', 'ed-swatch'); sw.type = 'color'; var c = parseColor(v); if (c) sw.value = toHex(c); sw.setAttribute('aria-label', t.id + ' colour');
      sw.addEventListener('input', function () { var nv = withPick(tok(t.id), sw.value); text.value = nv; setToken(t.id, nv, ':pick'); });
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
      list.forEach(function (f) { var o = h('option', null, f.split(',')[0].replace(/"/g, '').replace(/^-apple-system$/, 'System (SF)').replace(/^ui-rounded$/, 'System rounded (SF Rounded)')); o.value = f; fs.appendChild(o); }); fs.value = v;
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
    sc.addEventListener('change', function () { change('scheme', function () { theme().scheme = sc.value; S.scheme = sc.value; }); });
    if (th.other) { sc.disabled = true; sc.title = 'This theme has both halves; switch them with Dark / Light above the preview.'; }
    row = h('div', 'ed-row'); row.appendChild(h('label', null, th.other ? 'main half' : 'light or dark')); row.appendChild(sc); box.appendChild(row);
    var acts = h('div', 'ed-actions'), cp = h('button', 'ed-btn', 'Copy as a new theme');
    cp.type = 'button'; cp.addEventListener('click', copyTheme); acts.appendChild(cp);
    var rs = h('button', 'ed-btn', 'Back to as loaded'); rs.type = 'button';
    rs.disabled = !S.base || !S.base[th.id] || JSON.stringify(S.base[th.id]) === JSON.stringify(th);
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
    $('inspWhat').textContent = g ? 'Editing the ' + S.scheme + ' half. Every change shows in the preview at once.' : 'Its name, light or dark, and copies.';
    if (!g) { themeMeta(box); return; }
    S.tokens.tokens.forEach(function (t) { if (t.group === g.id) box.appendChild(tokenRow(t)); });
    drawInspectorRowState();
  }
  $('tree').addEventListener('click', function (e) { var b = e.target.closest('button[data-group]'); if (b) { S.group = b.dataset.group; drawTree(); drawInspector(); } });

  // ---- Draft, login, publish ----
  var API = 'api.php', DRAFT = 'uiEditorDraft';
  function store(k, v) { try { if (v === undefined) return localStorage.getItem(k); if (v === null) localStorage.removeItem(k); else localStorage.setItem(k, v); } catch (e) { return null; } }
  function apiGet(q) { return fetch(API + q, { cache: 'no-store' }).then(function (r) { return r.json(); }).catch(function () { return null; }); }
  function apiPost(body) {
    body.token = store('hubToken') || '';
    return fetch(API, { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify(body) })
      .then(function (r) { return r.json().then(function (j) { j.status = r.status; return j; }); })
      .catch(function () { return { ok: 0, status: 0, error: 'No connection to the store.' }; });
  }
  // What differs from the published version: one line per file.
  function changes() {
    var out = [];
    Object.keys(S.themes).forEach(function (id) { if (JSON.stringify(S.themes[id]) !== JSON.stringify(S.base[id])) out.push((S.base[id] ? 'Theme ' : 'New theme ') + S.themes[id].name); });
    Object.keys(S.screens).forEach(function (id) { if (JSON.stringify(S.screens[id]) !== JSON.stringify(S.baseScreens[id])) out.push((S.baseScreens[id] ? 'Screen ' : 'New screen ') + S.screens[id].name); });
    return out;
  }
  function saveDraft() {
    var n = changes().length;
    if (!n) store(DRAFT, null);
    else store(DRAFT, JSON.stringify({ base: S.live.version, at: Date.now(), themes: S.themes, screens: S.screens }));
    status();
  }
  function restoreDraft() {
    var d = null; try { d = JSON.parse(store(DRAFT) || 'null'); } catch (e) {}
    if (!d || !d.themes || !d.screens) return;
    S.themes = d.themes; S.screens = d.screens;
    toast(d.base === S.live.version ? 'Your draft is back.' : 'Your draft is back (someone published since you started it).');
  }
  function status() {
    var n = changes().length;
    $('status').textContent = (n ? n + ' unpublished · ' : '') + 'v' + S.live.version + ' live';
    $('pubOpen').textContent = n ? 'Publish ' + n : 'Publish';
  }
  function openPublish() {
    $('pub').hidden = false; $('pubScrim').hidden = false; $('pubMsg').textContent = '';
    var list = changes(), ul = $('pubChanges'); ul.textContent = '';
    list.forEach(function (c) { ul.appendChild(h('li', null, c)); });
    $('pubWhat').textContent = list.length ? 'These go live as version ' + (S.live.version + 1) + '. Every version is kept.' : 'Nothing changed since version ' + S.live.version + '.';
    $('pubGo').disabled = !list.length; $('discard').disabled = !list.length; $('discard').textContent = 'Discard my draft';
    apiPost({ a: 'me' }).then(function (j) {
      var out = j.status === 401 || j.status === 403;
      $('login').hidden = !out; $('pubForm').hidden = out;
      if (j.status === 403) $('loginMsg').textContent = j.error;
      if (j.ok) history();
    });
  }
  function closePublish() { $('pub').hidden = true; $('pubScrim').hidden = true; }
  function history() {
    apiPost({ a: 'history' }).then(function (j) {
      var ol = $('history'); ol.textContent = '';
      if (!j.ok) { ol.appendChild(h('li', 'ed-dim', j.error || 'Not available.')); return; }
      if (!j.versions.length) ol.appendChild(h('li', 'ed-dim', 'Nothing published yet.'));
      j.versions.forEach(function (v) {
        var li = h('li'); li.appendChild(h('b', null, 'v' + v.version + ' '));
        li.appendChild(document.createTextNode(v.author + ' · ' + new Date(v.at * 1000).toLocaleString() + (v.note ? ' · ' + v.note : '')));
        ol.appendChild(li);
      });
    });
  }
  function publish() {
    var files = {};
    Object.keys(S.themes).forEach(function (id) { files['themes/' + id + '.json'] = S.themes[id]; });
    Object.keys(S.screens).forEach(function (id) { files['screens/' + id + '.json'] = S.screens[id]; });
    $('pubGo').disabled = true; $('pubMsg').textContent = 'Publishing…';
    apiPost({ a: 'publish', base: S.live.version, note: $('pubNote').value.trim(), bundle: { format: 1, files: files } }).then(function (j) {
      if (!j.ok) { $('pubMsg').textContent = j.error || 'That didn\'t work.'; $('pubGo').disabled = false; return; }
      S.live = j.live; S.base = JSON.parse(JSON.stringify(S.themes)); S.baseScreens = JSON.parse(JSON.stringify(S.screens));
      store(DRAFT, null); $('pubNote').value = ''; status(); closePublish(); drawInspector();
      toast('Published v' + j.live.version + '. Claude brings it into the site at the next go.');
    });
  }
  $('pubOpen').addEventListener('click', openPublish);
  $('pubClose').addEventListener('click', closePublish);
  $('pubScrim').addEventListener('click', closePublish);
  $('pubGo').addEventListener('click', publish);
  $('discard').addEventListener('click', function () {
    var b = $('discard');
    if (b.textContent !== 'Tap again to discard') { b.textContent = 'Tap again to discard'; return; }
    change('', function () { S.themes = JSON.parse(JSON.stringify(S.base)); S.screens = JSON.parse(JSON.stringify(S.baseScreens)); if (!S.themes[S.themeId]) S.themeId = S.index.default; if (!S.screens[S.screenId]) S.screenId = Object.keys(S.screens)[0]; S.sel = null; });
    closePublish(); toast('Draft discarded. Undo brings it back.');
  });
  $('login').addEventListener('submit', function (e) {
    e.preventDefault(); $('loginMsg').textContent = 'Logging in…';
    fetch('/bar/account.php', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify({ a: 'login', user: $('user').value.trim(), pass: $('pass').value }) })
      .then(function (r) { return r.json(); }).then(function (j) {
        if (!j.ok) { $('loginMsg').textContent = j.error || 'That didn\'t work.'; return; }
        store('hubToken', j.token); openPublish();
      }).catch(function () { $('loginMsg').textContent = 'No connection.'; });
  });

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
  $('pick').addEventListener('change', function () {
    if (this.value === '+new') { newScreen(); return; }
    S.screenId = this.value; S.sel = null; drawAll();
  });
  $('themePick').addEventListener('change', function () { S.themeId = this.value; S.scheme = null; drawAll(); });

  // Deep links: ?mode=theme&group=shape&theme=jukebox&screen=sampler&device=computer&sel=1.0.1.0
  function fromAddress() {
    var q = {}; location.search.replace(/[?&]([a-z]+)=([a-z0-9.-]+)/g, function (_, k, v) { q[k] = v; });
    if (q.mode === 'theme' || q.mode === 'screen') S.mode = q.mode;
    if (q.group) S.group = q.group;
    if (q.theme && S.themes[q.theme]) S.themeId = q.theme;
    if (q.scheme === 'light' || q.scheme === 'dark') S.scheme = q.scheme;
    if (q.screen && S.screens[q.screen]) S.screenId = q.screen;
    if (/^(phone|computer|game)$/.test(q.device || '')) S.device = q.device;
    if (q.sel && partAt(q.sel)) S.sel = q.sel;
    pressed('[data-mode]', 'mode', S.mode); pressed('[data-device]', 'device', S.device);
  }
  load().then(function () {
    fromAddress();
    document.body.dataset.mode = S.mode;
    drawAll();
    status();
  }).catch(function (e) { $('status').textContent = 'Did not load: ' + e.message; });

  window.UIEditor = { state: S, select: select, partAt: partAt, toast: toast, change: change, undo: undo, redo: redo, drawAll: drawAll };
})();
