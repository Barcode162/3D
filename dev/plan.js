// The plan in the browser (Dev's Overview and Plan tabs): its data, the same
// files tools/plan.mjs writes (docs/plan/plan.json, docs/charter/rules.json),
// never PLAN.md's text. Dev.plan() fetches it once per visit (dev/api.php →
// private/dev/, put there by the site deploy).
(function () {
  'use strict';
  var D = window.Dev;
  var plain = function (s) { return String(s || '').replace(/\*\*/g, '').replace(/`/g, '').replace(/\*([^*]+)\*/g, '$1').trim(); };
  D.plain = plain;
  // The plan's data (docs/plan/plan.json and docs/charter/rules.json, written
  // only by tools/plan.mjs) in the shape the tabs draw.
  D.planFromData = function (p, r) {
    var names = {}; p.chapters.forEach(function (c) { names[c.id] = c.name; });
    var byName = function (map, fn) { var out = {}; Object.keys(map).forEach(function (id) { out[names[id]] = map[id].map(fn); }); return out; };
    var n = p.now;
    return {
      north: r.north.map(function (s) { return { id: s.id, title: s.title, text: plain(s.text).replace(/\s*\n\s*/g, ' ') }; }),  // whole lines (the old reader cut them at the first line break)
      rules: r.rules.map(function (x) { return plain('(' + x.date + ') ' + x.text); }),
      chapters: p.chapters.map(function (c) { return { name: c.name, what: plain(c.what), now: plain(c.now) }; }),
      now: n ? { tag: n.id, title: n.title, chapter: n.chapter, about: plain(n.about),
        pieces: n.pieces.map(function (x) { return { tag: x.id, done: x.done, text: plain(x.text), check: plain(x.check) }; }) } : null,
      road: byName(p.road, function (x) { return { tag: x.id, done: x.done, text: plain(x.text), serves: x.serves }; }),
      upNext: plain(p.upNext),
      parked: byName(p.parked, plain),
      done: p.done.map(function (d) { return { sha: d.build || 'this push', text: plain(d.text) }; }),
    };
  };
  var cached = null;
  D.plan = function () {
    if (cached) return cached;
    cached = (D.demo ? Promise.all([D.get('/docs/plan/plan.json'), D.get('/docs/charter/rules.json')]).then(function (a) { return { data: { plan: a[0], rules: a[1] } }; })
                     : D.post('/dev/api.php', { a: 'plan' }))
      .then(function (j) { if (!j.data || !j.data.plan || !j.data.rules) throw new Error(j.error || 'No plan yet'); return D.planFromData(j.data.plan, j.data.rules); })
      .catch(function (e) { cached = null; throw e; });
    return cached;
  };
})();
