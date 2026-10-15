// Thin bridge from the C++ audio code (engine/audio.cpp) to the browser's
// Web Audio API, the way WebGPU is the bridge for graphics. All decisions
// (what plays, where it is in 3D) are made in C++.
//
// iOS rule: sound may only start inside a user gesture. So the first touch
// (or click / key) creates the AudioContext and "blesses" every music
// element by playing a moment of silence; after that C++ can start music
// at any time.
window.bar3dAudio = (function () {
  'use strict';
  var ctx = null;
  var sources = [];
  var listener = null;  // last listener pose, applied once ctx exists
  var silence = null;

  function report(text) {
    if (window.Module && Module.printErr) Module.printErr('[error] ' + text);
  }

  // A 0.05 s silent WAV, used to unlock <audio> elements on iOS.
  function silentWav() {
    var samples = 2205, bytes = new Uint8Array(44 + samples * 2), v = new DataView(bytes.buffer);
    function str(o, s) { for (var i = 0; i < s.length; i++) bytes[o + i] = s.charCodeAt(i); }
    str(0, 'RIFF'); v.setUint32(4, 36 + samples * 2, true); str(8, 'WAVEfmt ');
    v.setUint32(16, 16, true); v.setUint16(20, 1, true); v.setUint16(22, 1, true);
    v.setUint32(24, 44100, true); v.setUint32(28, 88200, true); v.setUint16(32, 2, true);
    v.setUint16(34, 16, true); str(36, 'data'); v.setUint32(40, samples * 2, true);
    var bin = '';
    for (var i = 0; i < bytes.length; i++) bin += String.fromCharCode(bytes[i]);
    return 'data:audio/wav;base64,' + btoa(bin);
  }

  function setParam(param, value) { if (param) param.value = value; }

  function applyPosition(s) {
    if (!s.panner) return;
    if (s.panner.positionX) {
      setParam(s.panner.positionX, s.pos[0]); setParam(s.panner.positionY, s.pos[1]);
      setParam(s.panner.positionZ, s.pos[2]);
    } else {
      s.panner.setPosition(s.pos[0], s.pos[1], s.pos[2]);
    }
  }

  function applyListener() {
    if (!ctx || !listener) return;
    var l = ctx.listener, p = listener;
    if (l.positionX) {
      setParam(l.positionX, p[0]); setParam(l.positionY, p[1]); setParam(l.positionZ, p[2]);
      setParam(l.forwardX, p[3]); setParam(l.forwardY, p[4]); setParam(l.forwardZ, p[5]);
      setParam(l.upX, p[6]); setParam(l.upY, p[7]); setParam(l.upZ, p[8]);
    } else {
      l.setPosition(p[0], p[1], p[2]);
      l.setOrientation(p[3], p[4], p[5], p[6], p[7], p[8]);
    }
  }

  // Routes an <audio> element through a 3D panner once the context exists.
  function connect(s) {
    if (s.panner || !ctx) return;
    var node = ctx.createMediaElementSource(s.el);
    s.panner = ctx.createPanner();
    s.panner.panningModel = 'HRTF';
    s.panner.distanceModel = 'inverse';
    s.panner.refDistance = s.refDistance;
    s.panner.rolloffFactor = 0.8;
    s.panner.maxDistance = 100;
    node.connect(s.panner);
    s.panner.connect(ctx.destination);
    applyPosition(s);
  }

  function bless(s) {
    if (s.blessed) return;
    s.blessed = true;
    if (!s.el.src) {
      s.el.src = silence || (silence = silentWav());
      s.el.play().catch(function () {});
    }
  }

  function unlock() {
    if (!ctx) {
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) { report('This browser has no Web Audio.'); return; }
      ctx = new AC();
      applyListener();
    }
    if (ctx.state !== 'running') ctx.resume();
    sources.forEach(function (s) { connect(s); bless(s); });
  }
  ['touchend', 'mousedown', 'keydown'].forEach(function (type) {
    window.addEventListener(type, unlock, true);
  });

  // Your own songs, picked from the Files app. They stay on the phone: the
  // browser only gives us a local link to each file (lost on reload).
  // iOS opens the picker only from a real tap on a real element, so an
  // invisible <label> is placed over the "Load song" button that C++ draws.
  var userSongs = [];
  var picker = document.createElement('input');
  picker.type = 'file';
  picker.accept = 'audio/*';
  picker.multiple = true;
  picker.id = 'songPicker';
  picker.style.cssText = 'position:fixed;left:0;top:0;width:1px;height:1px;opacity:0;pointer-events:none;';
  picker.addEventListener('change', function () {
    Array.prototype.forEach.call(picker.files, function (file) {
      userSongs.push({ name: file.name.replace(/\.[^.]+$/, ''), url: URL.createObjectURL(file) });
    });
    picker.value = '';
  });
  var pickerButton = document.createElement('label');
  pickerButton.htmlFor = 'songPicker';
  pickerButton.style.cssText = 'position:fixed;display:none;z-index:5;-webkit-tap-highlight-color:transparent;';
  document.body.appendChild(picker);
  document.body.appendChild(pickerButton);

  return {
    ready: function () { return !!ctx && ctx.state === 'running'; },

    // A streaming music source (HTML audio element, so long songs don't
    // have to be decoded into memory). Returns its id.
    createSource: function (refDistance) {
      var el = new Audio();
      el.preload = 'auto';
      var s = { el: el, panner: null, pos: [0, 0, 0], refDistance: refDistance, blessed: false };
      el.addEventListener('error', function () {
        if (el.src && el.src.indexOf('data:') !== 0) report('Could not play ' + el.src.split('/').pop());
      });
      sources.push(s);
      if (ctx) connect(s);
      return sources.length - 1;
    },
    play: function (id, url) {
      var s = sources[id];
      s.el.src = url;
      s.el.play().catch(function (e) { report('Music did not start: ' + e.message); });
    },
    pause: function (id) { sources[id].el.pause(); },
    resume: function (id) { sources[id].el.play().catch(function () {}); },
    playing: function (id) { var el = sources[id].el; return !el.paused && !el.ended && !el.error; },
    finished: function (id) { return sources[id].el.ended; },
    setPosition: function (id, x, y, z) { var s = sources[id]; s.pos = [x, y, z]; applyPosition(s); },
    // Rectangle in CSS pixels; w = 0 hides the button.
    setSongPickerRect: function (x, y, w, h) {
      var st = pickerButton.style;
      st.display = w > 0 ? 'block' : 'none';
      st.left = x + 'px'; st.top = y + 'px'; st.width = w + 'px'; st.height = h + 'px';
    },
    userSongCount: function () { return userSongs.length; },
    userSongName: function (i) { return userSongs[i].name; },
    userSongUrl: function (i) { return userSongs[i].url; },
    setListener: function (px, py, pz, fx, fy, fz, ux, uy, uz) {
      listener = [px, py, pz, fx, fy, fz, ux, uy, uz];
      applyListener();
    },
  };
})();
