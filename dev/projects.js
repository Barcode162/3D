// Dev → Projects (PLAN.md R4.2, J90): every project from toolbelt/tools.json
// as a card with its tools as tiles you tap (a tool that asks for something,
// the phone report's code, gets its box); tap the card's name for the
// project's own page (#projects/<id>): its tools by group (Make, Tune and
// check) and what Claude runs for it, as text.
(function () {
  'use strict';
  var D = window.Dev;
  var tools = null;
  D.toolList = function () {
    if (!tools) tools = D.get('/toolbelt/tools.json').catch(function (e) { tools = null; throw e; });
    return tools;
  };
  function link(el, href) {
    var a = document.createElement('a'); a.className = 'dev-row-btn'; a.href = href;
    if (/^https?:/.test(href)) { a.target = '_blank'; a.rel = 'noopener'; }
    a.appendChild(el); return a;
  }
  function askBox(t) {
    var box = document.createElement('div'); box.className = 'dev-ask';
    var f = D.part({ type: 'field', placeholder: 'K7Q2XD', mono: true }), go = D.part({ type: 'key', text: 'Open', style: 'small' });
    f.maxLength = 6; f.autocapitalize = 'characters'; f.setAttribute('aria-label', t.ask);
    go.addEventListener('click', function () { var code = f.value.trim().toUpperCase(); if (/^[A-Z0-9]{6}$/.test(code)) location.href = t.href + code; else D.toast('A code is six letters or numbers.'); });
    box.appendChild(f); box.appendChild(go); return box;
  }
  function toolRow(t) {
    var soon = t.status === 'soon';
    var row = D.part({ type: 'listrow', text: t.name, detail: t.run && !soon ? t.what + '  ' : t.what, icon: t.icon, value: soon ? 'Soon' : '', chevron: !!t.href && !soon && !t.ask });
    if (t.run) { var c = document.createElement('code'); c.className = 'dev-cmd'; c.textContent = t.run; row.querySelector('.ls-lr-text').appendChild(c); }
    if (soon) { row.style.opacity = '.55'; return row; }
    if (t.ask) { row.querySelector('.ls-lr-text').appendChild(askBox(t)); return row; }
    return t.href ? link(row, t.href) : row;
  }
  // A tool as a tile: its icon, name and one line; it opens the tool.
  function toolTile(t, i) {
    var soon = t.status === 'soon';
    var el = document.createElement(t.href && !soon && !t.ask ? 'a' : 'div');
    el.className = 'dev-tile dev-tool' + (soon ? ' soon' : '') + (el.tagName === 'A' ? ' dev-data' : '');
    el.style.setProperty('--i', String(Math.min(i, 12)));
    if (el.tagName === 'A') { el.href = t.href; if (/^https?:/.test(t.href)) { el.target = '_blank'; el.rel = 'noopener'; } }
    var ic = document.createElement('span'); ic.className = 'dev-tile-icon';
    var ns = 'http://www.w3.org/2000/svg', sv = document.createElementNS(ns, 'svg'), u = document.createElementNS(ns, 'use');
    sv.setAttribute('class', 'ico'); sv.setAttribute('aria-hidden', 'true'); u.setAttribute('href', '/ui/icons.svg#i-' + (t.icon || 'box')); sv.appendChild(u); ic.appendChild(sv);
    var b = document.createElement('b'); b.textContent = t.name;
    var s = document.createElement('span'); s.textContent = soon ? 'Soon' : t.what;
    el.appendChild(ic); el.appendChild(b); el.appendChild(s);
    if (t.ask && !soon) el.appendChild(askBox(t));
    return el;
  }
  function grid(list) {
    var g = document.createElement('div'); g.className = 'dev-tiles';
    list.forEach(function (t, i) { g.appendChild(toolTile(t, i)); });
    return g;
  }
  D.tabs.projects.fill = function () {
    return D.toolList().then(function (data) {
      var slot = D.slot('projects'); if (!slot) return;
      data.projects.forEach(function (p) {
        var card = D.part({ type: 'panel', gap: 12 }); card.classList.add('dev-card');
        var head = D.part({ type: 'row', justify: 'between', children: [{ type: 'title', text: p.name, size: 'm' }, { type: 'chip', text: p.status, on: p.status === 'Live', led: p.status === 'Live' }] });
        card.appendChild(D.link(head, 'projects/' + p.id));
        card.appendChild(D.part({ type: 'text', text: p.about, tone: 'dim' }));
        card.dataset.project = p.id;
        var mine = data.tools.filter(function (t) { return t.project === p.id && !t.run; });
        var runs = data.tools.filter(function (t) { return t.project === p.id && t.run; });
        if (mine.length) card.appendChild(grid(mine));
        else if (!runs.length) card.appendChild(D.part({ type: 'text', text: 'No tools yet.', tone: 'dim' }));
        var open = D.part({ type: 'key', text: runs.length ? 'Open ' + p.name + ' (and what Claude runs)' : 'Open ' + p.name, style: 'small', icon: 'arrow' });
        open.dataset.go = 'projects/' + p.id; open.classList.add('dev-data');
        card.appendChild(open);
        slot.appendChild(card);
      });
      if (D.motion) D.motion.enter(slot.closest('#view'));
    }).catch(function () { var s = D.slot('projects'); if (s) s.appendChild(D.part({ type: 'text', text: 'The project list did not load.', tone: 'dim' })); });
  };
  D.pages.projects = [
    { match: /^([a-z0-9-]+)$/, title: function (m) { return ({ bar3d: 'Bar3D', looped: 'Looped', script: 'The script language', workshop: 'The workshop' })[m[1]] || m[1]; },
      draw: function (view, m) {
        return D.toolList().then(function (data) {
          var p = data.projects.filter(function (x) { return x.id === m[1]; })[0];
          if (!p) { view.appendChild(D.part({ type: 'text', text: 'No such project.', tone: 'dim' })); return; }
          view.appendChild(D.part({ type: 'stack', gap: 2, children: [{ type: 'label', text: p.status }, { type: 'title', text: p.name, size: 'l' }] }));
          var about = D.part({ type: 'panel', gap: 8, children: [{ type: 'text', text: p.about, tone: 'dim' }] });
          about.classList.add('dev-card'); about.dataset.project = p.id;
          if (p.href) { var k = D.part({ type: 'key', text: 'Open ' + p.name, style: 'go', href: p.href }); k.classList.add('dev-data'); about.appendChild(k); }
          view.appendChild(about);
          (data.groups || []).forEach(function (g) {
            var list = data.tools.filter(function (t) { return t.project === p.id && (t.group || 'make') === g.id; });
            if (!list.length) return;
            view.appendChild(D.part({ type: 'title', text: g.name, size: 's' }));
            var panel = D.part({ type: 'panel', pad: 0, gap: 0 }); panel.dataset.group = g.id;
            list.forEach(function (t) { panel.appendChild(toolRow(t)); });
            view.appendChild(panel);
          });
        });
      } },
  ];
})();
