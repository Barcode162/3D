// Dev → Overview (PLAN.md R4.3, J5.6): is everything up (the bar's live
// build, the room server, each deploy's real state: the game's and the
// site's, J2.3; Claude's errors this week, from the incidents), what needs you
// (news to publish, world proposals to look at, questions only you can
// settle), and the job in Now with its next piece. Each row shows "Checking…"
// until its answer comes.
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
  // One deploy's report (tools/ci_status.mjs) in words: {detail, value, tone}.
  function deployState(c) {
    if (!c) return { detail: 'No report yet (the next deploy writes one)', value: '?', tone: 'warn' };
    var list = function (a) { return (a || []).join(', '); };
    var tests = c.tests === 'running' ? 'tests running…'
      : c.tests === 'passed' ? 'every test passed'
      : c.tests === 'cancelled' ? 'stopped (a newer deploy took over)'
      : (c.failed && c.failed.length ? c.failed.length + ' failed: ' + list(c.failed) : 'a step failed');
    var allowed = c.allowed && c.allowed.length ? ' · allowed to fail: ' + list(c.allowed) : '';
    return {
      detail: 'build ' + c.build + ' · ' + (c.live ? 'live' : 'not live') + ' · ' + tests + allowed + ' · ' + ago(c.at),
      value: { running: 'Running', passed: 'Green', cancelled: 'Stopped' }[c.tests] || 'Red',
      tone: c.tests === 'passed' ? 'ok' : c.tests === 'failed' ? 'bad' : 'warn',
    };
  }
  D.deployState = deployState;
  function deployRow(slot, key, name, c) {
    var st = deployState(c), p = { text: name, icon: 'check', detail: st.detail, value: st.value, tone: st.tone };
    if (c && c.url) { p.chevron = true; linkRow(slot, 'ci-' + key, p, c.url); } else row(slot, 'ci-' + key, p);
  }

  D.tabs.overview.fill = function () {
    var d = new Date(), today = D.slot('today');
    if (today) today.textContent = 'Dev · ' + DAYS[d.getDay()] + ' ' + d.getDate() + ' ' + d.toLocaleString('en', { month: 'short' });
    var status = D.slot('status'), needs = D.slot('needs'), now = D.slot('now');
    if (status) {
      row(status, 'bar', { text: 'The bar', icon: 'bar', detail: 'Checking…' });
      row(status, 'room', { text: 'Room server', icon: 'people', detail: 'Checking…' });
      row(status, 'ci-game', { text: 'Game deploy', icon: 'check', detail: 'Checking…' });
      row(status, 'ci-site', { text: 'Site deploy', icon: 'check', detail: 'Checking…' });
      row(status, 'errors', { text: 'Errors this week', icon: 'check', detail: 'Checking…' });
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
      (D.demo ? Promise.resolve({ ok: 1, ci: { game: null, site: null } }) : D.post('/dev/api.php', { a: 'ci' })).then(function (j) {
        var ci = (j.ok && j.ci) || {};
        deployRow(status, 'game', 'Game deploy', ci.game);
        deployRow(status, 'site', 'Site deploy', ci.site);
      });
    }
    if (needs) {
      row(needs, 'news', { text: 'News', icon: 'news', detail: 'Checking…' });
      // World proposals (Claude's, from the Studio's store) waiting for your look.
      (D.demo ? Promise.resolve({ ok: 1, proposals: [] }) : D.post('/studio/api.php', { a: 'proposals' })).then(function (j) {
        var list = (j && j.ok && j.proposals) || [];
        if (list.length) linkRow(needs, 'proposals', { text: list.length === 1 ? 'A world change to look at' : list.length + ' world changes to look at', icon: 'place', detail: D.plain(list[0].note || ''), value: 'Proposed', tone: 'warn', chevron: true }, '/studio/');
      }).catch(function () {});
      (D.demo ? Promise.resolve({ ok: 1, posts: [] }) : D.post('/hub/hub.php', { a: 'posts' })).then(function (j) {
        if (!j.ok) return row(needs, 'news', { text: 'News', icon: 'news', detail: j.status === 403 || j.status === 401 ? 'Only the owner publishes news.' : 'Did not answer.', value: '?', tone: 'warn' });
        var drafts = j.posts.filter(function (p) { return !p.published; });
        if (!drafts.length) return row(needs, 'news', { text: 'Nothing waiting', icon: 'check', detail: 'News is all published.' });
        linkRow(needs, 'news', { text: drafts.length === 1 ? 'A news post to publish' : drafts.length + ' news posts to publish', icon: 'news', detail: D.plain(drafts[0].title), value: 'Draft', tone: 'warn', chevron: true }, '#publish');
      });
    }
    D.plan().then(function (plan) {
      // Claude's errors in the last 7 days (docs/records/incidents/, sent by plan.yml).
      var week = Date.now() - 7 * 86400000, errs = 0, last = null;
      plan.incidents.forEach(function (x) { if (Date.parse(x.date) >= week - 86400000) { errs += x.errors || 1; last = last || x; } });
      if (status) row(status, 'errors', { text: 'Errors this week', icon: 'check', detail: errs ? 'Latest: ' + D.plain(last.title) + ' (' + last.status + ')' : 'None: every session went to plan.', value: String(errs), tone: errs ? 'warn' : 'ok' });
      // Questions only you can settle (open known issues that wait for your call).
      if (needs) plan.questions.forEach(function (q) {
        row(needs, 'q-' + q.id, { text: 'Your call: ' + q.id, icon: 'note', detail: D.plain(q.text), value: 'Question', tone: 'warn' });
      });
    }).catch(function () { if (status) row(status, 'errors', { text: 'Errors this week', icon: 'check', detail: 'The plan did not load.', value: '?', tone: 'warn' }); });
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
