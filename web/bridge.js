// The mailbox between the game (C++) and the page: everything the page
// collects between frames (canvas size, safe area, touches, menu commands,
// voice levels...) is copied into WebAssembly memory in ONE call at the start
// of each frame (pull), and everything the game wants done on the page
// (status text, joystick rings, log lines, voice positions...) arrives as
// ONE list at the end of the frame (flush). Calls between the two sides are
// slow on phones, so they're batched here. C++ side: engine/bridge.*.
window.bar3dBridge = (function () {
  'use strict';
  var canvas = null;
  var cssW = 0, cssH = 0, dpr = 1;
  var safe = [0, 0, 0, 0];
  var blurs = 0;
  var touches = [];        // [code, id, x, y, time] per changed touch
  var lastTouch = -1e9;    // when the last touch came (the phone's pretend mouse clicks follow it)
  var encoder = new TextEncoder();
  var pulls = 0;  // = game frames (tests use it)
  var commands = [];       // menu commands since the last pull: [name, a, b, text, text2]
  var rosterSent = -1;     // the roster version the game has

  function measure() {
    if (!canvas) return;
    var r = canvas.getBoundingClientRect();
    cssW = r.width;
    cssH = r.height;
    dpr = window.devicePixelRatio || 1;
  }
  // Safe-area insets (the notch, the home bar) from CSS env().
  var probe = document.createElement('div');
  probe.style.cssText = 'position:fixed;visibility:hidden;pointer-events:none;' +
    'padding:env(safe-area-inset-top) env(safe-area-inset-right) env(safe-area-inset-bottom) env(safe-area-inset-left)';
  function measureSafe() {
    var s = getComputedStyle(probe);
    safe = [parseFloat(s.paddingLeft) || 0, parseFloat(s.paddingTop) || 0,
            parseFloat(s.paddingRight) || 0, parseFloat(s.paddingBottom) || 0];
  }

  function attach(selector) {
    canvas = document.querySelector(selector);
    if (!canvas) return;
    document.body.appendChild(probe);
    measure();
    measureSafe();
    // Size changes are noticed here, never measured inside a frame.
    if (window.ResizeObserver) {
      new ResizeObserver(function (entries) {
        var e = entries[entries.length - 1];
        var box = e.contentBoxSize && e.contentBoxSize[0];
        cssW = box ? box.inlineSize : e.contentRect.width;
        cssH = box ? box.blockSize : e.contentRect.height;
        dpr = window.devicePixelRatio || 1;
        measureSafe();
      }).observe(canvas);
    } else {
      window.addEventListener('resize', function () { measure(); measureSafe(); });
    }
    // Touches: passive listeners only queue them (they never make Safari wait
    // for the game before drawing). The page has touch-action: none.
    ['touchstart', 'touchmove', 'touchend', 'touchcancel'].forEach(function (type, code) {
      canvas.addEventListener(type, function (e) {
        if (touches.length > 2000) return;  // nobody is reading (a stalled frame loop)
        var now = e.timeStamp || performance.now();  // when it happened, not when handled
        for (var i = 0; i < e.changedTouches.length; i++) {
          var t = e.changedTouches[i];
          touches.push(code, t.identifier, t.clientX, t.clientY, now);
        }
        lastTouch = now;
      }, { passive: true });
    });
    // Lost focus / hidden: the game lets go of every held key and finger.
    window.addEventListener('blur', function () { blurs++; });
    window.addEventListener('pagehide', function () { blurs++; });
    document.addEventListener('visibilitychange', function () { if (document.hidden) blurs++; });
  }

  function focusedField() {
    var a = document.activeElement;
    return a && (a.tagName === 'INPUT' || a.tagName === 'TEXTAREA') ? 1 : 0;
  }

  // Writes a string as UTF-8 (zero-terminated) into the heap; returns its length.
  function writeText(heapU8, ptr, cap, text) {
    var bytes = encoder.encode(text || '');
    var n = Math.min(bytes.length, cap - 1);
    heapU8.set(bytes.subarray(0, n), ptr);
    heapU8[ptr + n] = 0;
    return n;
  }

  var HEADER = 24, MAX_TOUCHES = 256, MAX_VOICES = 32;
  return {
    attach: attach,
    lastTouch: function () { return lastTouch; },
    pulls: function () { return pulls; },
    // A menu command for the game (web/menu.js), read at the next pull.
    command: function (name, a, b, text, text2) {
      if (commands.length < 500) commands.push([String(name), +a || 0, +b || 0, String(text || ''), String(text2 || '')]);
    },
    // C++ → page, once per frame: f64 = HEAPF64, at = index of the area,
    // u8 = HEAPU8, text = byte offset for the menu commands. Layout: see
    // engine/bridge.cpp.
    pull: function (f64, at, u8, text, textCap) {
      var A = window.bar3dAudio, S = window.bar3dSettings, V = window.bar3dVoice;
      var h = at;
      pulls++;
      f64[h + 0] = performance.now();
      f64[h + 1] = cssW;
      f64[h + 2] = cssH;
      f64[h + 3] = dpr;
      f64[h + 4] = safe[0]; f64[h + 5] = safe[1]; f64[h + 6] = safe[2]; f64[h + 7] = safe[3];
      f64[h + 8] = document.hidden ? 1 : 0;
      f64[h + 9] = blurs;
      f64[h + 10] = focusedField();
      f64[h + 11] = window.bar3dModal && bar3dModal.isOpen() ? 1 : 0;
      f64[h + 12] = A && A['ready'] && A['ready']() ? 1 : 0;
      f64[h + 13] = A && A['now'] ? A['now']() : 0;
      f64[h + 14] = S ? S['version']() : 0;
      f64[h + 15] = V && V['myLevel'] ? V['myLevel']() : 0;
      // Touches.
      var nt = Math.min(touches.length / 5, MAX_TOUCHES);
      f64[h + 16] = nt;
      for (var i = 0; i < nt * 5; i++) f64[h + HEADER + i] = touches[i];
      touches.splice(0, nt * 5);
      // Voice levels of the other players: [index, level].
      var lv = V && V['levels'] ? V['levels']() : [];
      var nv = Math.min(lv.length, MAX_VOICES);
      f64[h + 17] = nv;
      var vb = h + HEADER + MAX_TOUCHES * 5;
      for (var k = 0; k < nv; k++) { f64[vb + k * 2] = lv[k][0]; f64[vb + k * 2 + 1] = lv[k][1]; }
      // The text area: menu commands in the first half, the roster in the second.
      var half = textCap >> 1;
      // Menu commands, as JSON text (nothing at all when there are none).
      if (commands.length) {
        var json = JSON.stringify(commands);
        if (json.length * 3 < half) { commands = []; f64[h + 18] = writeText(u8, text, half, json); }
        else { f64[h + 18] = writeText(u8, text, half, JSON.stringify(commands.splice(0, 1))); }
      } else {
        f64[h + 18] = 0;
      }
      f64[h + 19] = lastTouch;
      f64[h + 20] = A && A['userSongCount'] ? A['userSongCount']() : 0;
      // Who's in the room, only when it changed (-1 = same as before).
      var N = window.bar3dNet;
      var rv = N && N['rosterVersion'] ? N['rosterVersion']() : 0;
      if (rv !== rosterSent && N && N['rosterJson']) {
        rosterSent = rv;
        f64[h + 21] = writeText(u8, text + half, half, N['rosterJson']());
      } else {
        f64[h + 21] = -1;
      }
      return 1;
    },
    // Page work the game asked for this frame, as JSON: [[op, ...args], ...].
    flush: function (json) {
      var ops;
      try { ops = JSON.parse(json); } catch (e) {
        if (window.bar3dLog) bar3dLog.add('error', 'Bridge: bad page operations (' + e.message + '): ' + json.slice(0, 200));
        return;
      }
      for (var i = 0; i < ops.length; i++) {
        var op = ops[i];
        try {
          switch (op[0]) {
            case 'status': { var st = document.getElementById('status'); if (st) st.textContent = op[1]; break; }
            case 'stick': {
              var base = document.getElementById('stickBase'), knob = document.getElementById('stickKnob');
              if (!base || !knob) break;
              base.style.display = knob.style.display = op[1] ? 'block' : 'none';
              base.style.transform = 'translate(' + op[2] + 'px,' + op[3] + 'px)';
              knob.style.transform = 'translate(' + op[4] + 'px,' + op[5] + 'px)';
              break;
            }
            case 'log': if (window.bar3dLog) bar3dLog.add(op[1], op[2]); break;
            case 'voicePlace': {  // [index, x, y, z, boost] per player
              var V2 = window.bar3dVoice;
              if (V2) for (var j = 0; j < op[1].length; j++) { var p = op[1][j]; V2['place'](p[0], p[1], p[2], p[3], p[4]); }
              break;
            }
            case 'listener': { var A2 = window.bar3dAudio; if (A2) A2['setListener'].apply(A2, op.slice(1)); break; }
            case 'info': { var U2 = window.bar3dUi; if (U2) U2['setInfo'](op[1]); break; }  // an object: merged
            case 'toast': { var U3 = window.bar3dUi; if (U3) U3['toast'](op[1]); break; }
            case 'panel': { var U4 = window.bar3dUi; if (U4) U4['open'](op[1]); break; }
          }
        } catch (e) {
          if (window.bar3dLog) bar3dLog.add('error', 'Bridge: ' + op[0] + ' failed: ' + e);
        }
      }
    },
  };
})();

// One manager for every panel that covers the game (the menus in
// web/menu.js, the account screens in web/account.js): only one is open at
// a time (opening one closes the other), and the game is told (modalOpen in
// the mailbox) so it ignores look input meanwhile.
window.bar3dModal = (function () {
  'use strict';
  var current = null;  // {name, close}
  return {
    // Call when a panel opens; close() is how to close it if another opens.
    open: function (name, close) {
      if (current && current.name !== name) { var c = current; current = null; try { c.close(); } catch (e) {} }
      current = { name: name, close: close };
    },
    // Call when a panel closes itself.
    closed: function (name) { if (current && current.name === name) current = null; },
    isOpen: function () { return !!current; },
    name: function () { return current ? current.name : ''; },
  };
})();
