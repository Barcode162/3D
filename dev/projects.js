// Dev → Projects (PLAN.md R4.2): every project from toolbelt/tools.json with
// its tools. A tool with a page is a row you tap; one that asks for something
// (a report code) gets its box; Claude's commands show as text.
(function () {
  'use strict';
  var D = window.Dev;
  function link(el, href) {
    var a = document.createElement('a'); a.className = 'dev-row-btn'; a.href = href;
    if (/^https?:/.test(href)) { a.target = '_blank'; a.rel = 'noopener'; }
    a.appendChild(el); return a;
  }
  function toolRow(t) {
    var soon = t.status === 'soon';
    var row = D.part({ type: 'listrow', text: t.name, detail: t.run && !soon ? t.what + '  ' : t.what, icon: t.icon, value: soon ? 'Soon' : '', chevron: !!t.href && !soon && !t.ask });
    if (t.run) { var c = document.createElement('code'); c.className = 'dev-cmd'; c.textContent = t.run; row.querySelector('.ls-lr-text').appendChild(c); }
    if (soon) { row.style.opacity = '.55'; return row; }
    if (t.ask) {
      var box = document.createElement('div'); box.className = 'dev-ask';
      var f = D.part({ type: 'field', placeholder: 'K7Q2XD', mono: true }), go = D.part({ type: 'key', text: 'Open', style: 'small' });
      f.maxLength = 6; f.autocapitalize = 'characters'; f.setAttribute('aria-label', t.ask);
      go.addEventListener('click', function () { var code = f.value.trim().toUpperCase(); if (/^[A-Z0-9]{6}$/.test(code)) location.href = t.href + code; else D.toast('A code is six letters or numbers.'); });
      box.appendChild(f); box.appendChild(go); row.querySelector('.ls-lr-text').appendChild(box);
      return row;
    }
    return t.href ? link(row, t.href) : row;
  }
  D.tabs.projects.fill = function () {
    return D.get('/toolbelt/tools.json').then(function (data) {
      var slot = D.slot('projects'); if (!slot) return;
      data.projects.forEach(function (p) {
        var card = D.part({ type: 'panel', gap: 10, children: [
          { type: 'row', justify: 'between', children: [{ type: 'title', text: p.name, size: 'm' }, { type: 'chip', text: p.status, on: p.status === 'Live', led: p.status === 'Live' }] },
          { type: 'text', text: p.about, tone: 'dim' },
        ] });
        card.dataset.project = p.id;
        var list = D.part({ type: 'panel', pad: 0, gap: 0 });
        var tools = data.tools.filter(function (t) { return t.project === p.id; });
        tools.forEach(function (t) { list.appendChild(toolRow(t)); });
        if (list.children.length) card.appendChild(list);
        else card.appendChild(D.part({ type: 'text', text: 'No tools yet.', tone: 'dim' }));
        slot.appendChild(card);
      });
    }).catch(function () { var s = D.slot('projects'); if (s) s.appendChild(D.part({ type: 'text', text: 'The project list did not load.', tone: 'dim' })); });
  };
})();
