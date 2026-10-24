// Dev → Overview (PLAN.md R4.3): is everything up (the bar's live build, the
// room server, the last deploy's tests), what needs you (news to publish),
// and the step in Now. Each row shows "Checking…" until its answer comes.
(function () {
  'use strict';
  var D = window.Dev;
  var DAYS = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];
  function ago(t) { var s = Math.max(0, Date.now() / 1000 - t); return s < 90 ? 'just now' : s < 5400 ? Math.round(s / 60) + ' min ago' : s < 129600 ? Math.round(s / 3600) + ' h ago' : Math.round(s / 86400) + ' days ago'; }
  D.ago = ago;
  function row(slot, key, p) {
    var el = D.part(Object.assign({ type: 'listrow' }, p)); el.dataset.row = key;
    var old = slot.querySelector('[data-row="' + key + '"]');
    if (old) slot.replaceChild(el, old); else slot.appendChild(el);
    return el;
  }
  function linkRow(slot, key, p, href) {
    var el = row(slot, key, p), a = document.createElement('a');
    a.className = 'dev-row-btn'; a.href = href; a.dataset.row = key; slot.replaceChild(a, el); el.removeAttribute('data-row'); a.appendChild(el);
    if (/^https?:/.test(href)) { a.target = '_blank'; a.rel = 'noopener'; }
  }
  D.tabs.overview.fill = function () {
    var d = new Date(), today = D.slot('today');
    if (today) today.textContent = 'Dev · ' + DAYS[d.getDay()] + ' ' + d.getDate() + ' ' + d.toLocaleString('en', { month: 'short' });
    var status = D.slot('status'), needs = D.slot('needs'), now = D.slot('now');
    if (status) {
      row(status, 'bar', { text: 'The bar', icon: 'bar', detail: 'Checking…' });
      row(status, 'room', { text: 'Room server', icon: 'people', detail: 'Checking…' });
      row(status, 'ci', { text: 'Last deploy', icon: 'check', detail: 'Checking…' });
      (D.demo ? Promise.resolve({ build: 'demo', ws: '' }) : D.get('/bar/config.json')).then(function (c) {
        row(status, 'bar', { text: 'The bar', icon: 'bar', detail: 'build ' + c.build, value: 'Up', tone: 'ok' });
        if (!c.ws) return row(status, 'room', { text: 'Room server', icon: 'people', detail: 'Not set up', value: '?', tone: 'warn' });
        return D.get(c.ws.replace(/^ws/, 'http') + '/health').then(function (h) {
          var m = h.main || {};
          row(status, 'room', { text: 'Room server', icon: 'people', detail: (m.players || 0) + ' in the bar · ' + ((m.usage && m.usage.messages) || 0).toLocaleString('en') + ' messages today', value: h.ok ? 'Up' : 'Down', tone: h.ok ? 'ok' : 'bad' });
        });
      }).catch(function () {
        row(status, 'bar', { text: 'The bar', icon: 'bar', detail: 'Its config did not answer', value: 'Down', tone: 'bad' });
        row(status, 'room', { text: 'Room server', icon: 'people', detail: 'Not checked', value: '?', tone: 'warn' });
      });
      (D.demo ? Promise.resolve({ ok: 1, ci: null }) : D.post('/dev/api.php', { a: 'ci' })).then(function (j) {
        var c = j.ok && j.ci;
        if (!c) return row(status, 'ci', { text: 'Last deploy', icon: 'check', detail: 'No report yet (the next game deploy writes one)', value: '?', tone: 'warn' });
        var good = c.status === 'success';
        linkRow(status, 'ci', { text: 'Last deploy', icon: 'check', detail: 'build ' + c.build + ' · ' + ago(c.at) + (good ? ' · every test passed' : ' · a test failed'), value: good ? 'Green' : 'Red', tone: good ? 'ok' : 'bad', chevron: true }, c.url);
      });
    }
    if (needs) {
      (D.demo ? Promise.resolve({ ok: 1, posts: [] }) : D.post('/hub/hub.php', { a: 'posts' })).then(function (j) {
        if (!j.ok) return row(needs, 'news', { text: 'News', icon: 'news', detail: j.status === 403 || j.status === 401 ? 'Only the owner publishes news.' : 'Did not answer.', value: '?', tone: 'warn' });
        var drafts = j.posts.filter(function (p) { return !p.published; });
        if (!drafts.length) return row(needs, 'news', { text: 'Nothing waiting', icon: 'check', detail: 'News is all published.' });
        linkRow(needs, 'news', { text: drafts.length === 1 ? 'A news post to publish' : drafts.length + ' news posts to publish', icon: 'news', detail: D.plain(drafts[0].title), value: 'Draft', tone: 'warn', chevron: true }, '#publish');
      });
    }
    if (now) {
      D.plan().then(function (plan) {
        var n = plan.now; now.textContent = '';
        if (!n) return now.appendChild(D.part({ type: 'text', text: 'Nothing in Now.', tone: 'dim' }));
        var done = n.pieces.filter(function (p) { return p.done; }).length;
        now.appendChild(D.part({ type: 'row', justify: 'between', children: [{ type: 'text', text: n.tag + ' · ' + n.title }, { type: 'chip', text: done + ' of ' + n.pieces.length, on: true }] }));
        now.appendChild(D.part({ type: 'meter', level: n.pieces.length ? done / n.pieces.length : 0 }));
        var next = n.pieces.filter(function (p) { return !p.done; })[0];
        if (next) now.appendChild(D.part({ type: 'text', text: 'Next: ' + next.tag + ' ' + next.text, tone: 'dim' }));
        if (plan.upNext) now.appendChild(D.part({ type: 'text', text: 'After: ' + plan.upNext, tone: 'dim' }));
        if (window.Looped && Looped.scan) Looped.scan();
      }).catch(function () { now.textContent = ''; now.appendChild(D.part({ type: 'text', text: 'The plan did not load.', tone: 'dim' })); });
    }
  };
})();
