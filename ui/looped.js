// Looped UI (docs/STUDIO.md §4): themes, step lights and the feedback kit.
// Load after the page's markup (or with defer). Exposes window.Looped:
//   Looped.look()              the theme in use ('faceplate', 'backbar', ...)
//   Looped.setLook(name)       switch (saved for next time on this device)
//   Looped.LOOKS               the themes, with their names in words
//   Looped.lamps(el, stage)    save lamps: 'edit' | 'saving' | 'saved' | 'live'
//   Looped.roll(el, value)     roll a number to a new value (pops the change)
//   Looped.toast(text)         a short message at the bottom
//   Looped.click(strong)       the theme's key sound (if clicks are on)
//   Looped.clicks(on)          turn key sounds on/off (saved)
// Markup it brings to life by itself: .steps[data-period] (seconds per loop),
// [data-knob], [data-fader], .switch[role=switch], [data-vu] (set el.level).
(function () {
  'use strict';
  var LOOKS = {
    // LOOKS: written by tools/uikit.mjs from ui/system/themes/
    ember: { name: "Ember", about: "Warm amber, frosted glass you can see the glow through, titles with a light passing through them.", click: [1500,0.025,"triangle"] },
    faceplate: { name: "Faceplate", about: "The drum machine: glossy keys, keycap lights.", click: [2600,0.025,"square"] },
    backbar: { name: "Back Bar", about: "Lacquered wood, brass, Edison bulbs.", click: [900,0.04,"triangle"] },
    obsidian: { name: "Obsidian", about: "Deepest black, frosted glass, ice light.", click: [4200,0.05,"sine"] },
    jukebox: { name: "Jukebox", about: "Chrome, candy jelly keys, bubble tubes.", click: [520,0.06,"square"] },
    clean: { name: "Clean", about: "Calm and clear, like the phone itself: system type, soft cards, one blue.", click: [3000,0.012,"sine"] },
    warm: { name: "Warm", about: "The bar at night: amber light on dark wood, soft and round, words as typed.", click: [1200,0.03,"triangle"] },
    bold: { name: "Bold", about: "Loud and certain: black frames, hard shadows, square corners, one yellow.", click: [700,0.03,"square"] },
    glass: { name: "Glass", about: "Frosted glass over deep colour: see-through panels, pill keys, a violet-to-cyan glow.", click: [4400,0.02,"sine"] },
    neon: { name: "Neon", about: "The bar's signs come alive: deep night, hot pink and cyan tubes that glow.", click: [1800,0.04,"sawtooth"] },
    soft: { name: "Soft", about: "Friendly and light: very round, pastel, gentle shadows, rounded type.", click: [2200,0.02,"sine"] }
    // end LOOKS
  };
  var root = document.documentElement;
  var DEFAULT_LOOK = 'faceplate';
  function load(k, d) { try { var v = localStorage.getItem(k); return v === null ? d : v; } catch (e) { return d; } }
  function save(k, v) { try { localStorage.setItem(k, v); } catch (e) {} }
  var reduce = false;
  try { reduce = window.matchMedia('(prefers-reduced-motion: reduce)').matches; } catch (e) {}

  // ---- Themes ----
  var look = 'faceplate';
  function setLook(name) {
    if (!LOOKS[name]) name = DEFAULT_LOOK;
    look = name;
    root.setAttribute('data-look', name);
    if (!root.hasAttribute('data-look-pin')) save('loopedLook', name);
    try { document.dispatchEvent(new CustomEvent('looped:look', { detail: name })); } catch (e) {}
  }
  var asked = (location.search.match(/[?&]look=([a-z0-9-]+)/) || [])[1];
  // A page can pin its theme (<html data-look="ember" data-look-pin>): the Toolbelt wears the house theme.
  var pinned = root.hasAttribute('data-look-pin') && root.getAttribute('data-look');
  setLook(asked || pinned || load('loopedLook', 'faceplate'));
  // Light or dark: 'auto' follows the phone (a theme with both halves switches
  // by itself); 'light' or 'dark' pins it.
  var scheme = 'auto';
  function setScheme(s) {
    scheme = s === 'light' || s === 'dark' ? s : 'auto';
    if (scheme === 'auto') root.removeAttribute('data-scheme'); else root.setAttribute('data-scheme', scheme);
    save('loopedScheme', scheme);
  }
  setScheme((location.search.match(/[?&]scheme=([a-z]+)/) || [])[1] || load('loopedScheme', 'auto'));

  // ---- Key sounds ----
  var actx = null, clicksOn = load('loopedClicks', '0') === '1';
  function click(strong) {
    if (!clicksOn) return;
    try {
      actx = actx || new (window.AudioContext || window.webkitAudioContext)();
      var c = LOOKS[look].click, t = actx.currentTime, o = actx.createOscillator(), g = actx.createGain();
      o.type = c[2]; o.frequency.setValueAtTime(c[0] * (strong ? 0.75 : 1), t); o.frequency.exponentialRampToValueAtTime(c[0] * 0.5, t + c[1]);
      g.gain.setValueAtTime(0.0001, t); g.gain.exponentialRampToValueAtTime(strong ? 0.22 : 0.12, t + 0.004); g.gain.exponentialRampToValueAtTime(0.0001, t + c[1] + 0.03);
      o.connect(g); g.connect(actx.destination); o.start(t); o.stop(t + c[1] + 0.05);
    } catch (e) {}
  }
  document.addEventListener('pointerdown', function (e) {
    var k = e.target.closest && e.target.closest('.key, .switch');
    if (k) click(k.classList.contains('go'));
  });

  // ---- Step lights and level meters (one loop for the page) ----
  function frame(now) {
    var secs = now / 1000, all = document.querySelectorAll('.steps[data-period]');
    for (var j = 0; j < all.length; j++) {
      var s = all[j], p = parseFloat(s.getAttribute('data-period')) || 2;
      var n = reduce || p <= 0 ? (p <= 0 ? -1 : 0) : Math.floor(secs / p * 4) % 4;
      for (var i = 0; i < s.children.length; i++) {
        var on = i === n;
        if (s.children[i].classList.contains('on') !== on) s.children[i].classList.toggle('on', on);
      }
    }
    var vus = document.querySelectorAll('[data-vu]');
    for (var v = 0; v < vus.length; v++) {
      var m = vus[v];
      if (!m.children.length) for (var b = 0; b < 16; b++) m.appendChild(document.createElement('b'));
      var lit = Math.round(Math.max(0, Math.min(1, m.level || 0)) * 16);
      for (var k = 0; k < 16; k++) { m.children[k].classList.toggle('on', k < lit); m.children[k].classList.toggle('hot', k >= 13); }
    }
    requestAnimationFrame(frame);
  }
  requestAnimationFrame(frame);

  // ---- Save lamps ----
  var STAGES = ['edit', 'saving', 'saved', 'live'];
  function lamps(el, stage) {
    var leds = el.querySelectorAll('.kled'), i = STAGES.indexOf(stage);
    for (var j = 0; j < leds.length; j++) {
      leds[j].className = 'kled' + (j === i ? ' on' + (stage === 'saved' ? ' ok' : stage === 'live' ? '' : ' warn') + (stage === 'edit' ? ' blink' : '') : '');
    }
  }

  // ---- Rolling numbers ----
  function roll(el, value) {
    var from = parseFloat(el.getAttribute('data-roll') || '0') || 0;
    el.setAttribute('data-roll', String(value));
    var pop = el.parentNode && el.parentNode.querySelector('.pop');
    if (pop && value !== from) {
      pop.textContent = (value > from ? '+' : '−') + Math.abs(value - from).toLocaleString('en-US');
      pop.classList.add('show'); clearTimeout(pop._t); pop._t = setTimeout(function () { pop.classList.remove('show'); }, 1300);
    }
    var cur = from;
    (function step() {
      var d = value - cur;
      cur = Math.abs(d) <= 1 ? value : cur + Math.sign(d) * Math.max(1, Math.ceil(Math.abs(d) / 6));
      el.textContent = Math.round(cur).toLocaleString('en-US');
      if (cur !== value) setTimeout(step, 40);
    })();
  }

  // ---- Toast ----
  var toastEl = null, toastT = 0;
  function toast(text) {
    if (!toastEl) { toastEl = document.createElement('div'); toastEl.className = 'toast panel over'; toastEl.setAttribute('role', 'status'); (document.querySelector('.lp') || document.body).appendChild(toastEl); }
    toastEl.textContent = text; toastEl.classList.add('show');
    clearTimeout(toastT); toastT = setTimeout(function () { toastEl.classList.remove('show'); }, 2200);
  }

  // ---- Knobs, faders, switches ----
  function num(el, k, d) { var v = parseFloat(el.getAttribute(k)); return isNaN(v) ? d : v; }
  function fmt(el, v) { var dec = num(el, 'data-dec', 2); return v.toFixed(dec) + (el.getAttribute('data-unit') || ''); }
  function emit(el, v) { el.value = v; try { el.dispatchEvent(new CustomEvent('change', { detail: v, bubbles: true })); } catch (e) {} }
  function setupKnob(el) {
    if (el._lp) return; el._lp = 1; el.classList.add('knob');
    var min = num(el, 'data-min', 0), max = num(el, 'data-max', 1), v = num(el, 'data-value', min), dots = [];
    var cap = document.createElement('div'); cap.className = 'cap'; el.appendChild(cap);
    for (var i = 0; i < 15; i++) {
      var a = (-135 + i * 270 / 14) * Math.PI / 180, d = document.createElement('span');
      d.className = 'dot'; d.style.transform = 'translate(' + (Math.sin(a) * 46).toFixed(1) + 'px,' + (-Math.cos(a) * 46).toFixed(1) + 'px)';
      el.appendChild(d); dots.push(d);
    }
    el.setAttribute('role', 'slider'); el.tabIndex = 0;
    function render() {
      var t = (v - min) / (max - min || 1);
      cap.style.transform = 'rotate(' + (-135 + t * 270) + 'deg)';
      for (var i = 0; i < dots.length; i++) dots[i].classList.toggle('on', i <= Math.round(t * 14));
      el.setAttribute('aria-valuenow', String(v)); el.setAttribute('aria-valuetext', fmt(el, v));
      var out = el.getAttribute('data-out') && document.getElementById(el.getAttribute('data-out'));
      if (out) out.textContent = fmt(el, v);
    }
    var y = null;
    el.addEventListener('pointerdown', function (e) { el.setPointerCapture(e.pointerId); y = e.clientY; });
    el.addEventListener('pointermove', function (e) {
      if (y === null) return;
      var before = Math.round((v - min) / (max - min) * 14);
      v = Math.max(min, Math.min(max, v + (y - e.clientY) / 160 * (max - min))); y = e.clientY;
      if (Math.round((v - min) / (max - min) * 14) !== before) click(false);
      render(); emit(el, v);
    });
    el.addEventListener('pointerup', function () { y = null; });
    el.addEventListener('keydown', function (e) {
      var d = /Up|Right/.test(e.key) ? 1 : /Down|Left/.test(e.key) ? -1 : 0;
      if (d) { e.preventDefault(); v = Math.max(min, Math.min(max, v + d * (max - min) / 14)); render(); emit(el, v); }
    });
    el.set = function (nv) { v = nv; render(); };
    render();
  }
  function setupFader(el) {
    if (el._lp) return; el._lp = 1; el.classList.add('fader');
    var min = num(el, 'data-min', 0), max = num(el, 'data-max', 1), v = num(el, 'data-value', min);
    el.innerHTML = '<div class="ticks"></div><div class="fill"></div><div class="cap"><span class="bubble"></span></div>';
    var cap = el.querySelector('.cap'), fill = el.querySelector('.fill'), bub = el.querySelector('.bubble');
    el.setAttribute('role', 'slider'); el.tabIndex = 0;
    function render() {
      var t = (v - min) / (max - min || 1), w = el.clientWidth - 32, x = 16 + t * w;
      cap.style.left = x + 'px'; fill.style.width = Math.max(0, x - 16) + 'px'; bub.textContent = fmt(el, v);
      el.setAttribute('aria-valuenow', String(v)); el.setAttribute('aria-valuetext', fmt(el, v));
      var out = el.getAttribute('data-out') && document.getElementById(el.getAttribute('data-out'));
      if (out) out.textContent = fmt(el, v);
    }
    function at(e) { var r = el.getBoundingClientRect(); v = min + Math.max(0, Math.min(1, (e.clientX - r.left - 16) / (r.width - 32))) * (max - min); render(); emit(el, v); }
    el.addEventListener('pointerdown', function (e) { el.setPointerCapture(e.pointerId); el.classList.add('drag'); at(e); });
    el.addEventListener('pointermove', function (e) { if (el.classList.contains('drag')) at(e); });
    el.addEventListener('pointerup', function () { el.classList.remove('drag'); });
    el.addEventListener('pointercancel', function () { el.classList.remove('drag'); });
    el.addEventListener('keydown', function (e) {
      var d = e.key === 'ArrowRight' ? 1 : e.key === 'ArrowLeft' ? -1 : 0;
      if (d) { e.preventDefault(); v = Math.max(min, Math.min(max, v + d * (max - min) / 20)); render(); emit(el, v); }
    });
    window.addEventListener('resize', render);
    el.set = function (nv) { v = nv; render(); };
    render();
  }
  document.addEventListener('click', function (e) {
    var s = e.target.closest && e.target.closest('.switch[role="switch"]');
    if (s) { var on = s.getAttribute('aria-checked') !== 'true'; s.setAttribute('aria-checked', String(on)); emit(s, on); }
  });
  function scan() {
    [].forEach.call(document.querySelectorAll('[data-knob]'), setupKnob);
    [].forEach.call(document.querySelectorAll('[data-fader]'), setupFader);
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', scan); else scan();

  window.Looped = {
    LOOKS: LOOKS, look: function () { return look; }, setLook: setLook,
    scheme: function () { return scheme; }, setScheme: setScheme,
    lamps: lamps, roll: roll, toast: toast, click: click, scan: scan,
    clicks: function (on) { if (on === undefined) return clicksOn; clicksOn = !!on; save('loopedClicks', on ? '1' : '0'); if (on) click(true); }
  };
})();
