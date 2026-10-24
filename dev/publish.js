// Dev → Publish (PLAN.md R4.5): the news posts Claude writes after each step
// (the hub's store, hub/hub.php), published to joshuavelasco.com/news in one
// tap; and the site's words for the front door (Dev's store, read by R5).
(function () {
  'use strict';
  var D = window.Dev, HUB = '/hub/hub.php';
  function newsCard(p, redraw) {
    var card = D.part({ type: 'panel', gap: 10, children: [
      { type: 'row', justify: 'between', children: [{ type: 'title', text: D.plain(p.title), size: 's' }, { type: 'chip', text: p.published ? 'Public' : 'Draft', on: p.published, led: p.published }] },
      { type: 'label', text: (p.build ? 'build ' + p.build + ' · ' : '') + D.ago(p.at) },
    ] });
    card.dataset.post = p.id;
    var body = D.part({ type: 'text', text: p.text, tone: 'dim' }); body.classList.add('dev-post-text'); card.appendChild(body);
    (p.shots || []).slice(0, 4).forEach(function (s) { card.appendChild(D.part({ type: 'image', src: '/hub/' + s.url, ratio: '16:9' })); });
    var row = D.part({ type: 'row' });
    var key = D.part({ type: 'key', text: p.published ? 'Unpublish' : 'Publish', style: p.published ? 'small' : 'go' });
    key.dataset.act = 'publish';
    key.addEventListener('click', function () {
      key.disabled = true;
      D.post(HUB, { a: 'publish', id: p.id, on: p.published ? 0 : 1 }).then(function (j) {
        if (!j.ok) { key.disabled = false; D.toast(j.error || 'That didn\'t work.'); return; }
        D.toast(j.post.published ? 'Published: it\'s on joshuavelasco.com/news' : 'Taken off the public news'); redraw();
      });
    });
    row.appendChild(key);
    if (p.published) { var a = document.createElement('a'); a.className = 'key small'; a.href = '/news/'; a.textContent = 'See it'; row.appendChild(a); }
    card.appendChild(row);
    return card;
  }
  function drawNews() {
    var slot = D.slot('posts'); if (!slot) return;
    slot.appendChild(D.part({ type: 'text', text: 'Loading the news…', tone: 'dim' }));
    (D.demo ? Promise.resolve({ ok: 1, posts: [] }) : D.post(HUB, { a: 'posts' })).then(function (j) {
      slot = D.slot('posts'); if (!slot) return;
      if (!j.ok) { slot.appendChild(D.part({ type: 'text', text: j.status === 401 || j.status === 403 ? 'Only the owner publishes news.' : 'The news did not load.', tone: 'dim' })); return; }
      if (!j.posts.length) { slot.appendChild(D.part({ type: 'text', text: 'No posts yet. Claude writes one after every step.', tone: 'dim' })); return; }
      j.posts.forEach(function (p) { slot.appendChild(newsCard(p, drawNews)); });
    });
  }
  var FIELDS = [['headline', 'Headline', 'The lights are on.'], ['lede', 'One line under it', 'A bar you can walk into, built from scratch.'],
    ['news', 'The news highlight', 'New: real light that bounces'], ['newsLink', 'Where the highlight goes (a link)', '/news/']];
  function drawSite() {
    var slot = D.slot('posts'); if (!slot) return;
    var form = D.part({ type: 'panel', gap: 10, children: [{ type: 'title', text: 'The front page', size: 's' }, { type: 'text', text: 'The few words joshuavelasco.com shows (the new front door, R5, reads them).', tone: 'dim' }] });
    var inputs = {};
    FIELDS.forEach(function (f) {
      form.appendChild(D.part({ type: 'label', text: f[1] }));
      var i = D.part({ type: 'field', placeholder: f[2] }); i.dataset.field = f[0]; inputs[f[0]] = i; form.appendChild(i);
    });
    var save = D.part({ type: 'key', text: 'Save the words', style: 'go' }), when = D.part({ type: 'text', text: '', tone: 'dim' });
    form.appendChild(save); form.appendChild(when); slot.appendChild(form);
    if (!D.demo) D.get('/dev/api.php?site').then(function (j) {
      var s = j.site || {};
      FIELDS.forEach(function (f) { inputs[f[0]].value = s[f[0]] || ''; });
      if (s.at) when.textContent = 'Last saved ' + D.ago(s.at) + ' by ' + s.by + '.';
    }).catch(function () {});
    save.addEventListener('click', function () {
      var site = {}; FIELDS.forEach(function (f) { site[f[0]] = inputs[f[0]].value.trim(); });
      save.disabled = true;
      D.post('/dev/api.php', { a: 'site', site: site }).then(function (j) {
        save.disabled = false;
        if (!j.ok) { D.toast(j.error || 'That didn\'t save.'); return; }
        when.textContent = 'Saved just now.'; D.toast('Saved');
      });
    });
  }
  D.tabs.publish.fill = function () {
    var seg = D.slot('pub-tab', true), which = 0;
    function show(i) {
      which = i;
      if (seg) [].forEach.call(seg.querySelectorAll('button'), function (b, j) { b.classList.toggle('on', j === i); });
      D.slot('posts'); if (i) drawSite(); else drawNews();
    }
    if (seg) [].forEach.call(seg.querySelectorAll('button'), function (b, i) { b.addEventListener('click', function () { if (i !== which) show(i); }); });
    show(0);
  };
})();
