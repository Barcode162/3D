// The world changes while you're in the bar (docs/archive/STUDIO.md S9.1): every
// 15 s the page asks the Studio's store which version is live; when a newer
// one is published, it says so and comes back into the bar with it (the
// game reads the live world as it starts). Not in the Studio's own frame,
// not while you're racing or playing a game.
(function () {
  'use strict';
  if (/[?&](studio=1|net=off)/.test(location.search)) return;
  var STORE = '../studio/api.php?live', EVERY = 15000;
  var start = null, pending = false;
  function busy() {
    var b = document.body;
    return b.classList.contains('racing') || !!document.querySelector('iframe.projectFrame, #projectFrame') || document.hidden;
  }
  function note(text) {
    var t = document.getElementById('liveToast');
    if (!t) {
      t = document.createElement('div');
      t.id = 'liveToast';
      t.style.cssText = 'position:fixed;left:50%;top:calc(52px + env(safe-area-inset-top));transform:translateX(-50%);z-index:30;' +
        'padding:10px 16px;border-radius:12px;background:rgba(22,20,18,.9);color:#EEE8DC;font:600 14px Archivo,system-ui,sans-serif;' +
        'box-shadow:inset 0 1px 0 rgba(255,255,255,.07),0 0 0 1px #2C2926,0 14px 34px rgba(0,0,0,.55);pointer-events:none;text-align:center;max-width:86vw';
      document.body.appendChild(t);
    }
    t.textContent = text;
  }
  function check() {
    fetch(STORE + '&t=' + Date.now(), { cache: 'no-store' }).then(function (r) { return r.json(); }).then(function (live) {
      var v = live && live.version | 0;
      if (start === null) { start = v; return; }
      if (v > start && !pending) {
        pending = true;
        var wait = function () {
          if (busy()) return setTimeout(wait, 3000);
          note('The bar was just changed' + (live.author ? ' by ' + live.author : '') + ' — bringing it in…');
          setTimeout(function () { location.reload(); }, 2500);
        };
        wait();
      }
    }).catch(function () {});
  }
  check();
  setInterval(check, EVERY);
  window.bar3dLive = { check: check, version: function () { return start; } };
})();
