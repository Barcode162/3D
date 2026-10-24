// PLAN.md read in the browser (Dev's Overview and Plan tabs): the same eight
// parts tools/plan.mjs keeps, turned into plain data. Dev.plan() fetches it
// once per visit (dev/api.php → private/dev/plan.md, put there by CI).
(function () {
  'use strict';
  var D = window.Dev;
  var plain = function (s) { return String(s || '').replace(/\*\*/g, '').replace(/`/g, '').replace(/\*([^*]+)\*/g, '$1').trim(); };
  D.plain = plain;
  D.parsePlan = function (md) {
    var parts = {}, re = /^## (\d)\. (.*)$/gm, m, marks = [];
    while ((m = re.exec(md))) marks.push({ n: +m[1], at: m.index, body: m.index + m[0].length });
    marks.forEach(function (k, i) { parts[k.n] = md.slice(k.body, i + 1 < marks.length ? marks[i + 1].at : md.length); });
    var lines = function (s) { return String(s || '').split('\n'); };
    var chaptersOf = function (body) {
      var out = {}, cur = null;
      lines(body).forEach(function (l) { var h = /^### (.*)$/.exec(l); if (h) { cur = h[1]; out[cur] = []; } else if (cur && /^- /.test(l)) out[cur].push(l.slice(2)); });
      return out;
    };
    var plan = { north: [], rules: [], chapters: [], now: null, road: {}, upNext: '', parked: {}, done: [] };
    lines(parts[1]).forEach(function (l) { var x = /^- \*\*(N\d+) (.+?)\*\* ?(.*)$/.exec(l); if (x) plan.north.push({ id: x[1], title: x[2], text: plain(x[3]) }); });
    lines(parts[2]).forEach(function (l) { if (/^- /.test(l)) plan.rules.push(plain(l.slice(2))); });
    lines(parts[3]).forEach(function (l) { var x = /^- \*\*(The [a-z]+):\*\* (.*?)(?: \*Now:\* (.*))?$/.exec(l); if (x) plan.chapters.push({ name: x[1], what: plain(x[2]), now: plain(x[3]) }); });
    var now = parts[4] || '', head = /^\*\*([A-Z]+\d+) · ([^*]+)\*\* \(([^)]+)\)\. ?(.*)$/m.exec(now);
    if (head) {
      plan.now = { tag: head[1], title: head[2], chapter: head[3], about: plain(head[4]).replace(/ · N\d+( N\d+)*$/, ''), pieces: [] };
      lines(now).forEach(function (l) {
        var p = /^- \[( |x)\] \*\*([A-Z]+\d+(?:\.\d+)?)\*\* (.*?)(?: \*Check:\* (.*))?$/.exec(l);
        if (p) plan.now.pieces.push({ tag: p[2], done: p[1] === 'x', text: plain(p[3]), check: plain(p[4]) });
      });
    }
    var road = parts[5] || '', up = /\*\*Up next, in order:\*\* (.*)/.exec(road);
    plan.upNext = up ? plain(up[1]) : '';
    var r = chaptersOf(road);
    Object.keys(r).forEach(function (c) {
      plan.road[c] = r[c].map(function (l) {
        var x = /^\[( |x)\] \*\*([^*]+)\*\* (.*) · (N\d+(?: N\d+)*)$/.exec(l);
        return x ? { tag: x[2], done: x[1] === 'x', text: plain(x[3]), serves: x[4].split(' ') } : { tag: '', text: plain(l), serves: [] };
      });
    });
    var pk = chaptersOf(parts[6]);
    Object.keys(pk).forEach(function (c) { plan.parked[c] = pk[c].map(plain); });
    lines(parts[7]).forEach(function (l) { var x = /^- (?:`([0-9a-f]{7})`|\(this push\)) (.*)$/.exec(l); if (x) plan.done.push({ sha: x[1] || 'this push', text: plain(x[2]) }); });
    return plan;
  };
  var cached = null;
  D.plan = function () {
    if (cached) return cached;
    cached = (D.demo ? D.text('/PLAN.md').then(function (t) { return { plan: t }; }) : D.post('/dev/api.php', { a: 'plan' }))
      .then(function (j) { if (!j.plan) throw new Error(j.error || 'No plan yet'); return D.parsePlan(j.plan); })
      .catch(function (e) { cached = null; throw e; });
    return cached;
  };
})();
