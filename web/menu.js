// The in-game menus, as HTML panels over the game (like the account
// screens): a Menu button in the top bar opens Emotes, Avatar, Profile,
// Voice chat, Settings, Piano songs, and (for bartenders) the drink list.
// C++ reads the settings (bar3dSettings) and takes the commands the menus
// queue (bar3dUi.take); it tells the menus what's going on (bar3dUi.setInfo).
window.bar3dSettings = (function () {
  'use strict';
  var DEFAULTS = {
    look: 0, lookSpeed: 1, invertLook: false, showNames: true,
    musicVol: 0.8, sfxVol: 0.9, pianoVol: 0.9, voiceVol: 1,
    // Voice: the mic only sends when you're louder than micGate (dB);
    // micGain boosts a quiet mic.
    micGate: -50, micGain: 1, pushToTalk: false,
    echoCancel: true, noiseSuppress: true, autoGain: false,
    hq: true, stereo: false, voice3d: true,
  };
  var values = {}, version = 1, listeners = [];
  try { values = JSON.parse(localStorage.getItem('bar3dSettings') || '{}') || {}; } catch (e) { values = {}; }
  function save() { try { localStorage.setItem('bar3dSettings', JSON.stringify(values)); } catch (e) {} }
  return {
    get: function (k) { return k in values ? values[k] : DEFAULTS[k]; },
    set: function (k, v) {
      values[k] = v;
      version++;
      save();
      listeners.forEach(function (l) { l(k, v); });
    },
    reset: function () { values = {}; version++; save(); listeners.forEach(function (l) { l('*'); }); },
    version: function () { return version; },
    onChange: function (l) { listeners.push(l); },
  };
})();

