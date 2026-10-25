// Dev → Overview (PLAN.md R4.3, J5.6, J90): the Live panel first: tiles for
// the bar (its live build), who's in it now (the room server, with a
// heartbeat that beats with them), each deploy's real state (J2.3: running,
// green or red, the failed tests by name), Claude's errors this week (from
// the incidents) and Claude's latest finished job. It checks again every 15 s
// while you look at it; a tile whose answer changed flashes and its number
// counts to the new one. Tap a tile: its details and the reactions that go to
// Claude's notes (Looks good, Broke it, a note). Then what needs you (news to
// publish, world proposals, questions only you can settle) and the job in Now
// (tap: its page).
(function () {
  'use strict';
  var D = window.Dev;
  var DAYS = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];
  var REFRESH_MS = 15000;
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
    else if (/^#/.test(href)) a.dataset.go = href.slice(1);
  }
  // One deploy's report (tools/ci_status.mjs) in words: {detail, value, tone}.
  function deployState(c) {
    if (!c) return { detail: 'No report yet (the next deploy writes one)', value: '?', tone: 'warn' };
    var list = function (a) { return (a || []).join(', '); };
    var tests = c.tests === 'running' ? 'tests running…'
      : c.tests === 'off' ? 'browser tests on request (you test on the phone)'
      : c.tests === 'passed' ? 'every test passed'
      : c.tests === 'cancelled' ? 'stopped (a newer deploy took over)'
      : (c.failed && c.failed.length ? c.failed.length + ' failed: ' + list(c.failed) : 'a step failed');
    var allowed = c.allowed && c.allowed.length ? ' · allowed to fail: ' + list(c.allowed) : '';
    return {
      detail: 'build ' + c.build + ' · ' + (c.live ? 'live' : 'not live') + ' · ' + tests + allowed + ' · ' + ago(c.at),
      value: { running: 'Running', passed: 'Green', cancelled: 'Stopped', off: c.live ? 'Live' : 'Not live' }[c.tests] || 'Red',
      tone: c.tests === 'passed' || (c.tests === 'off' && c.live) ? 'ok' : c.tests === 'failed' ? 'bad' : 'warn',
      running: c.tests === 'running',
    };
  }
  D.deployState = deployState;

  // ---- The Live panel's tiles ----
  var ns = 'http://www.w3.org/2000/svg';
  function svgIcon(name) {
    var sv = document.createElementNS(ns, 'svg'), u = document.createElementNS(ns, 'use');
    sv.setAttribute('class', 'ico'); sv.setAttribute('aria-hidden', 'true'); u.setAttribute('href', '/ui/icons.svg#i-' + name); sv.appendChild(u); return sv;
  }
  // A heartbeat line that beats faster with more people (dev.css: --beat):
  // one beat drawn once (a mask), slid along by the GPU.
  function heartbeat() {
    var box = document.createElement('div'); box.className = 'dev-pulse'; box.setAttribute('aria-hidden', 'true');
    box.appendChild(document.createElement('i'));
    return box;
  }
  // A tile: its words in reading order (label, detail, value: the value is
  // drawn big on top by dev.css); `key` finds it again when it refreshes.
  function tile(slot, key, t) {
    var el = slot.querySelector('[data-tile="' + key + '"]');
    var fresh = !el;
    if (fresh) {
      el = document.createElement('button'); el.type = 'button'; el.className = 'dev-tile dev-live-tile'; el.dataset.tile = key; el.dataset.row = key;
      el.innerHTML = '<span class="dev-tile-icon"></span><span class="dev-tile-label"></span><span class="dev-tile-detail"></span><b class="dev-tile-value"></b>';
      slot.appendChild(el);
      el.addEventListener('click', function () { sheet(el); });
    }
    var sig = JSON.stringify([t.label, t.detail, t.value, t.tone]);
    var changed = !fresh && el.dataset.sig !== sig;
    el.dataset.sig = sig;
    el.dataset.tone = t.tone || 'normal';
    el.classList.toggle('dev-running', !!t.running);
    el.querySelector('.dev-tile-icon').replaceChildren(svgIcon(t.icon));
    el.querySelector('.dev-tile-label').textContent = t.label;
    el.querySelector('.dev-tile-detail').textContent = t.detail;
    var v = el.querySelector('.dev-tile-value');
    // A number counts to its new value when it changes (not when it first
    // appears: it shows at once, so what you read is always the answer).
    var val = String(t.value);
    if (/^\d+$/.test(val) && /^\d+$/.test(v.dataset.shown || '')) v.dataset.count = val;
    else { delete v.dataset.count; v.textContent = val; v.dataset.shown = val; }
    if (t.pulse !== undefined) {
      var hb = el.querySelector('.dev-pulse') || el.appendChild(heartbeat());
      el.style.setProperty('--beat', (t.pulse > 0 ? Math.max(0.45, 1.6 - t.pulse * 0.15) : 4) + 's');
      hb.classList.toggle('flat', !(t.pulse > 0));
    }
    el.dataset.href = t.href || '';
    el.dataset.what = t.what || (t.label + ': ' + t.value + ' (' + t.detail + ')');
    if (D.motion) { D.motion.count(el); if (changed) D.motion.flash(el); }
    return el;
  }

  // ---- A tile's sheet: the details, and reactions for Claude ----
  function sheet(tileEl) {
    var old = document.querySelector('.dev-sheet'); if (old) old.remove();
    var s = document.createElement('div'); s.className = 'dev-sheet'; s.setAttribute('role', 'dialog'); s.setAttribute('aria-label', tileEl.querySelector('.dev-tile-label').textContent);
    var card = D.part({ type: 'panel', gap: 10 }); card.classList.add('dev-sheet-card');
    card.appendChild(D.part({ type: 'label', text: 'Live' }));
    card.appendChild(D.part({ type: 'title', text: tileEl.querySelector('.dev-tile-label').textContent + ': ' + tileEl.querySelector('.dev-tile-value').textContent, size: 'm' }));
    card.appendChild(D.part({ type: 'text', text: tileEl.querySelector('.dev-tile-detail').textContent, tone: 'dim' }));
    if (tileEl.dataset.href) {
      var open = D.part({ type: 'key', text: 'Open', style: 'small', icon: 'arrow' });
      open.addEventListener('click', function () { close(); if (/^#/.test(tileEl.dataset.href)) D.go(tileEl.dataset.href.slice(1)); else window.open(tileEl.dataset.href, '_blank', 'noopener'); });
      card.appendChild(open);
    }
    card.appendChild(D.part({ type: 'label', text: 'Tell Claude' }));
    var react = document.createElement('div'); react.className = 'dev-react';
    var note = D.part({ type: 'field', placeholder: 'A note about this (optional)' });
    [['Looks good', 'check', 'Looks good'], ['Broke it', 'x', 'Broke it'], ['Note', 'note', 'Note']].forEach(function (r) {
      var b = document.createElement('button'); b.type = 'button'; b.dataset.react = r[0];
      b.appendChild(svgIcon(r[1])); b.appendChild(document.createTextNode(r[0]));
      b.addEventListener('click', function () {
        var extra = (note.value || '').trim();
        if (r[0] === 'Note' && !extra) { note.focus(); D.toast('Write the note first, then tap Note.'); return; }
        var text = r[2] + ': ' + tileEl.dataset.what + (extra ? ' — ' + extra : '');
        b.disabled = true;
        (D.demo ? Promise.resolve({ ok: 1 }) : D.post('/dev/api.php', { a: 'note', kind: 'note', text: text.slice(0, 1900) })).then(function (j) {
          b.disabled = false;
          if (!j.ok) { D.toast(j.error || 'That didn\'t send.'); return; }
          b.classList.add('sent');
          D.toast(r[0] === 'Broke it' ? 'Sent: Claude looks at it first thing at the next go.' : 'Sent to Claude.');
          setTimeout(close, 500);
        });
      });
      react.appendChild(b);
    });
    card.appendChild(react);
    card.appendChild(note);
    var x = D.part({ type: 'key', text: 'Close', style: 'small' }); x.addEventListener('click', close); card.appendChild(x);
    s.appendChild(card);
    s.addEventListener('click', function (e) { if (e.target === s) close(); });
    document.body.appendChild(s);
    requestAnimationFrame(function () { s.classList.add('open'); });
    function close() { s.classList.remove('open'); setTimeout(function () { s.remove(); }, 260); }
  }

  var timer = 0;
  D.onLeave = function () { clearInterval(timer); timer = 0; var s = document.querySelector('.dev-sheet'); if (s) s.remove(); };
  // Every answer the panel shows, once; refresh() runs it again.
  function live(status, fresh) {
    if (!status || !status.isConnected) { clearInterval(timer); timer = 0; return; }
    (D.demo ? Promise.resolve({ build: 'demo', ws: '' }) : D.get('/bar/config.json')).then(function (c) {
      tile(status, 'bar', { icon: 'bar', label: 'The bar', detail: 'build ' + c.build, value: 'Up', tone: 'ok', href: '/bar/', what: 'the bar, build ' + c.build });
      if (!c.ws) return tile(status, 'room', { icon: 'people', label: 'In the bar now', detail: 'Room server: not set up', value: '?', tone: 'warn', pulse: 0 });
      return D.get(c.ws.replace(/^ws/, 'http') + '/health').then(function (h) {
        var m = h.main || {}, n = m.players || 0;
        tile(status, 'room', { icon: 'people', label: 'In the bar now', detail: (h.ok ? 'Room server up' : 'Room server down') + ' · ' + ((m.usage && m.usage.messages) || 0).toLocaleString('en') + ' messages today', value: String(n), tone: h.ok ? 'ok' : 'bad', pulse: n, href: '/bar/', what: 'the room, ' + n + ' in the bar' });
      }).catch(function () { tile(status, 'room', { icon: 'people', label: 'In the bar now', detail: 'The room server did not answer', value: 'Down', tone: 'bad', pulse: 0 }); });
    }).catch(function () {
      tile(status, 'bar', { icon: 'bar', label: 'The bar', detail: 'Its config did not answer', value: 'Down', tone: 'bad' });
      tile(status, 'room', { icon: 'people', label: 'In the bar now', detail: 'Not checked', value: '?', tone: 'warn', pulse: 0 });
    });
    (D.demo ? Promise.resolve({ ok: 1, ci: { game: null, site: null } }) : D.post('/dev/api.php', { a: 'ci' })).then(function (j) {
      var ci = (j.ok && j.ci) || {};
      [['game', 'Game deploy'], ['site', 'Site deploy']].forEach(function (d) {
        var st = deployState(ci[d[0]]);
        tile(status, 'ci-' + d[0], { icon: 'check', label: d[1], detail: st.detail, value: st.value, tone: st.tone, running: st.running, href: ci[d[0]] && ci[d[0]].url, what: d[1].toLowerCase() + ', ' + st.detail });
      });
    });
    D.plan(fresh).then(function (plan) {
      // Claude's errors in the last 7 days (docs/records/incidents/, sent by plan.yml).
      var week = Date.now() - 7 * 86400000, errs = 0, last = null;
      plan.incidents.forEach(function (x) { if (Date.parse(x.date) >= week - 86400000) { errs += x.errors || 1; last = last || x; } });
      tile(status, 'errors', { icon: 'check', label: 'Errors this week', detail: errs ? 'Latest: ' + D.plain(last.title) + ' (' + last.status + ')' : 'None: every session went to plan.', value: String(errs), tone: errs ? 'warn' : 'ok', href: '#plan', what: 'Claude\'s errors this week: ' + errs });
      var d0 = plan.done[0];
      if (d0) {
        var tag = (d0.text.match(/^(J\d+)/) || [])[1];
        // The job's number big; what was done and its build under it.
        var said = D.planUi ? D.planUi.short(d0.text) : [d0.text, ''];
        tile(status, 'latest', { icon: 'history', label: 'Claude\'s latest', detail: (said[1] || said[0]) + ' · ' + (d0.sha === 'this push' ? 'going live now' : 'build ' + d0.sha),
          value: tag || 'Done', tone: 'normal', href: tag ? '#plan/' + tag : '#plan/done', what: 'Claude\'s latest: ' + d0.text.slice(0, 160) });
      }
    }).catch(function () { tile(status, 'errors', { icon: 'check', label: 'Errors this week', detail: 'The plan did not load.', value: '?', tone: 'warn' }); });
  }

  D.tabs.overview.fill = function () {
    var d = new Date(), today = D.slot('today');
    if (today) today.textContent = 'Dev · ' + DAYS[d.getDay()] + ' ' + d.getDate() + ' ' + d.toLocaleString('en', { month: 'short' });
    var status = D.slot('status'), needs = D.slot('needs'), now = D.slot('now');
    if (status) {
      status.classList.add('dev-live');
      if (!status.previousElementSibling || !status.previousElementSibling.classList.contains('dev-live-head')) {
        var head = D.part({ type: 'row', justify: 'between', children: [{ type: 'title', text: 'Live', size: 's' }, { type: 'chip', text: 'Updates by itself', led: true, on: true }] });
        head.classList.add('dev-live-head');
        status.parentNode.insertBefore(head, status);
      }
      [['bar', 'bar', 'The bar'], ['room', 'people', 'In the bar now'], ['ci-game', 'check', 'Game deploy'], ['ci-site', 'check', 'Site deploy'], ['errors', 'check', 'Errors this week'], ['latest', 'history', 'Claude\'s latest']].forEach(function (t) {
        tile(status, t[0], { icon: t[1], label: t[2], detail: 'Checking…', value: '…', tone: 'normal' });
      });
      live(status, false);
      clearInterval(timer);
      timer = setInterval(function () { if (!document.hidden) live(D.slot('status', true), true); }, REFRESH_MS);
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
      // Questions only you can settle (open known issues that wait for your call).
      D.plan().then(function (plan) {
        plan.questions.forEach(function (q) {
          row(needs, 'q-' + q.id, { text: 'Your call: ' + q.id, icon: 'note', detail: D.plain(q.text), value: 'Question', tone: 'warn' });
        });
      }).catch(function () {});
    }
    if (now) {
      D.plan().then(function (plan) {
        var n = plan.now; now.textContent = '';
        if (!n) return now.appendChild(D.part({ type: 'text', text: 'Nothing in Now.', tone: 'dim' }));
        var done = n.pieces.filter(function (p) { return p.done; }).length;
        now.classList.add('dev-card');
        var top = D.part({ type: 'row', gap: 14 });
        if (D.ring) top.appendChild(D.ring(done, n.pieces.length));
        var words = D.part({ type: 'stack', gap: 4 });
        words.appendChild(D.part({ type: 'row', justify: 'between', children: [{ type: 'text', text: n.tag + ' · ' + n.title }, { type: 'chip', text: done + ' of ' + n.pieces.length, on: true }] }));
        var next = n.pieces.filter(function (p) { return !p.done; })[0];
        if (next) words.appendChild(D.part({ type: 'text', text: 'Next: ' + next.tag + ' ' + next.text, tone: 'dim' }));
        top.appendChild(words);
        now.appendChild(D.link(top, 'plan/' + n.tag));
        now.appendChild(D.part({ type: 'meter', level: n.pieces.length ? done / n.pieces.length : 0 }));
        var up = D.upNextList ? D.upNextList(plan.upNext).ids : [];
        if (up.length) now.appendChild(D.part({ type: 'text', text: 'After: ' + up.slice(0, 6).join(' → ') + (up.length > 6 ? ' → …' : ''), tone: 'dim' }));
        if (window.Looped && Looped.scan) Looped.scan();
      }).catch(function () { now.textContent = ''; now.appendChild(D.part({ type: 'text', text: 'The plan did not load.', tone: 'dim' })); });
    }
  };
})();
