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
  var musicGain = null;  // all music sources pass through this (to turn them down)
  var musicLevel = 1;
  // The rhythm game's song: decoded fully (so it plays with sample-exact
  // timing) and played through its own gain, not placed in 3D.
  var track = { state: 0, buffer: null, mono: null, node: null, start: 0, url: '' };

  // Sound problems never stop the game: logged as warnings (no badge).
  function report(text) {
    if (window.bar3dLog) bar3dLog.add('warn', 'Audio: ' + text);
  }

  // Glides a parameter to a new value, dropping any older scheduled changes
  // (holding the value it has right now).
  function retarget(param, value, t) {
    if (param.cancelAndHoldAtTime) param.cancelAndHoldAtTime(t); else param.cancelScheduledValues(t);
    param.setTargetAtTime(value, t, 0.03);
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
    if (!musicGain) {
      musicGain = ctx.createGain();
      musicGain.gain.value = musicLevel * busLevel.music;
      musicGain.connect(ctx.destination);
    }
    s.panner.connect(musicGain);
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
      watchState();
    }
    if (ctx.state !== 'running') ctx.resume();
    sources.forEach(function (s) { connect(s); bless(s); });
  }
  ['touchend', 'mousedown', 'keydown'].forEach(function (type) {
    window.addEventListener(type, unlock, true);
  });

  // In the background the sound is paused (and its clock stops, so songs
  // pick up where they were). Back on the page it resumes; if iOS won't let
  // it (it was interrupted, e.g. by a call), a banner asks for a tap.
  var resumeBanner = null;
  function showResume(show) {
    if (show && !resumeBanner) {
      resumeBanner = document.createElement('div');
      resumeBanner.id = 'tapResume';
      resumeBanner.textContent = '\u{1F50A} Tap to resume sound';
      resumeBanner.style.cssText = 'position:fixed;left:50%;bottom:calc(90px + env(safe-area-inset-bottom));' +
        'transform:translateX(-50%);z-index:9;background:#1a1220ee;color:#f5ecf8;padding:10px 16px;' +
        'border-radius:16px;font:600 15px -apple-system,system-ui,sans-serif;pointer-events:auto;';
      resumeBanner.addEventListener('click', unlock);
      document.body.appendChild(resumeBanner);
    }
    if (resumeBanner) resumeBanner.style.display = show ? 'block' : 'none';
  }
  document.addEventListener('visibilitychange', function () {
    if (!ctx) return;
    if (document.hidden) {
      if (ctx.state === 'running') ctx.suspend().catch(function () {});
      return;
    }
    ctx.resume().catch(function () {});
    setTimeout(function () { if (ctx.state !== 'running' && !document.hidden) showResume(true); }, 600);
  });
  function watchState() {
    ctx.addEventListener('statechange', function () {
      if (ctx.state === 'running') showResume(false);
      else if (!document.hidden && ctx.state === 'interrupted') showResume(true);
    });
  }

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

  function decodeContext() {
    if (ctx) return ctx;
    var OAC = window.OfflineAudioContext || window.webkitOfflineAudioContext;
    return new OAC(1, 1, 44100);
  }

  // ---- Sounds made in C++ (engine/synth.cpp): piano notes, effects ----
  // Samples arrive once as a Float32Array and become an AudioBuffer when
  // sound is unlocked. Each plays through a bus (effects or piano) whose
  // volume follows the settings, optionally placed in 3D.
  var sounds = [];       // id -> {data, rate, buffer}
  var voices = {};       // id -> {src, gain}
  var nextVoice = 1;
  var buses = {};        // name -> GainNode
  var busLevel = { sfx: 1, piano: 1, music: 1, voice: 1 };
  function bus(name) {
    if (!ctx) return null;
    if (!buses[name]) {
      buses[name] = ctx.createGain();
      buses[name].gain.value = busLevel[name] === undefined ? 1 : busLevel[name];
      buses[name].connect(ctx.destination);
    }
    return buses[name];
  }
  function bufferOf(s) {
    if (!s.buffer && ctx) {
      s.buffer = ctx.createBuffer(1, s.data.length, s.rate);
      s.buffer.getChannelData(0).set(s.data);
    }
    return s.buffer;
  }

  return {
    ready: function () { return !!ctx && ctx.state === 'running'; },

    // ---- Generated sounds ----
    soundCreate: function (heap, start, count, rate) {
      sounds.push({ data: heap.slice(start, start + count), rate: rate, buffer: null });
      return sounds.length - 1;
    },
    // Plays sound `id`; when = seconds from now; placed = use x, y, z.
    // Returns a voice id (0 = not played).
    soundPlay: function (id, gain, placed, x, y, z, when, rate, busName, loop) {
      var s = sounds[id];
      if (!ctx || ctx.state !== 'running' || !s) return 0;
      var src = ctx.createBufferSource();
      src.buffer = bufferOf(s);
      src.playbackRate.value = rate || 1;
      src.loop = !!loop;
      var g = ctx.createGain();
      g.gain.value = gain;
      src.connect(g);
      var out = g;
      if (placed) {
        var p = ctx.createPanner();
        p.panningModel = 'HRTF';
        p.distanceModel = 'inverse';
        p.refDistance = 1.2;
        p.rolloffFactor = 1;
        if (p.positionX) { p.positionX.value = x; p.positionY.value = y; p.positionZ.value = z; }
        else p.setPosition(x, y, z);
        g.connect(p);
        out = p;
      }
      out.connect(bus(busName || 'sfx'));
      var id2 = nextVoice++;
      voices[id2] = { src: src, gain: g };
      src.onended = function () { delete voices[id2]; };
      src.start(ctx.currentTime + Math.max(0, when || 0));
      return id2;
    },
    // Fades a voice out over `release` seconds, `when` seconds from now.
    soundStop: function (voice, when, release) {
      var v = voices[voice];
      if (!v || !ctx) return;
      var t = ctx.currentTime + Math.max(0, when || 0);
      try {
        v.gain.gain.setTargetAtTime(0, t, Math.max(0.005, release / 3));
        v.src.stop(t + release + 0.05);
      } catch (e) {}
    },
    // Called every frame for engine sounds: only changes of 1% or more are
    // sent, and old scheduled changes are cleared first, so Safari's list of
    // pending changes can't grow (it lagged the race while revving).
    soundSet: function (voice, gain, rate) {
      var v = voices[voice];
      if (!v || !ctx) return;
      var t = ctx.currentTime;
      if (v.lastGain === undefined || Math.abs(gain - v.lastGain) > 0.01 * Math.max(v.lastGain, 0.05)) {
        v.lastGain = gain;
        retarget(v.gain.gain, gain, t);
      }
      if (v.lastRate === undefined || Math.abs(rate - v.lastRate) > 0.01 * v.lastRate) {
        v.lastRate = rate;
        retarget(v.src.playbackRate, rate, t);
      }
    },
    now: function () { return ctx ? ctx.currentTime : 0; },
    // Volumes from the settings (0..1): sfx, piano, music, voice.
    setBusVolume: function (name, v) {
      busLevel[name] = v;
      if (name === 'music') { this.setMusicGain(musicLevel); return; }
      if (buses[name] && ctx) buses[name].gain.setTargetAtTime(v, ctx.currentTime, 0.05);
    },
    context: function () { return ctx; },
    bus: function (name) { return bus(name); },

    // Turns all music sources (the jukebox) up or down, 0..1.
    setMusicGain: function (g) {
      musicLevel = g;
      if (musicGain) musicGain.gain.setTargetAtTime(g * busLevel.music, ctx.currentTime, 0.15);
    },

    // ---- The rhythm game's track ----
    // state: 0 nothing, 1 loading, 2 ready, 3 failed.
    trackLoad: function (url) {
      this.trackStop();
      track = { state: 1, buffer: null, mono: null, node: null, start: 0, url: url };
      var t = track;
      fetch(url).then(function (r) {
        if (!r.ok) throw new Error('HTTP ' + r.status);
        return r.arrayBuffer();
      }).then(function (data) {
        return new Promise(function (resolve, reject) {
          var p = decodeContext().decodeAudioData(data, resolve, reject);
          if (p && p.then) p.then(resolve, reject);
        });
      }).then(function (buffer) {
        if (t !== track) return;
        var n = buffer.length, mono = new Float32Array(n);
        for (var c = 0; c < buffer.numberOfChannels; c++) {
          var d = buffer.getChannelData(c);
          for (var i = 0; i < n; i++) mono[i] += d[i] / buffer.numberOfChannels;
        }
        t.buffer = buffer;
        t.mono = mono;
        t.state = 2;
      }).catch(function (e) {
        if (t !== track) return;
        report('Could not load the song for the rhythm game: ' + e);
        t.state = 3;
      });
    },
    trackState: function () { return track.state; },
    trackLength: function () { return track.mono ? track.mono.length : 0; },
    trackRate: function () { return track.buffer ? track.buffer.sampleRate : 44100; },
    // Hands over the mono samples once (C++ copies them into its memory);
    // the buffer itself stays for playing.
    trackTakeMono: function () {
      var m = track.mono;
      track.mono = null;
      return m;
    },
    // Starts playing `delay` seconds from now.
    trackPlay: function (delay) {
      if (!ctx || !track.buffer) return false;
      if (ctx.state !== 'running') ctx.resume();
      var src = ctx.createBufferSource();
      src.buffer = track.buffer;
      var g = ctx.createGain();
      g.gain.value = 0.9;
      src.connect(g);
      g.connect(ctx.destination);
      track.start = ctx.currentTime + delay;
      src.start(track.start);
      track.node = src;
      return true;
    },
    // Seconds into the song as you hear it (negative before it starts).
    trackTime: function () {
      if (!ctx || !track.node) return -1000;
      var latency = (ctx.outputLatency || 0) + (ctx.baseLatency || 0);
      return ctx.currentTime - track.start - latency;
    },
    trackDuration: function () { return track.buffer ? track.buffer.duration : 0; },
    trackStop: function () {
      if (track.node) { try { track.node.stop(); } catch (e) {} track.node = null; }
    },

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
    // offset: seconds into the song (to join other players mid-song).
    play: function (id, url, offset) {
      var s = sources[id], el = s.el;
      el.src = url;
      if (offset > 0.5) {
        // The position can only be set once the file's length is known;
        // add the time spent loading so it still lines up.
        var asked = performance.now();
        var seek = function () {
          el.removeEventListener('loadedmetadata', seek);
          var at = offset + (performance.now() - asked) / 1000;
          if (isFinite(el.duration) && at >= el.duration) at = Math.max(el.duration - 0.1, 0);
          try { el.currentTime = at; } catch (e) {}
        };
        el.addEventListener('loadedmetadata', seek);
      }
      el.play().catch(function (e) { report('Music did not start: ' + e.message); });
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
