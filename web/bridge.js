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
        var now = performance.now();
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
    // C++ → page, once per frame: f64 = HEAPF64, at = index of the area,
    // u8 = HEAPU8, text = byte offset for the menu commands. Layout: see
    // engine/bridge.cpp.
    pull: function (f64, at, u8, text, textCap) {
      var A = window.bar3dAudio, S = window.bar3dSettings, V = window.bar3dVoice, U = window.bar3dUi;
      var acct = window.bar3dAccount;
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
      f64[h + 11] = (U && U['isOpen'] && U['isOpen']()) || (acct && acct['isOpen'] && acct['isOpen']()) ? 1 : 0;
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
      // Menu commands, as JSON text.
      f64[h + 18] = writeText(u8, text, textCap, U ? U['take']() : '[]');
      f64[h + 19] = lastTouch;
      f64[h + 20] = A && A['userSongCount'] ? A['userSongCount']() : 0;
      return 1;
    },
    // Page work the game asked for this frame, as JSON: [[op, ...args], ...].
    flush: function (json) {
      var ops;
      try { ops = JSON.parse(json); } catch (e) { return; }
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
            case 'info': { var U2 = window.bar3dUi; if (U2) U2['setInfo'](op[1]); break; }
          }
        } catch (e) {
          if (window.bar3dLog) bar3dLog.add('error', 'Bridge: ' + op[0] + ' failed: ' + e);
        }
      }
    },
  };
})();
