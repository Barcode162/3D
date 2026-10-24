// Looped screens (docs/UI_PLAN.md R1.3): a screen file (ui/system/screens/)
// is a tree of parts; this draws one with looped.css, in the theme in use.
// Load after looped.js. Exposes:
//   Looped.parts                       the part list (ui/system/parts.json), once loaded
//   Looped.loadParts()                 → Promise of the part list
//   Looped.screen(screen, el, opts)    draws screen.root into el (replacing what's there)
//     opts.mark: true                  each drawn part gets data-path="0.2.1" (its place in the tree), for the editor
// A part: {"type":"key","text":"Publish","style":"go"}; containers have "children": [...].
// Unknown types draw as a dashed box saying so, never an error.
(function () {
  'use strict';
  var L = window.Looped = window.Looped || {};
  var BASE = (document.currentScript && document.currentScript.src.replace(/screen\.js(\?.*)?$/, '')) || '/ui/';
  var defs = {};
  L.loadParts = function () {
    if (L._parts) return L._parts;
    L._parts = fetch(BASE + 'system/parts.json', { cache: 'no-cache' }).then(function (r) { return r.json(); }).then(function (d) {
      L.parts = d.parts; d.parts.forEach(function (p) { defs[p.type] = p; }); return d.parts;
    });
    return L._parts;
  };
  function prop(part, k) {
    if (part[k] !== undefined) return part[k];
    var d = defs[part.type]; return d && d.props[k] ? d.props[k].default : undefined;
  }
  function h(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text != null && text !== '') e.textContent = text; return e; }
  function icon(name, size) {
    var ns = 'http://www.w3.org/2000/svg', s = document.createElementNS(ns, 'svg'), u = document.createElementNS(ns, 'use');
    s.setAttribute('class', 'ico'); s.setAttribute('aria-hidden', 'true');
    if (size) { s.style.width = size + 'px'; s.style.height = size + 'px'; }
    u.setAttribute('href', BASE + 'icons.svg#i-' + name); s.appendChild(u); return s;
  }
  var JUSTIFY = { start: 'flex-start', center: 'center', end: 'flex-end', between: 'space-between' };
  var RATIO = { '16:9': '16 / 9', '4:3': '4 / 3', '1:1': '1 / 1', '3:4': '3 / 4', '9:16': '9 / 16' };
  // Pictures named like "img/door.jpg" live in ui/system/.
  function picture(src) { return /^(\/|https?:|data:)/.test(src) ? src : BASE + 'system/' + src; }
  var DRAW = {
    stack: function (p) { var e = h('div', 'ls-stack'); e.style.gap = prop(p, 'gap') + 'px'; e.style.padding = prop(p, 'pad') + 'px'; e.style.justifyItems = prop(p, 'align'); return e; },
    row: function (p) { var e = h('div', 'ls-row'); e.style.gap = prop(p, 'gap') + 'px'; e.style.justifyContent = JUSTIFY[prop(p, 'justify')] || 'flex-start'; e.style.flexWrap = prop(p, 'wrap') ? 'wrap' : 'nowrap'; return e; },
    grid: function (p) { var e = h('div', 'ls-grid'); e.style.gap = prop(p, 'gap') + 'px'; e.style.gridTemplateColumns = 'repeat(auto-fit, minmax(min(100%, ' + prop(p, 'min') + 'px), 1fr))'; return e; },
    panel: function (p) { var e = h('div', 'panel ls-stack' + (prop(p, 'over') ? ' over' : '')); e.style.padding = prop(p, 'pad') + 'px'; e.style.gap = prop(p, 'gap') + 'px'; return e; },
    backdrop: function (p) {
      var e = h('div', 'ls-backdrop ls-stack'), sh = Math.max(0, Math.min(1, +prop(p, 'shade') || 0));
      e.style.minHeight = prop(p, 'height') + 'px'; e.style.padding = prop(p, 'pad') + 'px'; e.style.gap = prop(p, 'gap') + 'px';
      e.style.alignContent = prop(p, 'align');
      e.style.backgroundImage = 'linear-gradient(180deg, rgba(0,0,0,' + (sh * 0.25) + ') 0%, rgba(0,0,0,' + (sh * 0.35) + ') 45%, rgba(0,0,0,' + sh + ') 100%)' +
        (prop(p, 'src') ? ', url("' + picture(prop(p, 'src')).replace(/"/g, '%22') + '")' : '');
      return e;
    },
    title: function (p) { var s = prop(p, 'size'), e = h(s === 'xl' ? 'h1' : s === 'l' ? 'h2' : 'h3', 'display ls-t-' + s, prop(p, 'text')); return e; },
    text: function (p) { return h('p', (prop(p, 'tone') === 'dim' ? 'dim' : '') + (prop(p, 'mono') ? ' mono' : ''), prop(p, 'text')); },
    label: function (p) { return h('span', 'label', prop(p, 'text')); },
    key: function (p) {
      var st = prop(p, 'style'), href = prop(p, 'href');
      var e = h(href ? 'a' : 'button', 'key' + (st !== 'normal' ? ' ' + st : '') + (prop(p, 'on') ? ' lit' : ''));
      if (href) e.href = href; else e.type = 'button';
      if (prop(p, 'disabled')) { if (href) e.setAttribute('aria-disabled', 'true'); else e.disabled = true; }
      if (prop(p, 'led')) e.appendChild(h('span', 'kled'));
      if (prop(p, 'icon')) e.appendChild(icon(prop(p, 'icon')));
      e.appendChild(document.createTextNode(prop(p, 'text') || ''));
      return e;
    },
    chip: function (p) { var e = h('span', 'chip' + (prop(p, 'on') ? ' on' : '')); if (prop(p, 'led')) e.appendChild(h('span', 'kled' + (prop(p, 'on') ? ' on' : ''))); e.appendChild(document.createTextNode(prop(p, 'text') || '')); return e; },
    seg: function (p) {
      var e = h('div', 'seg field'), on = prop(p, 'on'); e.setAttribute('role', 'group');
      (prop(p, 'options') || []).forEach(function (o, i) { var b = h('button', i === on ? 'on' : '', o); b.type = 'button'; e.appendChild(b); });
      return e;
    },
    field: function (p) { var e = h('input', 'field' + (prop(p, 'mono') ? ' num' : '')); e.placeholder = prop(p, 'placeholder') || ''; e.value = prop(p, 'value') || ''; e.setAttribute('aria-label', prop(p, 'placeholder') || 'Field'); return e; },
    switch: function (p) {
      var e = h('div', 'row ls-switch'), s = h('button', 'switch');
      s.type = 'button'; s.setAttribute('role', 'switch'); s.setAttribute('aria-checked', String(!!prop(p, 'on'))); s.setAttribute('aria-label', prop(p, 'text')); s.appendChild(h('i'));
      e.appendChild(h('span', 'label', prop(p, 'text'))); e.appendChild(s); return e;
    },
    fader: function (p) { var e = h('div', 'fader field'); e.setAttribute('data-fader', ''); ['min', 'max', 'value', 'dec', 'unit'].forEach(function (k) { e.setAttribute('data-' + k, prop(p, k)); }); e.setAttribute('aria-label', 'Fader'); return e; },
    knob: function (p) { var e = h('div'); e.setAttribute('data-knob', ''); ['min', 'max', 'value', 'dec', 'unit'].forEach(function (k) { e.setAttribute('data-' + k, prop(p, k)); }); e.setAttribute('aria-label', 'Knob'); return e; },
    steps: function (p) { var e = h('span', 'steps' + (prop(p, 'big') ? ' big' : '')); e.setAttribute('data-period', prop(p, 'period')); for (var i = 0; i < 4; i++) e.appendChild(h('i')); return e; },
    meter: function (p) { var e = h('div', 'vu field'); e.setAttribute('data-vu', ''); e.level = prop(p, 'level'); return e; },
    lamps: function (p) {
      var e = h('div', 'lamps field');
      ['Edit', 'Saving', 'Saved', 'Live'].forEach(function (n) { var d = h('div'); d.appendChild(h('span', 'kled')); d.appendChild(h('span', 'label', n)); e.appendChild(d); });
      if (L.lamps) L.lamps(e, prop(p, 'stage'));
      return e;
    },
    icon: function (p) { var e = h('span', 'ls-icon'); e.appendChild(icon(prop(p, 'name'), prop(p, 'size'))); return e; },
    image: function (p) {
      var e = h('div', 'ls-image field'); e.style.aspectRatio = RATIO[prop(p, 'ratio')] || '16 / 9';
      if (prop(p, 'src')) { var i = h('img'); i.src = picture(prop(p, 'src')); i.alt = prop(p, 'alt') || ''; i.loading = 'lazy'; e.appendChild(i); }
      return e;
    },
    listrow: function (p) {
      var e = h('div', 'ls-listrow');
      if (prop(p, 'icon')) { var i = h('span', 'ls-lr-icon'); i.appendChild(icon(prop(p, 'icon'))); e.appendChild(i); }
      var t = h('div', 'ls-lr-text'); t.appendChild(h('span', 'ls-lr-main', prop(p, 'text')));
      if (prop(p, 'detail')) t.appendChild(h('span', 'ls-lr-detail', prop(p, 'detail')));
      e.appendChild(t);
      if (prop(p, 'value')) e.appendChild(h('span', 'ls-lr-value ls-tone-' + prop(p, 'tone'), prop(p, 'value')));
      if (prop(p, 'chevron')) { var c = h('span', 'ls-lr-chev'); c.appendChild(icon('arrow')); e.appendChild(c); }
      return e;
    },
    tabbar: function (p) {
      var e = h('nav', 'ls-tabbar'), icons = prop(p, 'icons') || [], on = prop(p, 'on');
      (prop(p, 'options') || []).forEach(function (o, i) {
        var b = h('button', 'ls-tab' + (i === on ? ' on' : '')); b.type = 'button';
        if (icons[i]) b.appendChild(icon(icons[i])); b.appendChild(h('span', null, o)); e.appendChild(b);
      });
      e.style.gridTemplateColumns = 'repeat(' + Math.max(1, (prop(p, 'options') || []).length) + ', 1fr)';
      return e;
    },
    space: function (p) { var e = h('div', 'ls-space'); e.style.height = prop(p, 'size') + 'px'; return e; },
  };
  function draw(part, path, opts) {
    var make = DRAW[part && part.type], e;
    if (!make) { e = h('div', 'ls-unknown', 'Unknown part: ' + (part && part.type)); }
    else e = make(part);
    if (opts.mark) e.setAttribute('data-path', path);
    if (part && part.children && defs[part.type] && defs[part.type].children) {
      part.children.forEach(function (c, i) { e.appendChild(draw(c, path === '' ? String(i) : path + '.' + i, opts)); });
    }
    return e;
  }
  L.screen = function (screen, el, opts) {
    opts = opts || {};
    el.textContent = '';
    if (!screen || !screen.root) return null;
    var root = draw(screen.root, '', opts);
    if (opts.mark) root.setAttribute('data-path', '');
    el.appendChild(root);
    if (L.scan) L.scan();
    return root;
  };
})();
