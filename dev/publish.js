// Dev → Publish (PLAN.md R4.5): the news posts Claude writes after each step
// (the hub's store, hub/hub.php), published to joshuavelasco.com/news in one
// tap; highlights for the front page (a picture and a few friendly words,
// pushed when you want, R5.2); and the site's words (Dev's store, read by R5).
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
  var FIELDS = [['headline', 'Headline', 'The lights are on.'], ['lede', 'One line under it', 'A bar you can walk into, built from scratch.']];

  // ---- Highlights: what the front page shows (a picture and a few friendly words), pushed when you want ----
  var PICS = ['hero', 'counter', 'backbar', 'jukebox', 'stage', 'arcade', 'chess', 'rhythm', 'mirror', 'rigs', 'wide'];
  function hlCard(h) {
    var c = document.createElement('div'); c.className = 'dev-hl';
    if (h.img) c.style.backgroundImage = 'linear-gradient(180deg, rgba(0,0,0,0) 30%, rgba(0,0,0,.78)), url("' + h.img + '")';
    var t = document.createElement('b'); t.textContent = h.title || 'Your title'; c.appendChild(t);
    if (h.text) { var p = document.createElement('span'); p.textContent = h.text; c.appendChild(p); }
    return c;
  }
  // A phone photo made small enough to send (JPEG, 1200 px wide at most).
  function shrink(file) {
    return new Promise(function (ok, bad) {
      var img = new Image(), url = URL.createObjectURL(file);
      img.onload = function () {
        var w = Math.min(1200, img.naturalWidth), h = Math.round(img.naturalHeight * w / img.naturalWidth), cv = document.createElement('canvas');
        cv.width = w; cv.height = h; cv.getContext('2d').drawImage(img, 0, 0, w, h); URL.revokeObjectURL(url);
        var q = 0.85, out = cv.toDataURL('image/jpeg', q);
        while (out.length > 780000 && q > 0.4) { q -= 0.1; out = cv.toDataURL('image/jpeg', q); }
        ok(out);
      };
      img.onerror = function () { bad(new Error('That file is not a picture.')); };
      img.src = url;
    });
  }
  function hlEditor(slot, h, done) {
    h = h || { title: '', text: '', link: '/bar/', img: '' };
    var state = { pic: /^\/site\/img\//.test(h.img) ? h.img : '', image: null, preview: h.img };
    var form = D.part({ type: 'panel', gap: 10, children: [{ type: 'title', text: h.id ? 'Change the highlight' : 'A new highlight', size: 's' },
      { type: 'text', text: 'A picture and a few friendly words: what people would want to know, no technical talk.', tone: 'dim' }] });
    var preview = hlCard(h); form.appendChild(preview);
    var fields = {};
    [['title', 'Title (short)', 'Live music on Fridays'], ['text', 'One or two friendly lines', 'Pull up a stool: the band starts at nine.'], ['link', 'Where a tap goes', '/bar/']].forEach(function (f) {
      form.appendChild(D.part({ type: 'label', text: f[1] }));
      var i = D.part({ type: 'field', placeholder: f[2] }); i.value = h[f[0]] || ''; i.dataset.field = f[0]; fields[f[0]] = i; form.appendChild(i);
      i.addEventListener('input', redraw);
    });
    form.appendChild(D.part({ type: 'label', text: 'The picture: one of the bar\'s, or your own' }));
    var pics = document.createElement('div'); pics.className = 'dev-pics';
    PICS.forEach(function (n) {
      var b = document.createElement('button'); b.type = 'button'; b.style.backgroundImage = 'url("/site/img/' + n + '.jpg")'; b.setAttribute('aria-label', n);
      b.dataset.pic = '/site/img/' + n + '.jpg';
      b.addEventListener('click', function () { state.pic = b.dataset.pic; state.image = null; state.preview = state.pic; redraw(); });
      pics.appendChild(b);
    });
    var up = document.createElement('label'); up.className = 'dev-up key small'; up.textContent = 'Your photo';
    var file = document.createElement('input'); file.type = 'file'; file.accept = 'image/*'; file.hidden = true; up.appendChild(file);
    file.addEventListener('change', function () {
      if (!file.files[0]) return;
      shrink(file.files[0]).then(function (data) { state.image = data; state.pic = ''; state.preview = data; redraw(); }).catch(function (e) { D.toast(e.message); });
    });
    pics.appendChild(up); form.appendChild(pics);
    function redraw() {
      var n = hlCard({ title: fields.title.value.trim(), text: fields.text.value.trim(), img: state.preview });
      preview.replaceWith(n); preview = n;
      [].forEach.call(pics.querySelectorAll('[data-pic]'), function (b) { b.classList.toggle('on', b.dataset.pic === state.pic); });
    }
    redraw();
    var row = D.part({ type: 'row' }), save = D.part({ type: 'key', text: 'Save', style: 'go' }), cancel = D.part({ type: 'key', text: 'Cancel', style: 'small' });
    save.dataset.act = 'save-hl';
    save.addEventListener('click', function () {
      save.disabled = true;
      D.post('/dev/api.php', { a: 'highlight', highlight: { id: h.id, title: fields.title.value, text: fields.text.value, link: fields.link.value.trim(), pic: state.pic }, image: state.image }).then(function (j) {
        save.disabled = false;
        if (!j.ok) { D.toast(j.error || 'That didn\'t save.'); return; }
        D.toast('Saved. Push it when you want it on the front page.'); done();
      });
    });
    cancel.addEventListener('click', done);
    row.appendChild(save); row.appendChild(cancel); form.appendChild(row);
    slot.insertBefore(form, slot.firstChild);
    fields.title.focus();
  }
  function drawHighlights() {
    var slot = D.slot('posts'); if (!slot) return;
    var add = D.part({ type: 'key', text: 'Make a highlight', style: 'go', icon: 'plus' }); add.dataset.act = 'new-hl';
    add.addEventListener('click', function () { add.remove(); hlEditor(D.slot('posts', true), null, drawHighlights); });
    slot.appendChild(add);
    if (D.demo) return;
    D.post('/dev/api.php', { a: 'highlights' }).then(function (j) {
      var s = D.slot('posts', true); if (!s || !j.ok) return;
      if (!j.highlights.length) s.appendChild(D.part({ type: 'text', text: 'No highlights yet. Make one: a picture and a few friendly words.', tone: 'dim' }));
      j.highlights.forEach(function (h) {
        var card = D.part({ type: 'panel', gap: 10, children: [{ type: 'row', justify: 'between', children: [{ type: 'label', text: D.ago(h.at) + ' · ' + h.by }, { type: 'chip', text: h.pushed ? 'On the front page' : 'Not shown', on: h.pushed, led: h.pushed }] }] });
        card.dataset.highlight = h.id;
        card.appendChild(hlCard(h));
        var row = D.part({ type: 'row' });
        var push = D.part({ type: 'key', text: h.pushed ? 'Take it down' : 'Push to the front page', style: h.pushed ? 'small' : 'go' }); push.dataset.act = 'push';
        push.addEventListener('click', function () { push.disabled = true; D.post('/dev/api.php', { a: 'push', id: h.id, on: h.pushed ? 0 : 1 }).then(function (r) { if (!r.ok) { push.disabled = false; D.toast(r.error); return; } D.toast(h.pushed ? 'Taken down' : 'It\'s on the front page'); D.slot('posts'); drawHighlights(); }); });
        var edit = D.part({ type: 'key', text: 'Change', style: 'small' });
        edit.addEventListener('click', function () { hlEditor(D.slot('posts', true), h, function () { D.slot('posts'); drawHighlights(); }); });
        var del = D.part({ type: 'key', text: 'Delete', style: 'small' });
        del.addEventListener('click', function () {
          if (del.textContent !== 'Tap again to delete') { del.textContent = 'Tap again to delete'; return; }
          D.post('/dev/api.php', { a: 'unhighlight', id: h.id }).then(function () { D.slot('posts'); drawHighlights(); });
        });
        row.appendChild(push); row.appendChild(edit); row.appendChild(del); card.appendChild(row);
        s.appendChild(card);
      });
    });
  }
  function drawSite() {
    var slot = D.slot('posts'); if (!slot) return;
    var form = D.part({ type: 'panel', gap: 10, children: [{ type: 'title', text: 'The front page', size: 's' }, { type: 'text', text: 'The few words at the top of joshuavelasco.com. The picture news goes in Highlights.', tone: 'dim' }] });
    var inputs = {};
    FIELDS.forEach(function (f) {
      form.appendChild(D.part({ type: 'label', text: f[1] }));
      var i = D.part({ type: 'field', placeholder: f[2] }); i.dataset.field = f[0]; inputs[f[0]] = i; form.appendChild(i);
    });
    var save = D.part({ type: 'key', text: 'Save the words', style: 'go' }), when = D.part({ type: 'text', text: '', tone: 'dim' });
    form.appendChild(save); form.appendChild(when); slot.appendChild(form);
    // The saved words fill only the boxes you haven't typed in yet: a slow answer
    // never wipes what you've just written (J5.7).
    var typed = {};
    FIELDS.forEach(function (f) { inputs[f[0]].addEventListener('input', function () { typed[f[0]] = true; }); });
    if (!D.demo) D.get('/dev/api.php?site').then(function (j) {
      var s = j.site || {};
      FIELDS.forEach(function (f) { if (!typed[f[0]]) inputs[f[0]].value = s[f[0]] || ''; });
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
      D.slot('posts'); if (i === 2) drawSite(); else if (i === 1) drawHighlights(); else drawNews();
    }
    if (seg) [].forEach.call(seg.querySelectorAll('button'), function (b, i) { b.addEventListener('click', function () { if (i !== which) show(i); }); });
    show(0);
  };
})();
