// The lounge bookshelf's reader (J99): tap the bookshelf, pick a book, read
// it in pages. The books are public-domain works (bar/web/books/, made by
// tools/books.py), one chapter laid out at a time in CSS columns (a long
// novel stays light on the phone). Tap the right side or swipe left for the
// next page, the left side or swipe right for the one before; your place in
// each book, the text size and light or dark paper are kept on this phone.
window.bar3dReader = (function () {
  'use strict';
  var root = null, index = null, book = null, chapter = 0, page = 0, pages = 1;
  var prefs = load('bar3dReader.prefs', { size: 18, dark: true });

  function load(key, def) {
    try { var v = JSON.parse(localStorage.getItem(key)); return v || def; } catch (e) { return def; }
  }
  function save(key, value) {
    try { localStorage.setItem(key, JSON.stringify(value)); } catch (e) { /* private mode: not kept */ }
  }
  function el(tag, attrs, kids) {
    var e = document.createElement(tag);
    for (var k in attrs || {}) {
      if (k === 'text') e.textContent = attrs[k];
      else if (k === 'class') e.className = attrs[k];
      else if (k.slice(0, 2) === 'on') e[k] = attrs[k];
      else e.setAttribute(k, attrs[k]);
    }
    (kids || []).forEach(function (c) { if (c) e.appendChild(typeof c === 'string' ? document.createTextNode(c) : c); });
    return e;
  }

  function style() {
    if (document.getElementById('readerStyle')) return;
    var css = [
      '#reader{position:fixed;inset:0;z-index:60;display:flex;flex-direction:column;font-family:Georgia,"Times New Roman",serif;',
      '  background:var(--rd-bg);color:var(--rd-ink);padding:env(safe-area-inset-top) 0 env(safe-area-inset-bottom);}',
      '#reader.dark{--rd-bg:#1b1611;--rd-ink:#e8dcc4;--rd-dim:#a8987c;--rd-line:#3a3026;--rd-key:#2a221a;}',
      '#reader.light{--rd-bg:#f4ecdc;--rd-ink:#2b2219;--rd-dim:#7a6a52;--rd-line:#d9cbb0;--rd-key:#e8dcc4;}',
      '#reader .rdTop,#reader .rdFoot{display:flex;align-items:center;gap:8px;padding:10px 14px;font:600 13px system-ui,sans-serif;',
      '  color:var(--rd-dim);border-bottom:1px solid var(--rd-line);}',
      '#reader .rdFoot{border-bottom:0;border-top:1px solid var(--rd-line);justify-content:space-between;}',
      '#reader button{font:600 14px system-ui,sans-serif;color:var(--rd-ink);background:var(--rd-key);border:0;border-radius:8px;',
      '  padding:8px 11px;min-width:40px;}',
      '#reader .rdTitle{flex:1;min-width:0;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;text-align:center;}',
      '#reader .rdView{flex:1;position:relative;overflow:hidden;margin:18px 22px;}',
      '#reader .rdCols{height:100%;column-gap:44px;column-fill:auto;transition:transform .25s ease;}',
      '#reader .rdCols h2{font-size:1.25em;font-weight:normal;font-style:italic;margin:0 0 1em;color:var(--rd-dim);}',
      '#reader .rdCols p{margin:0 0 .25em;text-indent:1.4em;line-height:1.55;text-align:justify;hyphens:auto;-webkit-hyphens:auto;}',
      '#reader .rdList{flex:1;overflow-y:auto;padding:8px 16px 24px;}',
      '#reader .rdBook{display:block;width:100%;text-align:left;margin:10px 0;padding:14px 16px;border-radius:12px;font-family:Georgia,serif;}',
      '#reader .rdBook b{display:block;font-size:19px;font-weight:normal;margin-bottom:4px;}',
      '#reader .rdBook span{display:block;font:500 13px system-ui,sans-serif;color:var(--rd-dim);}',
      '#reader .rdIntro{font:14px system-ui,sans-serif;color:var(--rd-dim);margin:6px 2px 4px;line-height:1.4;}',
      '#reader .rdToc{position:absolute;inset:0;overflow-y:auto;background:var(--rd-bg);padding:4px 0;}',
      '#reader .rdToc button{display:block;width:100%;text-align:left;margin:4px 0;background:transparent;font-family:Georgia,serif;}',
      '#reader .rdToc button.here{background:var(--rd-key);}',
    ].join('\n');
    document.head.appendChild(el('style', { id: 'readerStyle' }, [css]));
  }

  function shell(title, kids, foot) {
    style();
    if (!root) {
      root = el('div', { id: 'reader' });
      document.body.appendChild(root);
    }
    root.className = prefs.dark ? 'dark' : 'light';
    root.innerHTML = '';
    var top = el('div', { class: 'rdTop' }, title);
    root.appendChild(top);
    kids.forEach(function (k) { root.appendChild(k); });
    if (foot) root.appendChild(foot);
    if (window.bar3dModal) bar3dModal.open('reader', close);
  }

  function close() {
    if (root) { root.remove(); root = null; }
    if (window.bar3dModal) bar3dModal.closed('reader');
  }

  // ---- The bookshelf: the list of books ----
  function library() {
    var list = el('div', { class: 'rdList' }, [el('div', { class: 'rdIntro', text: 'Books from the lounge shelf. Your place in each is kept on this phone.' })]);
    shell([el('button', { text: '✕', 'aria-label': 'Close', onclick: close }), el('div', { class: 'rdTitle', text: 'The bookshelf' })], [list]);
    var fill = function () {
      index.books.forEach(function (b) {
        var at = load('bar3dReader.at.' + b.id, null);
        var by = b.author + (b.translator ? ', translated by ' + b.translator : '') + ' · ' + b.year;
        var where = at ? 'Chapter ' + (at.ch + 1) + ' of ' + b.chapters : Math.round(b.words / 250 / 60) + ' hours of reading';
        list.appendChild(el('button', { class: 'rdBook', onclick: function () { openBook(b.id); } },
          [el('b', { text: b.title }), el('span', { text: by }), el('span', { text: where })]));
      });
    };
    if (index) fill();
    else fetch('books/index.json').then(function (r) { return r.json(); }).then(function (j) { index = j; if (root) fill(); })
      .catch(function () { list.appendChild(el('div', { class: 'rdIntro', text: 'The books could not be loaded. Try again in a moment.' })); });
  }

  // ---- A book ----
  function openBook(id) {
    var at = load('bar3dReader.at.' + id, { ch: 0, frac: 0 });
    var waiting = el('div', { class: 'rdIntro', text: 'Opening…' });
    shell([el('button', { text: '‹', 'aria-label': 'Back to the books', onclick: library }), el('div', { class: 'rdTitle', text: '' })], [waiting]);
    fetch('books/' + id + '.json').then(function (r) { return r.json(); }).then(function (b) {
      book = b;
      chapter = Math.min(at.ch, b.chapters.length - 1);
      showChapter(at.frac);
    }).catch(function () { waiting.textContent = 'This book could not be loaded. Try again in a moment.'; });
  }

  // A paragraph; _words_ between underscores are the book's italics.
  function para(text) {
    var p = el('p', {});
    text.split(/_([^_]+)_/).forEach(function (part, i) {
      p.appendChild(i % 2 ? el('em', { text: part }) : document.createTextNode(part));
    });
    return p;
  }

  var view, cols, pageLabel, titleLabel;
  function showChapter(frac, atEnd) {
    var c = book.chapters[chapter];
    titleLabel = el('div', { class: 'rdTitle', text: book.title });
    var smaller = el('button', { text: 'A−', 'aria-label': 'Smaller text', onclick: function () { resize(-2); } });
    var bigger = el('button', { text: 'A+', 'aria-label': 'Bigger text', onclick: function () { resize(2); } });
    var paper = el('button', { text: prefs.dark ? '☀' : '☾', 'aria-label': 'Light or dark paper', onclick: function () {
      prefs.dark = !prefs.dark; save('bar3dReader.prefs', prefs); var f = place(); showChapter(f);
    } });
    var toc = el('button', { text: '☰', 'aria-label': 'Contents', onclick: contents });
    view = el('div', { class: 'rdView' });
    cols = el('div', { class: 'rdCols' }, [el('h2', { text: c.title })].concat(c.paras.map(para)));
    cols.style.fontSize = prefs.size + 'px';
    view.appendChild(cols);
    pageLabel = el('span', {});
    var foot = el('div', { class: 'rdFoot' }, [
      el('button', { text: '‹', 'aria-label': 'Previous page', onclick: function () { turn(-1); } }), pageLabel,
      el('button', { text: '›', 'aria-label': 'Next page', onclick: function () { turn(1); } })]);
    shell([el('button', { text: '‹', 'aria-label': 'Back to the books', onclick: function () { remember(); library(); } }), titleLabel,
      toc, smaller, bigger, paper], [view], foot);
    layout();
    page = atEnd ? pages - 1 : Math.min(pages - 1, Math.round((frac || 0) * pages));
    show();
    touch(view);
  }

  function layout() {
    var w = view.clientWidth, h = view.clientHeight;
    cols.style.columnWidth = w + 'px';
    cols.style.height = h + 'px';
    cols.style.transition = 'none';
    pages = Math.max(1, Math.round((cols.scrollWidth + 44) / (w + 44)));
  }
  function show() {
    cols.style.transform = 'translateX(' + (-page * (view.clientWidth + 44)) + 'px)';
    pageLabel.textContent = 'Chapter ' + (chapter + 1) + ' of ' + book.chapters.length + ' · page ' + (page + 1) + ' of ' + pages;
    requestAnimationFrame(function () { cols.style.transition = ''; });
    remember();
  }
  function place() { return pages > 1 ? page / pages : 0; }
  function remember() { if (book) save('bar3dReader.at.' + book.id, { ch: chapter, frac: place() }); }
  function turn(d) {
    if (page + d >= 0 && page + d < pages) { page += d; show(); return; }
    if (d > 0 && chapter + 1 < book.chapters.length) { chapter++; showChapter(0); }
    else if (d < 0 && chapter > 0) { chapter--; showChapter(0, true); }
  }
  function resize(d) {
    var f = place();
    prefs.size = Math.max(13, Math.min(30, prefs.size + d));
    save('bar3dReader.prefs', prefs);
    showChapter(f);
  }
  function contents() {
    var list = el('div', { class: 'rdToc' });
    book.chapters.forEach(function (c, i) {
      list.appendChild(el('button', { class: i === chapter ? 'here' : '', text: c.title, onclick: function () { chapter = i; showChapter(0); } }));
    });
    view.appendChild(list);
    var here = list.querySelector('.here');
    if (here) here.scrollIntoView({ block: 'center' });
  }
  // Taps on the sides and swipes turn pages.
  function touch(v) {
    var x0 = null, y0 = 0;
    v.addEventListener('touchstart', function (e) { x0 = e.touches[0].clientX; y0 = e.touches[0].clientY; }, { passive: true });
    v.addEventListener('touchend', function (e) {
      if (x0 === null || v.querySelector('.rdToc')) return;
      var dx = e.changedTouches[0].clientX - x0, dy = e.changedTouches[0].clientY - y0;
      x0 = null;
      if (Math.abs(dx) > 40 && Math.abs(dx) > Math.abs(dy)) turn(dx < 0 ? 1 : -1);
    });
    v.addEventListener('click', function (e) {
      if (v.querySelector('.rdToc')) return;
      var r = v.getBoundingClientRect(), x = (e.clientX - r.left) / r.width;
      if (x > 0.66) turn(1);
      else if (x < 0.33) turn(-1);
    });
  }
  window.addEventListener('resize', function () {
    if (root && book && cols && document.body.contains(cols)) { var f = place(); layout(); page = Math.min(pages - 1, Math.round(f * pages)); show(); }
  });

  return { open: library, openBook: openBook, close: close, isOpen: function () { return !!root; } };
})();
