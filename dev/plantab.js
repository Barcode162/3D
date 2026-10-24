// Dev → Plan (PLAN.md R4.4, J5.6, J7.1): the plan on the phone (Now with its ticks,
// the road by chapter with each item's verdict, Parked, Done, the North star and your rules; tap a job
// that has a spec, in Now or on the road, to read it) and the box
// to tell Claude something: a note, or an idea to park. Claude reads them at
// the start of every go (node tools/dev.mjs notes).
(function () {
  'use strict';
  var D = window.Dev;
  var short = function (t) { var i = t.indexOf(': '); return i > 0 && i < 60 ? [t.slice(0, i), t.slice(i + 2)] : [t.length > 60 ? t.slice(0, 57) + '…' : t, t.length > 60 ? t : '']; };
  function itemRow(tag, text, extra) {
    var s = short(text);
    return D.part(Object.assign({ type: 'listrow', text: (tag ? tag + ' · ' : '') + s[0], detail: s[1] }, extra || {}));
  }
  // A road or parked row with its verdict: the short word as its value, the
  // reason under the words (J7.1).
  function verdictRow(tag, text, verdict, note, extra) {
    var s = short(text), v = verdict;
    var row = D.part(Object.assign({ type: 'listrow', text: (tag ? tag + ' · ' : '') + s[0], detail: [s[1], v ? v.line : '', note].filter(Boolean).join(' · '),
      value: v ? v.word : '', tone: v ? v.tone : 'normal' }, extra || {}));
    if (v) row.dataset.verdict = v.kind;
    return row;
  }
  function wrapRows(rows) { var p = D.part({ type: 'panel', pad: 0, gap: 0 }); rows.forEach(function (r) { p.appendChild(r); }); return p; }
  // A long list folds: the first few, then "Show all".
  function folded(rows, keep) {
    var p = wrapRows(rows.slice(0, keep));
    if (rows.length > keep) {
      var more = D.part({ type: 'key', text: 'Show all ' + rows.length, style: 'small' });
      more.addEventListener('click', function () { rows.slice(keep).forEach(function (r) { p.appendChild(r); }); more.remove(); });
      var box = D.part({ type: 'stack', gap: 8 }); box.appendChild(p); box.appendChild(more); return box;
    }
    return p;
  }
  function drawNotes(el, notes) {
    el.textContent = '';
    if (!notes.length) { el.textContent = 'Claude reads these at the start of the next go.'; return; }
    var list = wrapRows(notes.slice(0, 6).map(function (n) {
      return D.part({ type: 'listrow', icon: n.kind === 'park' ? 'place' : 'note', text: n.text, detail: (n.kind === 'park' ? 'To park · ' : '') + D.ago(n.at), value: n.read ? 'Read' : 'Waiting', tone: n.read ? 'ok' : 'warn' });
    }));
    el.replaceWith(list); list.setAttribute('data-id', 'notes');
  }
  // A job's spec, folded under its row: tap to open, tap again to close.
  function withSpec(rowEl, spec) {
    if (!spec) return rowEl;
    var box = D.part({ type: 'stack', gap: 0 }), btn = document.createElement('button');
    btn.type = 'button'; btn.className = 'dev-row-btn'; btn.setAttribute('aria-expanded', 'false'); btn.appendChild(rowEl);
    var body = null;
    btn.addEventListener('click', function () {
      if (body) { body.remove(); body = null; btn.setAttribute('aria-expanded', 'false'); return; }
      body = D.part({ type: 'panel', gap: 6 });
      body.dataset.spec = '1';
      spec.split(/\n{2,}/).forEach(function (para) { body.appendChild(D.part({ type: 'text', text: D.plain(para.replace(/^#+ /, '').replace(/\s*\n\s*/g, ' ')), tone: 'dim' })); });
      box.appendChild(body); btn.setAttribute('aria-expanded', 'true');
    });
    box.appendChild(btn);
    return box;
  }
  D.tabs.plan.fill = function () {
    var kind = 'note';
    var seg = D.slot('note-kind', true);
    if (seg) [].forEach.call(seg.querySelectorAll('button'), function (b, i) {
      b.addEventListener('click', function () { kind = i ? 'park' : 'note'; [].forEach.call(seg.querySelectorAll('button'), function (x, j) { x.classList.toggle('on', j === i); }); });
    });
    var field = D.slot('note', true), send = D.slot('note-send', true), notesEl = D.slot('notes', true);
    function loadNotes() { if (D.demo) return; D.post('/dev/api.php', { a: 'notes' }).then(function (j) { var el = D.slot('notes', true); if (j.ok && el) drawNotes(el, j.notes); }); }
    if (send) send.addEventListener('click', function () {
      var text = (field.value || '').trim();
      if (!text) { D.toast('Write something first.'); return; }
      send.disabled = true;
      D.post('/dev/api.php', { a: 'note', kind: kind, text: text }).then(function (j) {
        send.disabled = false;
        if (!j.ok) { D.toast(j.error || 'That didn\'t send.'); return; }
        field.value = ''; D.toast(kind === 'park' ? 'Parked: Claude adds it to the plan at the next go.' : 'Sent: Claude reads it at the next go.'); loadNotes();
      });
    });
    if (notesEl) loadNotes();
    return D.plan().then(function (plan) {
      var nowEl = D.slot('plan-now'), ch = D.slot('chapters');
      if (nowEl && plan.now) {
        var n = plan.now;
        nowEl.appendChild(D.part({ type: 'label', text: 'Now · ' + n.chapter }));
        var specs = plan.specs || {};
        nowEl.appendChild(specs[n.tag] ? withSpec(D.part({ type: 'listrow', icon: 'note', text: n.tag + ' · ' + n.title, detail: 'Tap for its spec', chevron: true }), specs[n.tag])
                                       : D.part({ type: 'title', text: n.tag + ' · ' + n.title, size: 's' }));
        if (plan.asOf && plan.asOf.at) nowEl.appendChild(D.part({ type: 'label', text: 'As of ' + (plan.asOf.sha ? 'build ' + plan.asOf.sha + ', ' : '') + D.ago(plan.asOf.at) }));
        nowEl.appendChild(wrapRows(n.pieces.map(function (p) {
          return D.part({ type: 'listrow', icon: p.done ? 'check' : 'loop', text: p.tag + ' · ' + short(p.text)[0], detail: p.check ? 'Check: ' + p.check : short(p.text)[1], value: p.done ? 'Done' : 'Next', tone: p.done ? 'ok' : 'warn' });
        })));
      }
      if (!ch) return;
      ch.appendChild(D.part({ type: 'title', text: 'The road', size: 's' }));
      if (plan.upNext) ch.appendChild(D.part({ type: 'text', text: 'Up next: ' + plan.upNext, tone: 'dim' }));
      plan.chapters.forEach(function (c) {
        var items = plan.road[c.name] || [];
        var card = D.part({ type: 'panel', gap: 10, children: [
          { type: 'row', justify: 'between', children: [{ type: 'title', text: c.name, size: 's' }, { type: 'chip', text: items.length + ' to do' }] },
          { type: 'text', text: c.now || c.what, tone: 'dim' },
        ] });
        card.dataset.chapter = c.name;
        if (items.length) card.appendChild(folded(items.map(function (it) {
          var s = (plan.specs || {})[it.tag], row = verdictRow(it.tag, it.text, it.verdict, it.formerly ? 'Formerly ' + it.formerly : '', s ? { chevron: true } : null);
          row.dataset.tag = it.tag;
          if (it.formerly) row.dataset.formerly = it.formerly;
          return withSpec(row, s);
        }), 4));
        var parked = plan.parked[c.name] || [];
        if (parked.length) card.appendChild(folded(parked.map(function (t) { var row = verdictRow('', t.text, t.verdict, 'Parked', { icon: 'place' }); row.dataset.parked = '1'; return row; }), 2));
        ch.appendChild(card);
      });
      ch.appendChild(D.part({ type: 'title', text: 'Done', size: 's' }));
      ch.appendChild(folded(plan.done.map(function (d) { return itemRow('', d.text, { detail: d.sha === 'this push' ? 'going live now' : 'build ' + d.sha, icon: 'check' }); }), 5));
      ch.appendChild(D.part({ type: 'title', text: 'North star', size: 's' }));
      ch.appendChild(wrapRows(plan.north.map(function (s) { return D.part({ type: 'listrow', text: s.id + ' · ' + s.title, detail: s.text }); })));
      ch.appendChild(D.part({ type: 'title', text: 'Your rules', size: 's' }));
      ch.appendChild(folded(plan.rules.map(function (r) { return D.part({ type: 'listrow', text: r.replace(/^\(\d{4}-\d\d-\d\d\) /, ''), detail: (/^\((\d{4}-\d\d-\d\d)\)/.exec(r) || [])[1] || '' }); }), 4));
    }).catch(function () { var el = D.slot('plan-now'); if (el) el.appendChild(D.part({ type: 'text', text: 'The plan did not load.', tone: 'dim' })); });
  };
})();
