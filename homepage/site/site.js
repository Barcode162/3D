// joshuavelasco.com: the little bits of life on every page (homepage/site/).
// The header's shadow and menu, things sliding in as you scroll, numbers
// that count up, the live build label, the latest news, and Sign out.
(function () {
  'use strict';
  var root = document.documentElement;
  var reduce = window.matchMedia && matchMedia('(prefers-reduced-motion: reduce)').matches;

  // ---- Header: a line under it once you scroll; the phone menu ----
  var top = document.getElementById('top');
  function onScroll() { if (top) top.classList.toggle('scrolled', window.scrollY > 8); }
  window.addEventListener('scroll', onScroll, { passive: true });
  onScroll();

  var burger = document.querySelector('.burger');
  var drawer = document.getElementById('drawer');
  function setMenu(open) {
    if (!burger || !drawer) return;
    burger.setAttribute('aria-expanded', open ? 'true' : 'false');
    drawer.hidden = !open;
    document.body.classList.toggle('menu-open', open);
  }
  if (burger) burger.addEventListener('click', function () { setMenu(burger.getAttribute('aria-expanded') !== 'true'); });
  document.addEventListener('keydown', function (e) { if (e.key === 'Escape') setMenu(false); });
  window.addEventListener('resize', function () { if (window.innerWidth >= 960) setMenu(false); });

  // ---- Slide in on scroll ----
  var items = document.querySelectorAll('.reveal');
  if ('IntersectionObserver' in window && !reduce) {
    var io = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) {
        if (!e.isIntersecting) return;
        e.target.classList.add('in');
        io.unobserve(e.target);
      });
    }, { rootMargin: '0px 0px -8% 0px', threshold: 0.08 });
    items.forEach(function (el) { io.observe(el); });
  } else {
    items.forEach(function (el) { el.classList.add('in'); });
  }

  // ---- Numbers that count up: <b data-count="129942">0</b> ----
  function countUp(el) {
    var to = parseFloat(el.getAttribute('data-count'));
    var dec = (el.getAttribute('data-count').split('.')[1] || '').length;
    if (reduce || !isFinite(to)) { el.textContent = fmt(to, dec); return; }
    var t0 = null, dur = 1400;
    function step(t) {
      if (t0 === null) t0 = t;
      var k = Math.min(1, (t - t0) / dur);
      k = 1 - Math.pow(1 - k, 3);
      el.textContent = fmt(to * k, dec);
      if (k < 1) requestAnimationFrame(step);
    }
    requestAnimationFrame(step);
  }
  function fmt(v, dec) { return Number(v).toLocaleString('en-US', { minimumFractionDigits: dec, maximumFractionDigits: dec }); }
  var counters = document.querySelectorAll('[data-count]');
  if ('IntersectionObserver' in window) {
    var co = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) { if (e.isIntersecting) { countUp(e.target); co.unobserve(e.target); } });
    }, { threshold: 0.4 });
    counters.forEach(function (el) { co.observe(el); });
  } else counters.forEach(countUp);

  // ---- The live build (the bar's version.txt: the commit that is live) ----
  fetch('/bar/version.txt', { cache: 'no-store' }).then(function (r) { return r.ok ? r.text() : ''; }).then(function (t) {
    var sha = (t || '').trim();
    if (!/^[0-9a-f]{7,40}$/.test(sha)) return;
    document.querySelectorAll('[data-live-text]').forEach(function (el) { el.textContent = 'Live now · build ' + sha.slice(0, 7); });
    document.querySelectorAll('[data-build]').forEach(function (el) { el.textContent = sha.slice(0, 7); });
  }).catch(function () {});

  // ---- The latest news (public posts from /news/) ----
  var newsBox = document.querySelector('[data-news]');
  if (newsBox) {
    fetch('/news/?json', { cache: 'no-store' }).then(function (r) { return r.ok ? r.json() : null; }).then(function (j) {
      var posts = j && (j.posts || j);
      if (!Array.isArray(posts) || !posts.length) return;
      var list = newsBox.querySelector('.news-list');
      posts.slice(0, 3).forEach(function (p) {
        var a = document.createElement('a');
        a.className = 'news-item';
        a.href = '/news/';
        if (p.shots && p.shots[0]) {
          var img = document.createElement('img');
          img.loading = 'lazy'; img.alt = ''; img.src = '/news/' + p.shots[0];
          a.appendChild(img);
        }
        var when = document.createElement('span');
        when.className = 'when';
        var d = p.at ? new Date(p.at * (p.at < 1e12 ? 1000 : 1)) : null;
        when.textContent = (p.projectName || 'Update') + (d ? ' · ' + d.toLocaleDateString('en-US', { month: 'short', day: 'numeric' }) : '');
        var b = document.createElement('b'); b.textContent = p.title || 'Update';
        var txt = document.createElement('p'); txt.textContent = p.text || '';
        a.appendChild(when); a.appendChild(b); a.appendChild(txt);
        list.appendChild(a);
      });
      newsBox.hidden = false;
    }).catch(function () {});
  }

  // ---- Sign out (the login cookie is HttpOnly: ask for the token, then end it) ----
  function post(body) {
    return fetch('/bar/account.php', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify(body), credentials: 'same-origin' })
      .then(function (r) { return r.json(); });
  }
  function forget() { try { localStorage.removeItem('bar3dToken'); localStorage.removeItem('bar3dProfile'); } catch (e) {} }
  document.querySelectorAll('[data-signout]').forEach(function (btn) {
    btn.addEventListener('click', function () {
      btn.disabled = true;
      var saved = null;
      try { saved = localStorage.getItem('bar3dToken'); } catch (e) {}
      (saved ? Promise.resolve({ ok: 1, token: saved }) : post({ a: 'session' }))
        .then(function (j) { return j && j.ok ? post({ a: 'logout', token: j.token }) : null; })
        .catch(function () {})
        .then(function () { forget(); location.href = '/'; });
    });
  });

  root.classList.add('ready');
})();
