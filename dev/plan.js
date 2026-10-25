// The plan in the browser (Dev's Overview and Plan tabs): its data, the same
// files tools/plan.mjs writes (docs/plan/plan.json, docs/charter/rules.json),
// never PLAN.md's text. Dev.plan() fetches it once per visit (dev/api.php →
// private/dev/, put there by .github/workflows/plan.yml within about a minute
// of a push), with the build it came from (asOf: {sha, at}), the incidents,
// the questions only the user can settle and the jobs' specs.
(function () {
  'use strict';
  var D = window.Dev;
  var plain = function (s) { return String(s || '').replace(/\*\*/g, '').replace(/`/g, '').replace(/\*([^*]+)\*/g, '$1').trim(); };
  D.plain = plain;
  // An item's verdict (J7.1, docs/playbook/intake.md step 3): a short word for
  // the row's value, its colour, and the whole line.
  var VERDICT = { now: ['Doable', 'ok'], after: ['After', 'normal'], cost: ['At a cost', 'warn'], experiment: ['Try first', 'warn'], impossible: ['Not possible', 'bad'], rule: ['Against a rule', 'bad'] };
  var LONG = { now: 'Doable now', after: 'Doable after', cost: 'Doable at a cost', experiment: 'Needs an experiment', impossible: 'Not possible', rule: 'Against a rule' };
  var verdictOf = function (v) {
    if (!v || !VERDICT[v.kind]) return null;
    return { kind: v.kind, word: v.kind === 'after' ? 'After ' + v.after : VERDICT[v.kind][0], tone: VERDICT[v.kind][1], line: LONG[v.kind] + (v.after ? ' ' + v.after : '') + ': ' + plain(v.why) };
  };
  // The plan's data (docs/plan/plan.json and docs/charter/rules.json, written
  // only by tools/plan.mjs) in the shape the tabs draw.
  D.planFromData = function (p, r) {
    var names = {}; p.chapters.forEach(function (c) { names[c.id] = c.name; });
    var byName = function (map, fn) { var out = {}; Object.keys(map).forEach(function (id) { out[names[id]] = map[id].map(fn); }); return out; };
    var n = p.now;
    return {
      north: r.north.map(function (s) { return { id: s.id, title: s.title, text: plain(s.text).replace(/\s*\n\s*/g, ' ') }; }),  // whole lines (the old reader cut them at the first line break)
      rules: r.rules.map(function (x) { return plain('(' + x.date + ') ' + x.text); }),
      chapters: p.chapters.map(function (c) { return { id: c.id, name: c.name, what: plain(c.what), now: plain(c.now) }; }),
      now: n ? { tag: n.id, title: n.title, chapter: n.chapter, about: plain(n.about),
        pieces: n.pieces.map(function (x) { return { tag: x.id, done: x.done, text: plain(x.text), check: plain(x.check) }; }) } : null,
      road: byName(p.road, function (x) { return { tag: x.id, formerly: x.formerly || '', done: x.done, text: plain(x.text), serves: x.serves, verdict: verdictOf(x.verdict), after: (x.verdict && x.verdict.after) || '' }; }),
      upNext: plain(p.upNext),
      parked: byName(p.parked, function (x) { return typeof x === 'string' ? { text: plain(x), verdict: null } : { text: plain(x.text), verdict: verdictOf(x.verdict) }; }),
      done: p.done.map(function (d) { return { sha: d.build || 'this push', text: plain(d.text) }; }),
    };
  };
  var cached = null;
  D.plan = function (fresh) {
    if (cached && !fresh) return cached;
    cached = (D.demo ? Promise.all([D.get('/docs/plan/plan.json'), D.get('/docs/charter/rules.json')]).then(function (a) { return { data: { plan: a[0], rules: a[1] } }; })
                     : D.post('/dev/api.php', { a: 'plan' }))
      .then(function (j) {
        if (!j.data || !j.data.plan || !j.data.rules) throw new Error(j.error || 'No plan yet');
        var plan = D.planFromData(j.data.plan, j.data.rules);
        plan.asOf = { sha: j.sha || '', at: j.at || 0 };
        plan.incidents = j.incidents || [];
        plan.questions = j.questions || [];
        plan.specs = j.specs || {};
        return plan;
      })
      .catch(function (e) { cached = null; throw e; });
    return cached;
  };
})();
