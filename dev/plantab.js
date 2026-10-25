// Dev → Plan (PLAN.md R4.4, J5.6, J7.1, J90): the plan as places to go.
// The tab is the hub: the job in Now (tap: its page), Up next as chips (tap
// one: its page), the five chapters as cards (the first few of each with its
// verdict; tap the card: the chapter's page with all of it), Done, Parked and
// the North star with your rules on their own pages, and the box to tell
// Claude something (a note, or an idea to park; Claude reads them at the
// start of every go: node tools/dev.mjs notes).
// Pages: #plan/<chapter id> (world, site, engine, tools, backbone),
// #plan/J85 (a job: its words, verdict, what it waits for, its pieces when
// it's in Now, its spec), #plan/done, #plan/parked, #plan/star.
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
      more.addEventListener('click', function () { rows.slice(keep).forEach(function (r) { p.appendChild(r); }); more.remove(); if (D.motion) D.motion.enter(p.parentNode.parentNode); });
      var box = D.part({ type: 'stack', gap: 8 }); box.appendChild(p); box.appendChild(more); return box;
    }
    return p;
  }
  D.planUi = { verdictRow: verdictRow, wrapRows: wrapRows, folded: folded, short: short };
  function drawNotes(el, notes) {
    el.textContent = '';
    if (!notes.length) { el.textContent = 'Claude reads these at the start of the next go.'; return; }
    var list = wrapRows(notes.slice(0, 6).map(function (n) {
      return D.part({ type: 'listrow', icon: n.kind === 'park' ? 'place' : 'note', text: n.text, detail: (n.kind === 'park' ? 'To park · ' : '') + D.ago(n.at), value: n.read ? 'Read' : 'Waiting', tone: n.read ? 'ok' : 'warn' });
    }));
    el.replaceWith(list); list.setAttribute('data-id', 'notes');
  }
  // A road item's row that opens its page.
  function roadRow(it, plan) {
    var row = verdictRow(it.tag, it.text, it.verdict, it.formerly ? 'Formerly ' + it.formerly : '', { chevron: true });
    row.dataset.tag = it.tag;
    if (it.formerly) row.dataset.formerly = it.formerly;
    return D.link(row, 'plan/' + it.tag);
  }
  // "J84 → J90 → J85 (after J54 …)" → ['J84', 'J90', 'J85', …] (the order) and the note in brackets.
  function upNextList(text) {
    var head = String(text || '').split('(')[0];
    return { ids: head.match(/J\d+/g) || [], note: (String(text || '').match(/\(([^)]*)\)/) || [])[1] || '' };
  }
  D.upNextList = upNextList;
  // Up next as a row of chips you can scroll and tap.
  function chips(ids, plan) {
    var row = document.createElement('div'); row.className = 'dev-chips'; row.setAttribute('role', 'list');
    ids.forEach(function (id, i) {
      var it = findItem(plan, id), c = document.createElement('a');
      c.className = 'dev-chip'; c.href = '#plan/' + id; c.dataset.go = 'plan/' + id; c.setAttribute('role', 'listitem');
      c.style.setProperty('--i', String(Math.min(i, 14)));
      var n = document.createElement('b'); n.textContent = id; c.appendChild(n);
      if (it) { var w = document.createElement('span'); w.textContent = short(it.text)[0]; c.appendChild(w); }
      row.appendChild(c);
    });
    return row;
  }
  function findItem(plan, id) {
    if (plan.now && plan.now.tag === id) return { tag: id, text: plan.now.title, now: true, chapter: plan.now.chapter };
    for (var ch in plan.road) {
      var it = plan.road[ch].filter(function (x) { return x.tag === id || x.formerly === id; })[0];
      if (it) return Object.assign({ chapter: ch }, it);
    }
    return null;
  }
  D.findItem = findItem;

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
        var n = plan.now, done = n.pieces.filter(function (p) { return p.done; }).length;
        nowEl.appendChild(D.part({ type: 'label', text: 'Now · ' + n.chapter }));
        nowEl.appendChild(D.link(D.part({ type: 'listrow', icon: 'note', text: n.tag + ' · ' + n.title, detail: done + ' of ' + n.pieces.length + ' pieces done · tap for the whole job', chevron: true }), 'plan/' + n.tag));
        if (plan.asOf && plan.asOf.at) nowEl.appendChild(D.part({ type: 'label', text: 'As of ' + (plan.asOf.sha ? 'build ' + plan.asOf.sha + ', ' : '') + D.ago(plan.asOf.at) }));
        nowEl.appendChild(wrapRows(n.pieces.map(function (p) {
          return D.part({ type: 'listrow', icon: p.done ? 'check' : 'loop', text: p.tag + ' · ' + short(p.text)[0], detail: p.check ? 'Check: ' + p.check : short(p.text)[1], value: p.done ? 'Done' : 'Next', tone: p.done ? 'ok' : 'warn' });
        })));
      }
      if (!ch) return;
      var up = upNextList(plan.upNext);
      if (up.ids.length) {
        ch.appendChild(D.part({ type: 'title', text: 'Up next', size: 's' }));
        ch.appendChild(chips(up.ids, plan));
      }
      ch.appendChild(D.part({ type: 'title', text: 'The road', size: 's' }));
      plan.chapters.forEach(function (c) {
        var items = plan.road[c.name] || [];
        var head = D.part({ type: 'row', justify: 'between', children: [{ type: 'title', text: c.name, size: 's' }, { type: 'chip', text: items.length + ' to do' }] });
        var card = D.part({ type: 'panel', gap: 10 });
        card.classList.add('dev-card');
        card.dataset.chapter = c.name;
        card.appendChild(D.link(head, 'plan/' + c.id));
        card.appendChild(D.part({ type: 'text', text: c.now || c.what, tone: 'dim' }));
        if (items.length) card.appendChild(wrapRows(items.slice(0, 3).map(function (it) { return roadRow(it, plan); })));
        var all = D.part({ type: 'key', text: items.length > 3 ? 'All ' + items.length + ' in ' + c.name.replace(/^The /, 'the ') : 'Open ' + c.name.replace(/^The /, 'the '), style: 'small', icon: 'arrow' });
        all.dataset.go = 'plan/' + c.id; all.classList.add('dev-data');
        card.appendChild(all);
        ch.appendChild(card);
      });
      var parkedCount = Object.keys(plan.parked).reduce(function (n, k) { return n + plan.parked[k].length; }, 0);
      ch.appendChild(wrapRows([
        D.link(D.part({ type: 'listrow', icon: 'check', text: 'Done', detail: 'Every job finished, newest first', value: String(plan.done.length), chevron: true }), 'plan/done'),
        D.link(D.part({ type: 'listrow', icon: 'place', text: 'Parked', detail: 'Ideas waiting for your word', value: String(parkedCount), chevron: true }), 'plan/parked'),
        D.link(D.part({ type: 'listrow', icon: 'target', text: 'North star and your rules', detail: 'What it all serves, and the rules that always hold', chevron: true }), 'plan/star'),
      ]));
    }).catch(function () { var el = D.slot('plan-now'); if (el) el.appendChild(D.part({ type: 'text', text: 'The plan did not load.', tone: 'dim' })); });
  };

  // ---- The nested pages ----
  function chapterOf(plan, id) { return plan.chapters.filter(function (c) { return c.id === id; })[0]; }
  function loading(view) { var p = D.part({ type: 'text', text: 'Loading…', tone: 'dim' }); view.appendChild(p); return p; }
  function failed(view) { view.appendChild(D.part({ type: 'text', text: 'The plan did not load.', tone: 'dim' })); }
  var NAMES = { world: 'The world', site: 'The site', engine: 'The engine', tools: 'The tools', backbone: 'The backbone' };
  // A spec's Markdown, drawn plainly: headings, paragraphs, bullets, code.
  function specView(text) {
    var box = D.part({ type: 'panel', gap: 8 }); box.dataset.spec = '1'; box.classList.add('dev-spec');
    String(text).split(/\n{2,}/).forEach(function (block) {
      block = block.trim(); if (!block) return;
      var h = /^(#{1,4})\s+(.*)$/.exec(block.split('\n')[0]);
      if (h && block.indexOf('\n') < 0) { box.appendChild(D.part({ type: 'title', text: D.plain(h[2]), size: h[1].length <= 2 ? 'm' : 's' })); return; }
      if (/^```/.test(block)) { var pre = document.createElement('pre'); pre.className = 'dev-code'; pre.textContent = block.replace(/^```\w*\n?|```$/g, ''); box.appendChild(pre); return; }
      if (/^[-*] /.test(block)) {
        var ul = document.createElement('ul'); ul.className = 'dev-bullets';
        block.split(/\n(?=[-*] )/).forEach(function (li) { var l = document.createElement('li'); l.textContent = D.plain(li.replace(/^[-*] /, '').replace(/\s*\n\s*/g, ' ')); ul.appendChild(l); });
        box.appendChild(ul); return;
      }
      box.appendChild(D.part({ type: 'text', text: D.plain(block.replace(/^#+ /, '').replace(/\s*\n\s*/g, ' ')), tone: 'dim' }));
    });
    return box;
  }
  // A progress ring (dev.css): --p from 0 to 1.
  function ring(done, total) {
    var r = document.createElement('div'); r.className = 'dev-ring';
    r.style.setProperty('--p', String(total ? done / total : 0));
    var b = document.createElement('b'); b.textContent = done + '/' + total; r.appendChild(b);
    return r;
  }
  D.ring = ring;
  D.pages.plan = [
    { match: /^(world|site|engine|tools|backbone)$/, title: function (m) { return NAMES[m[1]]; },
      draw: function (view, m) {
        var l = loading(view);
        return D.plan().then(function (plan) {
          l.remove();
          var c = chapterOf(plan, m[1]), items = plan.road[c.name] || [], parked = plan.parked[c.name] || [];
          view.appendChild(D.part({ type: 'stack', gap: 2, children: [{ type: 'label', text: 'Chapter' }, { type: 'title', text: c.name, size: 'l' }] }));
          var card = D.part({ type: 'panel', gap: 8, children: [{ type: 'text', text: c.what, tone: 'dim' }, { type: 'text', text: 'Now: ' + c.now }] });
          card.classList.add('dev-card'); card.dataset.chapter = c.name;
          view.appendChild(card);
          view.appendChild(D.part({ type: 'title', text: items.length + ' on the road', size: 's' }));
          var road = wrapRows(items.map(function (it) { return roadRow(it, plan); }));
          road.dataset.chapter = c.name;
          view.appendChild(road);
          if (parked.length) {
            view.appendChild(D.part({ type: 'title', text: 'Parked here', size: 's' }));
            var pk = wrapRows(parked.map(function (t) { var row = verdictRow('', t.text, t.verdict, 'Parked', { icon: 'place' }); row.dataset.parked = '1'; return row; }));
            pk.dataset.chapter = c.name;
            view.appendChild(pk);
          }
        }).catch(function () { l.remove(); failed(view); });
      } },
    { match: /^(J\d+)$/, title: function (m) { return m[1]; },
      crumbs: function () { return []; },
      draw: function (view, m) {
        var l = loading(view);
        return D.plan().then(function (plan) {
          l.remove();
          var id = m[1], it = findItem(plan, id), spec = (plan.specs || {})[id];
          var doneLine = plan.done.filter(function (d) { return new RegExp('\\b' + id + '(\\.\\d+)?\\b').test(d.text); })[0];
          var head = D.part({ type: 'stack', gap: 4 });
          head.appendChild(D.part({ type: 'label', text: it ? (it.now ? 'Now · ' + it.chapter : it.chapter) : doneLine ? 'Done' : 'Job' }));
          head.appendChild(D.part({ type: 'title', text: id + (it ? ' · ' + short(it.text)[0] : ''), size: 'l' }));
          view.appendChild(head);
          var card = D.part({ type: 'panel', gap: 10 }); card.classList.add('dev-card');
          if (it) {
            var chipsRow = D.part({ type: 'row', gap: 6, wrap: true });
            if (it.now) chipsRow.appendChild(D.part({ type: 'chip', text: 'In progress', on: true, led: true }));
            if (it.verdict) chipsRow.appendChild(D.part({ type: 'chip', text: it.verdict.word }));
            if (it.formerly) chipsRow.appendChild(D.part({ type: 'chip', text: 'Formerly ' + it.formerly }));
            (it.serves || []).forEach(function (s) { chipsRow.appendChild(D.part({ type: 'chip', text: 'Serves ' + s })); });
            card.appendChild(chipsRow);
            if (!it.now) card.appendChild(D.part({ type: 'text', text: it.text }));
            if (it.verdict) card.appendChild(D.part({ type: 'text', text: it.verdict.line, tone: 'dim' }));
            var pos = upNextList(plan.upNext).ids.indexOf(id);
            if (pos >= 0) card.appendChild(D.part({ type: 'text', text: pos === 0 ? 'Next in line.' : 'Number ' + (pos + 1) + ' in line.', tone: 'dim' }));
          } else if (doneLine) {
            card.appendChild(D.part({ type: 'text', text: doneLine.text }));
            card.appendChild(D.part({ type: 'text', text: doneLine.sha === 'this push' ? 'Going live now' : 'Build ' + doneLine.sha, tone: 'dim' }));
          } else card.appendChild(D.part({ type: 'text', text: 'Not on the plan any more.', tone: 'dim' }));
          view.appendChild(card);
          var waits = it && it.after ? it.after.match(/J\d+/g) : null;
          if (waits) {
            view.appendChild(D.part({ type: 'title', text: 'Waits for', size: 's' }));
            view.appendChild(chips(waits, plan));
          }
          if (it && it.now && plan.now) {
            var n = plan.now, done = n.pieces.filter(function (p) { return p.done; }).length;
            var pr = D.part({ type: 'row', gap: 14 }); pr.appendChild(ring(done, n.pieces.length));
            pr.appendChild(D.part({ type: 'text', text: done === n.pieces.length ? 'Every piece is done: it ships next.' : (n.pieces.length - done) + ' piece' + (n.pieces.length - done === 1 ? '' : 's') + ' to go.' }));
            view.appendChild(pr);
            view.appendChild(wrapRows(n.pieces.map(function (p) {
              return D.part({ type: 'listrow', icon: p.done ? 'check' : 'loop', text: p.tag + ' · ' + short(p.text)[0], detail: p.check ? 'Check: ' + p.check : short(p.text)[1], value: p.done ? 'Done' : 'Next', tone: p.done ? 'ok' : 'warn' });
            })));
          }
          if (spec) { view.appendChild(D.part({ type: 'title', text: 'Its plan', size: 's' })); view.appendChild(specView(spec)); }
        }).catch(function () { l.remove(); failed(view); });
      } },
    { match: /^done$/, title: function () { return 'Done'; },
      draw: function (view) {
        var l = loading(view);
        return D.plan().then(function (plan) {
          l.remove();
          view.appendChild(D.part({ type: 'stack', gap: 2, children: [{ type: 'label', text: plan.done.length + ' jobs' }, { type: 'title', text: 'Done', size: 'l' }] }));
          view.appendChild(folded(plan.done.map(function (d) {
            var tag = (d.text.match(/^(J\d+)/) || [])[1], row = itemRow('', d.text, { detail: d.sha === 'this push' ? 'going live now' : 'build ' + d.sha, icon: 'check', chevron: !!tag });
            return tag ? D.link(row, 'plan/' + tag) : row;
          }), 12));
        }).catch(function () { l.remove(); failed(view); });
      } },
    { match: /^parked$/, title: function () { return 'Parked'; },
      draw: function (view) {
        var l = loading(view);
        return D.plan().then(function (plan) {
          l.remove();
          view.appendChild(D.part({ type: 'stack', gap: 2, children: [{ type: 'label', text: 'Ideas waiting for your word' }, { type: 'title', text: 'Parked', size: 'l' }] }));
          plan.chapters.forEach(function (c) {
            var list = plan.parked[c.name] || [];
            if (!list.length) return;
            view.appendChild(D.part({ type: 'title', text: c.name, size: 's' }));
            var p = wrapRows(list.map(function (t) { var row = verdictRow('', t.text, t.verdict, '', { icon: 'place' }); row.dataset.parked = '1'; return row; }));
            p.dataset.chapter = c.name;
            view.appendChild(p);
          });
        }).catch(function () { l.remove(); failed(view); });
      } },
    { match: /^star$/, title: function () { return 'North star'; },
      draw: function (view) {
        var l = loading(view);
        return D.plan().then(function (plan) {
          l.remove();
          view.appendChild(D.part({ type: 'stack', gap: 2, children: [{ type: 'label', text: 'What it all serves' }, { type: 'title', text: 'North star', size: 'l' }] }));
          view.appendChild(wrapRows(plan.north.map(function (s) { return D.part({ type: 'listrow', text: s.id + ' · ' + s.title, detail: s.text }); })));
          view.appendChild(D.part({ type: 'title', text: 'Your rules', size: 's' }));
          view.appendChild(wrapRows(plan.rules.map(function (r) { return D.part({ type: 'listrow', text: r.replace(/^\(\d{4}-\d\d-\d\d\) /, ''), detail: (/^\((\d{4}-\d\d-\d\d)\)/.exec(r) || [])[1] || '' }); })));
        }).catch(function () { l.remove(); failed(view); });
      } },
  ];
})();