window.bar3dUi = (function () {
  'use strict';
  var S = window.bar3dSettings;
  var queue = [], info = {}, root = null, toastEl = null, toastTimer = 0, current = '';
  var EMOJI = ['\u{1F44B}', '\u{1F483}', '\u{1F44F}', '\u{1F64C}', '\u{1F647}', '\u{1F449}', '\u{1F937}', '\u{1F602}',
               '\u{1FAE1}', '\u{1F44D}', '\u{1F926}'];
  var LOOK_COLORS = ['#232326', '#ececE6', '#0d0d0d', '#bf1a1a', '#141a38', '#ff40a6', '#2e6138', '#5a5e6b'];

  function cmd(name, a, b, text) { queue.push([name, a || 0, b || 0, text || '']); }
  function el(tag, attrs, children) {
    var e = document.createElement(tag);
    for (var k in attrs || {}) {
      if (k === 'style') e.style.cssText = attrs[k];
      else if (k === 'text') e.textContent = attrs[k];
      else if (k.slice(0, 2) === 'on') e.addEventListener(k.slice(2), attrs[k]);
      else e.setAttribute(k, attrs[k]);
    }
    (children || []).forEach(function (c) { if (c) e.appendChild(typeof c === 'string' ? document.createTextNode(c) : c); });
    return e;
  }

  function addStyles() {
    var css = [
      '#menuRoot{position:fixed;inset:0;z-index:8;display:none;align-items:flex-end;justify-content:center;',
      'background:#0b071066;padding:12px;padding-bottom:calc(12px + env(safe-area-inset-bottom));',
      'font:15px/1.4 -apple-system,system-ui,sans-serif;color:#f5ecf8;}',
      '#menuRoot.open{display:flex;}',
      '.mPanel{width:min(94vw,420px);max-height:78vh;overflow:auto;background:#1a1220f2;border-radius:20px;',
      'box-shadow:0 10px 40px #000a;padding:16px 16px 12px;-webkit-overflow-scrolling:touch;}',
      '.mPanel h2{margin:0 0 10px;font-size:20px;display:flex;justify-content:space-between;align-items:center;}',
      '.mPanel h3{margin:14px 0 6px;font-size:14px;color:#cbb8d6;text-transform:uppercase;letter-spacing:1px;}',
      '.mX{font-size:22px;background:none;border:0;color:#f5ecf8;padding:0 6px;}',
      '.mGrid{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;}',
      '.mTile{background:#2a1f33;border:1px solid #4a3a55;border-radius:14px;padding:12px 4px;text-align:center;',
      'color:#f5ecf8;font:600 13px -apple-system,system-ui;}',
      '.mTile .big{font-size:28px;display:block;margin-bottom:4px;}',
      '.mTile.on{border-color:#ff5ab4;background:#3d2040;}',
      '.mBtn{display:block;width:100%;margin-top:8px;padding:11px;border:0;border-radius:12px;',
      'font:600 15px -apple-system,system-ui;color:#1a1220;background:#ff5ab4;}',
      '.mBtn.ghost{background:#2a1f33;color:#f5ecf8;}',
      '.mRow{display:flex;align-items:center;justify-content:space-between;gap:10px;margin:8px 0;}',
      '.mRow label{flex:1;}',
      '.mRow input[type=range]{flex:1.3;}',
      '.mRow .val{width:52px;text-align:right;color:#cbb8d6;font-size:13px;}',
      '.mToggle{width:50px;height:30px;border-radius:15px;background:#3a2f44;border:0;position:relative;flex:none;}',
      '.mToggle::after{content:"";position:absolute;left:3px;top:3px;width:24px;height:24px;border-radius:50%;',
      'background:#f5ecf8;transition:left .15s;}',
      '.mToggle.on{background:#ff5ab4;}.mToggle.on::after{left:23px;}',
      '.mSmall{font-size:12px;color:#cbb8d6;margin:4px 0;}',
      '.mMeter{position:relative;height:16px;border-radius:8px;background:#2a1f33;overflow:hidden;margin:6px 0;}',
      '.mMeter .lvl{position:absolute;left:0;top:0;bottom:0;background:linear-gradient(90deg,#5ad1ff,#7dff8a);}',
      '.mMeter .gate{position:absolute;top:0;bottom:0;width:3px;background:#ff5ab4;}',
      '.mList button{display:flex;justify-content:space-between;width:100%;text-align:left;margin:4px 0;padding:10px 12px;',
      'border-radius:10px;border:1px solid #4a3a55;background:#2a1f33;color:#f5ecf8;font:14px -apple-system,system-ui;}',
      '.mList button.on{border-color:#ff5ab4;background:#3d2040;}',
      '#menuToast{position:fixed;left:50%;top:calc(42px + env(safe-area-inset-top));transform:translateX(-50%);z-index:9;',
      'background:#1a1220ee;color:#f5ecf8;padding:8px 14px;border-radius:14px;font:14px -apple-system,system-ui;',
      'max-width:86vw;text-align:center;display:none;white-space:pre-line;}',
      '#hudMenu,#hudMic{pointer-events:auto;font:inherit;color:inherit;background:#0008;border:1px solid #f5d9a855;',
      'border-radius:6px;padding:4px 8px;margin-right:6px;}',
      '#hudMic.live{background:#2d7a3a;border-color:#7dff8a;}#hudMic.muted{background:#7a2d3a;}',
    ].join('');
    document.head.appendChild(el('style', { text: css }));
  }

  function close() { if (root) { root.classList.remove('open'); root.innerHTML = ''; } current = ''; stopMeter(); }
  function show(title, children) {
    root.innerHTML = '';
    var panel = el('div', { class: 'mPanel' }, [
      el('h2', {}, [title, el('button', { class: 'mX', text: '✕', onclick: close })]),
    ].concat(children));
    root.appendChild(panel);
    root.classList.add('open');
  }
  function toggle(label, key, onChange) {
    var b = el('button', { class: 'mToggle' + (S.get(key) ? ' on' : '') });
    b.onclick = function () {
      var v = !S.get(key);
      S.set(key, v);
      b.classList.toggle('on', v);
      if (onChange) onChange(v);
    };
    return el('div', { class: 'mRow' }, [el('label', { text: label }), b]);
  }
  function slider(label, key, min, max, step, fmt) {
    var val = el('span', { class: 'val', text: fmt(S.get(key)) });
    var r = el('input', { type: 'range', min: min, max: max, step: step, value: S.get(key) });
    r.oninput = function () { S.set(key, +r.value); val.textContent = fmt(+r.value); };
    return el('div', { class: 'mRow' }, [el('label', { text: label }), r, val]);
  }
  var pct = function (v) { return Math.round(v * 100) + '%'; };

  // ---- Panels ----
  function mainPanel() {
    var tiles = [
      ['\u{1F44B}', 'Emotes', emotesPanel], ['\u{1F9CD}', 'Avatar', avatarPanel], ['\u{1FAAA}', 'Profile', profilePanel],
      ['\u{1F3A4}', 'Voice chat', voicePanel], ['⚙️', 'Settings', settingsPanel], ['\u{1F3B9}', 'Piano', pianoPanel],
      ['\u{1F378}', 'Bartender', bartenderPanel], ['❓', 'Help', helpPanel],
    ];
    if (info.seated) tiles.push(['\u{1FA91}', 'Stand up', function () { cmd('standUp'); close(); }]);
    show('Menu', [el('div', { class: 'mGrid' }, tiles.map(function (t) {
      return el('button', { class: 'mTile', onclick: t[2] }, [el('span', { class: 'big', text: t[0] }), t[1]]);
    }))]);
    current = 'main';
  }

  function emotesPanel() {
    var names = info.emotes || ['Wave', 'Dance', 'Clap', 'Cheer', 'Bow', 'Point', 'Shrug', 'Laugh', 'Salute', 'Thumbs up', 'Facepalm'];
    show('Emotes', [el('div', { class: 'mGrid' }, names.map(function (n, i) {
      return el('button', { class: 'mTile', onclick: function () { cmd('emote', i + 1); close(); } },
                [el('span', { class: 'big', text: EMOJI[i] || '⭐' }), n]);
    })), el('div', { class: 'mSmall', text: 'Others see your emote. Dancing goes on until you walk.' })]);
    current = 'emotes';
  }

  function avatarPanel() {
    var names = info.looks || ['Josh'];
    show('Avatar', [el('div', { class: 'mGrid' }, names.map(function (n, i) {
      var swatch = el('span', { class: 'big', style: 'width:34px;height:34px;border-radius:50%;margin:0 auto 6px;border:2px solid #fff4;background:' + (LOOK_COLORS[i] || '#888') });
      return el('button', { class: 'mTile' + (S.get('look') === i ? ' on' : ''), onclick: function () {
        S.set('look', i);
        cmd('look', i);
        avatarPanel();
      } }, [swatch, n]);
    })), el('div', { class: 'mSmall', text: 'Look in the giant mirror to see it. Everyone else sees your new look too.' })]);
    current = 'avatar';
  }

  function profilePanel() {
    close();
    var a = window.bar3dAccount;
    if (a && a.openProfile) a.openProfile();
    else toast('Profiles need the multiplayer server.');
  }

  function settingsPanel() {
    show('Settings', [
      el('h3', { text: 'Controls' }),
      slider('Look speed', 'lookSpeed', 0.3, 2.5, 0.1, function (v) { return v.toFixed(1) + 'x'; }),
      toggle('Invert look up/down', 'invertLook'),
      el('h3', { text: 'Screen' }),
      toggle('Show name tags', 'showNames'),
      el('h3', { text: 'Sound' }),
      slider('Music', 'musicVol', 0, 1, 0.05, pct),
      slider('Effects', 'sfxVol', 0, 1, 0.05, pct),
      slider('Piano', 'pianoVol', 0, 1, 0.05, pct),
      slider('Voices', 'voiceVol', 0, 2, 0.05, pct),
      el('button', { class: 'mBtn ghost', text: 'Reset all settings', onclick: function () { S.reset(); settingsPanel(); } }),
    ]);
    current = 'settings';
  }

  // Voice chat settings with a live level meter and the gate threshold.
  var meterTimer = 0;
  function stopMeter() { if (meterTimer) { clearInterval(meterTimer); meterTimer = 0; } }
  function voicePanel() {
    var V = window.bar3dVoice;
    if (!V) { toast('Voice chat is not available.'); return; }
    var lvl = el('div', { class: 'lvl' }), gate = el('div', { class: 'gate' });
    var meter = el('div', { class: 'mMeter' }, [lvl, gate]);
    var status = el('div', { class: 'mSmall' });
    var joinBtn = el('button', { class: 'mBtn' });
    function refresh() {
      joinBtn.textContent = V.micOn() ? 'Turn my mic off' : 'Turn my mic on';
      joinBtn.className = 'mBtn' + (V.micOn() ? ' ghost' : '');
      status.textContent = V.statusText();
    }
    joinBtn.onclick = function () { (V.micOn() ? V.micOff() : V.micStart()).then(refresh, function (e) { toast(e.message || String(e)); refresh(); }); };
    var test = el('button', { class: 'mBtn ghost', text: 'Test my mic (record 4 s, play back)' });
    test.onclick = function () {
      test.disabled = true;
      test.textContent = 'Recording... say something';
      V.testMic(4, function (stage) { test.textContent = stage; }).then(function () {
        test.disabled = false;
        test.textContent = 'Test my mic (record 4 s, play back)';
      }, function (e) { toast(e.message || String(e)); test.disabled = false; test.textContent = 'Test my mic (record 4 s, play back)'; });
    };
    var monitor = el('button', { class: 'mToggle' + (V.monitoring() ? ' on' : '') });
    monitor.onclick = function () { V.setMonitor(!V.monitoring()); monitor.classList.toggle('on', V.monitoring()); };
    show('Voice chat', [
      joinBtn, status,
      el('h3', { text: 'Microphone level' }), meter,
      el('div', { class: 'mSmall', text: 'Your voice is only sent when the bar passes the pink line (the threshold). Move the slider so the line sits just above the noise when you are quiet.' }),
      slider('Threshold', 'micGate', -80, -10, 1, function (v) { return v + ' dB'; }),
      slider('Mic boost', 'micGain', 0.5, 4, 0.1, function (v) { return v.toFixed(1) + 'x'; }),
      toggle('Push to talk (hold the mic button)', 'pushToTalk'),
      el('h3', { text: 'Test' }), test,
      el('div', { class: 'mRow' }, [el('label', { text: 'Hear myself live (use headphones)' }), monitor]),
      el('h3', { text: 'Quality' }),
      toggle('Studio quality (Opus 510 kbps, near lossless)', 'hq', function () { V.applySettings(); }),
      toggle('Stereo', 'stereo', function () { V.applySettings(); }),
      toggle('Echo cancellation (keep on without headphones)', 'echoCancel', function () { V.restartMic(); }),
      toggle('Noise suppression', 'noiseSuppress', function () { V.restartMic(); }),
      toggle('Automatic mic volume', 'autoGain', function () { V.restartMic(); }),
      toggle('3D voices (louder when close)', 'voice3d', function () { V.applySettings(); }),
      slider('Voice volume', 'voiceVol', 0, 2, 0.05, pct),
    ]);
    current = 'voice';
    refresh();
    stopMeter();
    meterTimer = setInterval(function () {
      var db = V.inputDb(), g = S.get('micGate');
      var toPct = function (d) { return Math.max(0, Math.min(100, (d + 80) / 70 * 100)); };
      lvl.style.width = toPct(db) + '%';
      lvl.style.opacity = V.gateOpen() ? 1 : 0.45;
      gate.style.left = toPct(g) + '%';
      status.textContent = V.statusText();
    }, 60);
  }

  function pianoPanel() {
    var songs = info.songs || [];
    var list = el('div', { class: 'mList' }, songs.map(function (n, i) {
      return el('button', { class: info.pianoSong === i ? 'on' : '', onclick: function () { cmd('pianoSong', i); close(); toast('The piano plays: ' + n); } },
                [el('span', { text: (i < 10 ? (i + 1) + '. ' : '\u{1F4C1} ') + n }), el('span', { text: '▶' })]);
    }));
    var file = el('input', { type: 'file', accept: '.mid,.midi,audio/midi,audio/x-midi', id: 'midiPicker', style: 'display:none' });
    file.onchange = function () {
      Array.prototype.forEach.call(file.files, function (f) {
        cmd('pianoAdd', 0, 0, f.name.replace(/\.[^.]+$/, '') + '\n' + URL.createObjectURL(f));
      });
      toast('Added to the piano. Open Piano again to play it.');
      close();
    };
    var midiBtn = el('button', { class: 'mBtn ghost', text: midiAccess ? 'MIDI keyboard connected' : 'Connect a MIDI keyboard' });
    midiBtn.onclick = function () { connectMidi().then(function (n) { midiBtn.textContent = n + ' MIDI input(s) connected'; }, function (e) { toast(e.message || String(e)); }); };
    show('Piano', [
      el('div', { class: 'mSmall', text: 'Pick a song: the grand piano plays it for everyone, keys and strings light up. Sit on the bench to play it yourself.' }),
      list,
      el('button', { class: 'mBtn ghost', text: 'Stop the piano', onclick: function () { cmd('pianoStop'); close(); } }),
      el('label', { for: 'midiPicker', class: 'mBtn ghost', style: 'text-align:center;', text: 'Load a MIDI file (.mid)' }), file,
      midiBtn,
      el('div', { class: 'mSmall', text: 'A real MIDI keyboard works on a computer or Android with Chrome. iPhone Safari does not support MIDI keyboards yet.' }),
    ]);
    current = 'piano';
  }

  // Web MIDI: a real keyboard plays the piano (sit on the bench or not).
  var midiAccess = null;
  function connectMidi() {
    if (!navigator.requestMIDIAccess) return Promise.reject(new Error('This browser has no MIDI support (iPhone Safari does not). Try Chrome on a computer.'));
    return navigator.requestMIDIAccess().then(function (access) {
      midiAccess = access;
      var n = 0;
      access.inputs.forEach(function (input) {
        n++;
        input.onmidimessage = function (m) {
          var d = m.data;
          if (d.length >= 3) cmd('midi', d[0], d[1] * 1000 + d[2]);
        };
      });
      if (!n) throw new Error('No MIDI keyboard found. Plug it in and try again.');
      return n;
    });
  }

  function bartenderPanel() {
    var drinks = info.drinks || [];
    var a = window.bar3dAccount;
    var can = a && a.hasBadge && a.hasBadge('bartender');
    if (!can && /^localhost$|^127\./.test(location.hostname) && /[?&]badge=bartender/.test(location.search)) can = true;
    if (!can) {
      show('Bartender', [el('div', { text: 'Only players with the Bartender badge can make drinks. The bar’s owner gives badges (Profile → Give badges).' })]);
      return;
    }
    if (!info.shift) {
      show('Bartender', [el('div', { text: 'Walk behind the bar to the BARTENDER station and tap it to start your shift. Then come back here to make drinks.' })]);
      return;
    }
    show('Make a drink', [el('div', { class: 'mList' }, drinks.map(function (n, i) {
      return el('button', { onclick: function () { cmd('make', i); close(); } }, [el('span', { text: n }), el('span', { text: '\u{1F378}' })]);
    })), el('button', { class: 'mBtn ghost', text: 'End my shift', onclick: function () { cmd('endShift'); close(); } })]);
    current = 'bartender';
  }

  function helpPanel() {
    show('Help', [el('div', { style: 'white-space:pre-line', text:
      'Walk: left thumb.  Look: drag with the right thumb.\n' +
      'Tap things to use them: seats to sit, glasses to pick up, the jukebox, the piano, the darts, the racing sims, the 4K MANIA machine, the bell, the guitar, the light switch, the fireplace...\n' +
      'Stand up: move the joystick (or Menu → Stand up).\n' +
      'Voice chat: the mic button at the top. Settings: Menu → Voice chat.\n' +
      'Tap a person to see their card.' })]);
  }

  function toast(text) {
    if (!toastEl) return;
    toastEl.textContent = text;
    toastEl.style.display = 'block';
    clearTimeout(toastTimer);
    toastTimer = setTimeout(function () { toastEl.style.display = 'none'; }, 3500);
  }

  function init() {
    addStyles();
    root = el('div', { id: 'menuRoot' });
    root.addEventListener('click', function (e) { if (e.target === root) close(); });
    document.body.appendChild(root);
    toastEl = el('div', { id: 'menuToast' });
    document.body.appendChild(toastEl);
    var logBtn = document.getElementById('logToggle');
    if (logBtn) {
      var menuBtn = el('button', { id: 'hudMenu', text: '☰ Menu', onclick: function () { current ? close() : mainPanel(); } });
      var micBtn = el('button', { id: 'hudMic', text: '\u{1F3A4} off' });
      // Tap: mic on/off. With push to talk: hold it to talk.
      micBtn.addEventListener('click', function () {
        var V = window.bar3dVoice;
        if (!V || S.get('pushToTalk')) return;
        (V.micOn() ? V.micOff() : V.micStart()).catch(function (e) { toast(e.message || String(e)); });
      });
      ['touchstart', 'mousedown'].forEach(function (t) {
        micBtn.addEventListener(t, function () {
          var V = window.bar3dVoice;
          if (V && S.get('pushToTalk')) { if (!V.micOn()) V.micStart().catch(function () {}); V.setTalking(true); }
        });
      });
      ['touchend', 'touchcancel', 'mouseup', 'mouseleave'].forEach(function (t) {
        micBtn.addEventListener(t, function () { var V = window.bar3dVoice; if (V && S.get('pushToTalk')) V.setTalking(false); });
      });
      logBtn.parentNode.insertBefore(micBtn, logBtn);
      logBtn.parentNode.insertBefore(menuBtn, logBtn);
      setInterval(function () {
        var V = window.bar3dVoice;
        if (!V) return;
        var on = V.micOn();
        micBtn.textContent = on ? (V.gateOpen() ? '\u{1F3A4} talking' : '\u{1F3A4} on') : '\u{1F3A4} off';
        micBtn.className = on ? (V.gateOpen() ? 'live' : '') : 'muted';
      }, 150);
    }
    // Volumes follow the settings.
    function applyVolumes() {
      var A = window.bar3dAudio;
      if (!A) return;
      A.setBusVolume('music', S.get('musicVol'));
      A.setBusVolume('sfx', S.get('sfxVol'));
      A.setBusVolume('piano', S.get('pianoVol'));
      A.setBusVolume('voice', S.get('voiceVol'));
    }
    S.onChange(applyVolumes);
    applyVolumes();
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init); else init();

  return {
    take: function () { var out = JSON.stringify(queue); queue = []; return out; },
    setInfo: function (json) { try { info = JSON.parse(json); } catch (e) {} },
    toast: toast,
    open: function (name) {
      var panels = { main: mainPanel, emotes: emotesPanel, avatar: avatarPanel, settings: settingsPanel, voice: voicePanel,
                     piano: pianoPanel, bartender: bartenderPanel, help: helpPanel };
      if (name === 'bartenderShift') { info.shift = 1; name = 'bartender'; }
      if (panels[name]) panels[name]();
    },
    isOpen: function () { return !!current; },
    command: cmd,
  };
})();
