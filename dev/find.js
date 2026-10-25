// Dev → Find (J90): one field over everything Dev knows: every page, every
// project and tool (toolbelt/tools.json), every chapter and job on the plan
// (its number or any word of it). Type, tap the answer, you're there. Opens
// from the magnifier in the top bar.
(function () {
  'use strict';
  var D = window.Dev;
  var PAGES = [['Overview', 'overview', 'home', 'The Live panel: the bar, who\'s in it, the deploys'], ['Projects', 'projects', 'layers', 'Every project and its tools'],
    ['Plan', 'plan', 'rule', 'The job in Now, Up next, the road, tell Claude'], ['Publish', 'publish', 'publish', 'News, highlights, the site\'s words'],
    ['People', 'people', 'people', 'Who may play, admins, Claude\'s key'], ['Done', 'plan/done', 'check', 'Every finished job'],
    ['Parked', 'plan/parked', 'place', 'Ideas waiting for your word'], ['North star and your rules', 'plan/star', 'target', 'What it all serves']];
  var index = null;
  function build() {
    if (index) return Promise.resolve(index);
    var out = PAGES.map(function (p) { return { title: p[0], detail: p[3], go: p[1], icon: p[2], kind: 'Page' }; });
    return Promise.all([
      D.toolList ? D.toolList().catch(function () { return null; }) : null,
      D.plan().catch(function () { return null; }),
    ]).then(function (a) {
      var tools = a[0], plan = a[1];
      if (tools) {
        tools.projects.forEach(function (p) { out.push({ title: p.name, detail: p.about, go: 'projects/' + p.id, icon: 'layers', kind: 'Project' }); });
        tools.tools.forEach(function (t) {
          out.push(t.href && !t.ask ? { title: t.name, detail: t.what, href: t.href, icon: t.icon || 'box', kind: 'Tool' }
                                    : { title: t.name, detail: t.what + (t.run ? ' · ' + t.run : ''), go: 'projects/' + t.project, icon: t.icon || 'box', kind: 'Tool' });
        });
      }
      if (plan) {
        plan.chapters.forEach(function (c) { out.push({ title: c.name, detail: c.now || c.what, go: 'plan/' + c.id, icon: 'rule', kind: 'Chapter' }); });
        if (plan.now) out.push({ title: plan.now.tag + ' · ' + plan.now.title, detail: 'In progress now', go: 'plan/' + plan.now.tag, icon: 'loop', kind: 'Job' });
        Object.keys(plan.road).forEach(function (ch) {
          plan.road[ch].forEach(function (it) { out.push({ title: it.tag + ' · ' + D.planUi.short(it.text)[0], detail: it.text, go: 'plan/' + it.tag, icon: 'note', kind: 'Job', extra: it.formerly || '' }); });
        });
      }
      index = out;
      return out;
    });
  }
  function score(item, words) {
    var hay = (item.title + ' ' + item.detail + ' ' + (item.extra || '') + ' ' + item.kind).toLowerCase(), title = item.title.toLowerCase(), s = 0;
    for (var i = 0; i < words.length; i++) {
      var w = words[i];
      if (hay.indexOf(w) < 0) return -1;
      s += title.indexOf(w) === 0 ? 6 : title.indexOf(w) > 0 ? 3 : 1;
    }
    return s + (item.kind === 'Page' ? 1 : 0);
  }
  D.find = function (q) {
    return build().then(function (all) {
      var words = String(q || '').toLowerCase().split(/\s+/).filter(Boolean);
      if (!words.length) return all.filter(function (x) { return x.kind === 'Page' || x.kind === 'Project'; });
      return all.map(function (x) { return [score(x, words), x]; }).filter(function (p) { return p[0] >= 0; })
        .sort(function (a, b) { return b[0] - a[0]; }).slice(0, 24).map(function (p) { return p[1]; });
    });
  };
  function open() {
    if (document.querySelector('.dev-find')) return;
    var s = document.createElement('div'); s.className = 'dev-find'; s.setAttribute('role', 'dialog'); s.setAttribute('aria-label', 'Find');
    var card = D.part({ type: 'panel', gap: 10 }); card.classList.add('dev-find-card');
    var field = D.part({ type: 'field', placeholder: 'Find a page, a tool, a job…' });
    field.setAttribute('aria-label', 'Find'); field.type = 'search'; field.autocapitalize = 'none'; field.spellcheck = false;
    var list = D.part({ type: 'panel', pad: 0, gap: 0 }); list.classList.add('dev-find-list');
    card.appendChild(field); card.appendChild(list);
    s.appendChild(card);
    s.addEventListener('click', function (e) { if (e.target === s) close(); });
    document.body.appendChild(s);
    requestAnimationFrame(function () { s.classList.add('open'); field.focus(); });
    var asked = 0;
    function show() {
      var my = ++asked;
      D.find(field.value).then(function (items) {
        if (my !== asked) return;
        list.textContent = '';
        if (!items.length) { list.appendChild(D.part({ type: 'listrow', text: 'Nothing found', detail: 'Try another word: a tool, a job number (J85), a chapter.' })); return; }
        items.forEach(function (it) {
          var row = D.part({ type: 'listrow', icon: it.icon, text: it.title, detail: it.detail, value: it.kind, chevron: true });
          var a = document.createElement('a'); a.className = 'dev-row-btn'; a.dataset.find = it.go || it.href;
          if (it.go) { a.href = '#' + it.go; a.addEventListener('click', function (e) { e.preventDefault(); close(); D.go(it.go); }); }
          else { a.href = it.href; a.addEventListener('click', function () { close(); }); }
          a.appendChild(row); list.appendChild(a);
        });
      });
    }
    field.addEventListener('input', show);
    field.addEventListener('keydown', function (e) {
      if (e.key === 'Escape') close();
      if (e.key === 'Enter') { var first = list.querySelector('a'); if (first) first.click(); }
    });
    show();
    function close() { s.classList.remove('open'); setTimeout(function () { s.remove(); }, 220); }
  }
  D.openFind = open;
  document.addEventListener('click', function (e) { if (e.target.closest && e.target.closest('#find')) open(); });
})();
