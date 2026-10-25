// Dev's motion (J90, dev.css → Motion): what makes the page feel alive.
//   - a ripple from wherever you touch a row, a card, a key or a tile;
//   - cards and tiles tilt a little toward your finger (or the mouse);
//   - every new page comes in piece by piece (a stagger);
//   - numbers count to their new value ([data-count]);
//   - pages slide: deeper from the right, back from the left (the View
//     Transitions API where the browser has it, a CSS slide elsewhere).
// Calm (the top bar's switch, or iPhone's Reduce Motion) stops all of it; it
// all pauses while Dev is in the background. Only transform and opacity
// move, so the phone's GPU does it without repainting the page.
(function () {
  'use strict';
  var D = window.Dev, root = document.documentElement;
  var reduce = false;
  try { reduce = window.matchMedia('(prefers-reduced-motion: reduce)').matches; } catch (e) {}
  var calm = D.store('devCalm') === '1' || (D.store('devCalm') === null && reduce);
  function applyCalm() {
    root.classList.toggle('calm', calm);
    var b = document.getElementById('calm');
    if (b) { b.setAttribute('aria-pressed', String(calm)); b.title = calm ? 'Motion is off (Calm): tap to bring it back' : 'Calm: stop the motion'; }
  }
  applyCalm();
  var M = D.motion = {
    calm: function () { return calm; },
    setCalm: function (on) { calm = !!on; D.store('devCalm', calm ? '1' : '0'); applyCalm(); },
  };
  document.addEventListener('DOMContentLoaded', applyCalm);
  document.addEventListener('click', function (e) {
    if (e.target.closest && e.target.closest('#calm')) { M.setCalm(!calm); D.toast(calm ? 'Calm: the motion is off.' : 'The motion is back.'); }
  });
  // Paused in the background: nothing runs while you're in another app.
  document.addEventListener('visibilitychange', function () { root.classList.toggle('dev-paused', document.hidden); });

  // ---- Touch: press, ripple, tilt ----
  var TOUCHABLE = '.ls-listrow, .dev-tile, .dev-card, .key, .chip, .dev-chip, .dev-crumb, .ls-tab, #tabs button, .dev-react button';
  function ripple(el, x, y) {
    if (calm) return;
    var r = el.getBoundingClientRect(), d = Math.max(r.width, r.height) * 2.2;
    var s = document.createElement('span'); s.className = 'dev-ripple';
    s.style.width = s.style.height = d + 'px';
    s.style.left = (x - r.left - d / 2) + 'px'; s.style.top = (y - r.top - d / 2) + 'px';
    if (getComputedStyle(el).position === 'static') el.style.position = 'relative';
    el.classList.add('dev-rippling');
    el.appendChild(s);
    s.addEventListener('animationend', function () { s.remove(); });
    setTimeout(function () { if (s.parentNode) s.remove(); }, 900);
  }
  var tilted = null;
  function tilt(el, x, y) {
    if (calm || !el) return;
    var r = el.getBoundingClientRect();
    var nx = (x - r.left) / r.width - 0.5, ny = (y - r.top) / r.height - 0.5;
    el.style.setProperty('--ry', (nx * 7).toFixed(2) + 'deg');
    el.style.setProperty('--rx', (-ny * 7).toFixed(2) + 'deg');
    el.classList.add('dev-tilting');
    tilted = el;
  }
  function untilt() { if (tilted) { tilted.classList.remove('dev-tilting'); tilted.style.removeProperty('--rx'); tilted.style.removeProperty('--ry'); tilted = null; } }
  document.addEventListener('pointerdown', function (e) {
    var el = e.target.closest && e.target.closest(TOUCHABLE);
    if (!el) return;
    el.classList.add('dev-pressed');
    ripple(el, e.clientX, e.clientY);
    var card = e.target.closest('.dev-tile, .dev-card');
    if (card) tilt(card, e.clientX, e.clientY);
  }, { passive: true });
  var raf = 0, lastMove = null;
  document.addEventListener('pointermove', function (e) {
    lastMove = e;
    if (raf) return;
    raf = requestAnimationFrame(function () {
      raf = 0;
      var ev = lastMove; if (!ev) return;
      var card = ev.target.closest && ev.target.closest('.dev-tile, .dev-card');
      if (card && (ev.pointerType === 'mouse' || ev.buttons)) tilt(card, ev.clientX, ev.clientY);
      else if (tilted && card !== tilted) untilt();
    });
  }, { passive: true });
  function release() {
    [].forEach.call(document.querySelectorAll('.dev-pressed'), function (el) { el.classList.remove('dev-pressed'); });
    if (lastMove && lastMove.pointerType !== 'mouse') untilt();
  }
  document.addEventListener('pointerup', release, { passive: true });
  document.addEventListener('pointercancel', function () { release(); untilt(); }, { passive: true });
  document.addEventListener('pointerleave', untilt, { passive: true });

  // ---- A new page comes in piece by piece ----
  M.enter = function (view) {
    var top = view.firstElementChild;
    if (!top) return;
    var items = [].slice.call(top.children);
    items.forEach(function (el, i) { el.style.setProperty('--i', String(Math.min(i, 12))); el.classList.add('dev-in'); });
    // Cards and tiles further down come in as they scroll into view.
    [].forEach.call(view.querySelectorAll('.dev-tile'), function (el, i) { el.style.setProperty('--i', String(Math.min(i, 12))); });
    M.count(view);
  };

  // ---- Numbers count to their value: <b data-count="12"> ----
  M.count = function (scope) {
    [].forEach.call((scope || document).querySelectorAll('[data-count]'), function (el) {
      var to = +el.dataset.count, from = +(el.dataset.shown || 0);
      if (!isFinite(to)) return;
      el.dataset.shown = String(to);
      if (calm || from === to) { el.textContent = String(to); return; }
      var t0 = performance.now(), dur = 700;
      (function step(now) {
        var k = Math.min(1, (now - t0) / dur), e = 1 - Math.pow(1 - k, 3);
        el.textContent = String(Math.round(from + (to - from) * e));
        if (k < 1) requestAnimationFrame(step);
      })(t0);
    });
  };

  // ---- Pages slide ----
  M.transition = function (dir, run) {
    root.dataset.dir = dir;
    if (calm || !document.startViewTransition) {
      var r = run();
      var view = document.getElementById('view');
      if (!calm) { view.classList.remove('dev-push', 'dev-pop', 'dev-swap'); void view.offsetWidth; view.classList.add('dev-' + dir); }
      return r;
    }
    // The new page is waited for at most 350 ms (its data can follow): the
    // screen never freezes on a slow answer.
    // A slide that hasn't finished in 1.5 s (a slow phone, a busy GPU) is
    // dropped: the new page shows at once and takes taps (a slide holds them).
    var t = document.startViewTransition(function () { return Promise.race([Promise.resolve(run()), new Promise(function (r) { setTimeout(r, 350); })]); });
    var late = setTimeout(function () { try { t.skipTransition(); } catch (e) {} }, 1500);
    if (t && t.finished) t.finished.then(function () { clearTimeout(late); }, function () { clearTimeout(late); });
    return t;
  };

  // A part that just changed: it glows once (the live panel, dev/live.js).
  M.flash = function (el) {
    if (!el || calm) return;
    el.classList.remove('dev-flash'); void el.offsetWidth; el.classList.add('dev-flash');
  };
})();
