// The in-game menus, as HTML panels over the game (like the account
// screens): a Menu button in the top bar opens Emotes, Avatar, Profile,
// Voice chat, Settings, Piano songs, and (for bartenders) the drink list.
// C++ reads the settings (bar3dSettings) and gets the menus' commands through
// the page mailbox (bar3dBridge.command); it tells the menus what's going on
// (bar3dUi.setInfo, only when something changed). Only one panel covers the
// game at a time (bar3dModal, bar/web/bridge.js).
window.bar3dSettings = (function () {
  'use strict';
  // Every setting: [default, min, max] for numbers, the default alone for
  // on/off. Saved in localStorage 'bar3dSettings'; anything there that
  // doesn't fit (wrong type, out of range, broken JSON) falls back to the
  // default with one warning in the log.
  var SCHEMA = {
    look: [0, 0, 7], lookSpeed: [1, 0.3, 2.5], invertLook: false, showNames: true,
    musicVol: [0.8, 0, 1], sfxVol: [0.9, 0, 1], pianoVol: [0.9, 0, 1], voiceVol: [1, 0, 2],
    // Voice: the mic only sends when you're louder than micGate (dB);
    // micGain boosts a quiet mic.
    micGate: [-50, -80, -10], micGain: [1, 0.5, 4], pushToTalk: false,
    echoCancel: true, noiseSuppress: true, autoGain: false,
    hq: true, stereo: false, voice3d: true,
    perf: false,  // performance overlay (looped/profiler)
    // Graphics: quality 0 Auto, 1 High, 2 Medium, 3 Low (looped/quality.h); fov: degrees, up/down.
    quality: [0, 0, 3], fov: [70, 50, 100], mirrorQ: [0, 0, 1], lighting: [0, 0, 1],
  };
  function defaultOf(k) { var d = SCHEMA[k]; return Array.isArray(d) ? d[0] : d; }
  // The value if it fits the schema, else undefined.
  function check(k, v) {
    var d = SCHEMA[k];
    if (d === undefined) return undefined;
    if (Array.isArray(d)) return typeof v === 'number' && isFinite(v) ? Math.min(d[2], Math.max(d[1], v)) : undefined;
    return typeof v === 'boolean' ? v : undefined;
  }
  var values = {}, version = 1, listeners = [], problems = [];
  try {
    var saved = JSON.parse(localStorage.getItem('bar3dSettings') || '{}');
    if (!saved || typeof saved !== 'object' || Array.isArray(saved)) throw new Error('not an object');
    for (var k in saved) {
      var v = check(k, saved[k]);
      if (v === undefined) problems.push(k);
      else values[k] = v;
    }
  } catch (e) {
    values = {};
    problems.push('all (' + e.message + ')');
  }
  function save() { try { localStorage.setItem('bar3dSettings', JSON.stringify(values)); } catch (e) {} }
  if (problems.length) {
    var msg = 'Settings: reset to defaults: ' + problems.join(', ') + '.';
    if (window.bar3dLog) bar3dLog.add('warn', msg); else console.warn(msg);
    save();
  }
  return {
    get: function (k) { return k in values ? values[k] : defaultOf(k); },
    set: function (k, v) {
      var ok = check(k, v);
      if (ok === undefined) return;
      values[k] = ok;
      version++;
      save();
      listeners.forEach(function (l) { l(k, ok); });
    },
    reset: function () { values = {}; version++; save(); listeners.forEach(function (l) { l('*'); }); },
    version: function () { return version; },
    onChange: function (l) { listeners.push(l); },
  };
})();

window.bar3dUi = (function () {
  'use strict';
  var S = window.bar3dSettings;
  var info = {}, root = null, toastEl = null, toastTimer = 0, current = '';
  // 3D-rendered icons (tools/assets/icons.py), in the emote list's order.
  var EMOTE_ICONS = ['wave', 'dance', 'clap', 'cheer', 'bow', 'point', 'shrug', 'laugh', 'salute', 'thumbsup', 'facepalm'];
  // The menu's keys use the look kit's line icons (ui/icons.svg, S4.3); the
  // emotes keep their 3D pictures.
  var LINE_ICONS = { menu_emotes: 'i-hand', menu_avatar: 'i-mirror', menu_profile: 'i-people', menu_voice: 'i-sound',
    menu_settings: 'i-gear', menu_piano: 'i-note', menu_bartender: 'i-bar', menu_help: 'i-note-claude',
    menu_standup: 'i-arrow', menu_home: 'i-home', menu_places: 'i-place' };
  function icon(name) {
    if (LINE_ICONS[name]) {
      var NS = 'http://www.w3.org/2000/svg', svg = document.createElementNS(NS, 'svg'), use = document.createElementNS(NS, 'use');
      svg.setAttribute('class', 'crIcon'); svg.setAttribute('aria-hidden', 'true');
      use.setAttribute('href', 'ui/icons.svg#' + LINE_ICONS[name]);
      svg.appendChild(use);
      return svg;
    }
    return el('img', { class: 'crIcon', src: 'icons/' + name + '.png', alt: '', draggable: 'false' });
  }
  var LOOK_COLORS = ['#232326', '#ececE6', '#0d0d0d', '#bf1a1a', '#141a38', '#ff40a6', '#2e6138', '#5a5e6b'];

  function cmd(name, a, b, text, text2) { if (window.bar3dBridge) bar3dBridge.command(name, a, b, text, text2); }
  // A button that answers even if the browser drops the click after a
  // finger lifts on it (an open bug: after some games the Menu button and a
  // panel's ✕ stopped answering on the phone). A short tap that gets no
  // click within half a second runs the handler anyway, and says so in the
  // log (so a report shows it).
  function onTap(e, fn) {
    var down = null, pending = 0;
    e.addEventListener('pointerdown', function (ev) { down = { x: ev.clientX, y: ev.clientY }; });
    e.addEventListener('pointerup', function (ev) {
      if (!down || Math.abs(ev.clientX - down.x) > 10 || Math.abs(ev.clientY - down.y) > 10) { down = null; return; }
      down = null;
      clearTimeout(pending);
      pending = setTimeout(function () {
        pending = 0;
        if (!e.isConnected) return;
        if (window.bar3dLog) bar3dLog.add('warn', 'Menu: a tap on "' + (e.textContent || e.id).slice(0, 24) + '" got no click; handled it anyway.');
        fn(ev);
      }, 500);
    });
    e.addEventListener('click', function (ev) {
      if (pending) { clearTimeout(pending); pending = 0; }
      fn(ev);
    });
  }
  function el(tag, attrs, children) {
    var e = document.createElement(tag);
    for (var k in attrs || {}) {
      if (k === 'style') e.style.cssText = attrs[k];
      else if (k === 'text') e.textContent = attrs[k];
      else if (k === 'onclick' && tag === 'button') onTap(e, attrs[k]);
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
      '#menuRoot.avatarEdit{background:none;pointer-events:none;}',
      '#menuRoot.avatarEdit .mPanel{pointer-events:auto;max-height:45vh;}',
      '.avTabs{display:flex;gap:6px;overflow-x:auto;margin:0 0 8px;-webkit-overflow-scrolling:touch;}',
      '.avTab{flex:none;padding:7px 12px;border-radius:12px;border:2px solid #241a2b;background:#3a74db;color:#fff;font-size:15px;}',
      '.avTab.on{background:#f5b400;color:#241a2b;}',
      '.avChips{display:flex;flex-wrap:wrap;gap:6px;}',
      '.avChip{padding:8px 11px;border-radius:12px;border:2px solid #241a2b;background:#6aa8ff;color:#fff;font-size:14px;}',
      '.avChip.on{background:#ffe066;color:#241a2b;}',
      '.avSwatches{display:flex;flex-wrap:wrap;gap:7px;}',
      '.avSwatch{width:38px;height:38px;border-radius:50%;border:3px solid #241a2b;box-shadow:inset 0 3px 0 #fff6,0 3px 0 #241a2b;}',
      '.avSwatch.on{outline:3px solid #ffe066;outline-offset:2px;}',
      '.avDot{display:block;width:34px;height:34px;border-radius:50%;margin:4px auto 6px;border:3px solid #241a2b;}',
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
      '.mChoice{display:flex;gap:6px;margin:6px 0 4px;}',
      '.mChoice button{flex:1;padding:8px 0;border-radius:10px;border:1px solid #4a3a55;background:#2a1f33;color:#f5ecf8;',
      'font:600 13px -apple-system,system-ui;}',
      '.mChoice button.on{border-color:#ff5ab4;background:#3d2040;}',
      '.mMeter{position:relative;height:16px;border-radius:8px;background:#2a1f33;overflow:hidden;margin:6px 0;}',
      '.mMeter .lvl{position:absolute;left:0;top:0;bottom:0;background:linear-gradient(90deg,#5ad1ff,#7dff8a);}',
      '.mMeter .gate{position:absolute;top:0;bottom:0;width:3px;background:#ff5ab4;}',
      '.mList button{display:flex;justify-content:space-between;width:100%;text-align:left;margin:4px 0;padding:10px 12px;',
      'border-radius:10px;border:1px solid #4a3a55;background:#2a1f33;color:#f5ecf8;font:14px -apple-system,system-ui;}',
      '.mList button.on{border-color:#ff5ab4;background:#3d2040;}',
      '#menuToast{position:fixed;left:50%;top:calc(42px + env(safe-area-inset-top));transform:translateX(-50%);z-index:9;',
      'background:#1a1220ee;color:#f5ecf8;padding:8px 14px;border-radius:14px;font:14px -apple-system,system-ui;',
      'max-width:86vw;text-align:center;display:none;white-space:pre-line;pointer-events:none;}',
      '#hudMenu,#hudMic{pointer-events:auto;font:inherit;color:inherit;background:#0008;border:1px solid #f5d9a855;',
      'border-radius:6px;padding:4px 8px;margin-right:6px;}',
      '#hudMic.live{background:#2d7a3a;border-color:#7dff8a;}#hudMic.muted{background:#7a2d3a;}',
    ].join('');
    document.head.appendChild(el('style', { text: css }));
  }

  function close() {
    if (current === 'avatar') cmd('avatarPreview', 0);
    if (root) { root.classList.remove('open', 'avatarEdit'); root.innerHTML = ''; }
    current = '';
    stopMeter();
    if (window.bar3dModal) bar3dModal.closed('menu');
  }
  function show(title, children) {
    if (window.bar3dModal) bar3dModal.open('menu', close);
    if (current === 'avatar' && title !== 'Avatar') cmd('avatarPreview', 0);
    root.classList.remove('avatarEdit');
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
  // One of a few options (a row of small buttons), for a number setting.
  function choice(label, key, names) {
    var row = el('div', { class: 'mChoice' });
    names.forEach(function (n, i) {
      row.appendChild(el('button', { class: S.get(key) === i ? 'on' : '', text: n, onclick: function () {
        S.set(key, i);
        [].forEach.call(row.children, function (b, j) { b.classList.toggle('on', j === i); });
      } }));
    });
    return el('div', {}, [el('label', { text: label }), row]);
  }

  // ---- Panels ----
  function mainPanel() {
    var tiles = [
      ['menu_emotes', 'Emotes', emotesPanel], ['menu_avatar', 'Avatar', avatarPanel], ['menu_profile', 'Profile', profilePanel],
      ['menu_voice', 'Voice chat', voicePanel], ['menu_settings', 'Settings', settingsPanel], ['menu_piano', 'Piano', pianoPanel],
      ['menu_bartender', 'Bartender', bartenderPanel], ['menu_help', 'Help', helpPanel],
    ];
    if (info.seated) tiles.push(['menu_standup', 'Stand up', function () { cmd('standUp'); close(); }]);
    // Travel between places (docs/ENGINE.md E7.3): the page reloads into the
    // other place, which has its own room.
    var url = new URL(location.href);
    var away = url.searchParams.get('place') && url.searchParams.get('place') !== 'bar';
    tiles.push(['menu_home', away ? 'Back to the bar' : 'Visit the test home', function () {
      if (away) { url.searchParams.delete('place'); url.searchParams.delete('room'); }
      else { url.searchParams.set('place', 'home'); url.searchParams.set('room', 'home'); }
      location.href = url.toString();
    }]);
    tiles.push(['menu_places', 'Places', placesPanel]);
    show('Menu', [walletLine(), el('div', { class: 'mGrid' }, tiles.map(function (t) {
      return el('button', { class: 'mTile', onclick: t[2] }, [icon(t[0]), t[1]]);
    }))]);
    current = 'main';
  }

  // Places (S6.4): the bar, the test home and every place made in the Studio
  // (the live world's places/*.json). Each has its own room.
  function placesPanel() {
    var list = el('div', { class: 'mList' }, [el('div', { class: 'mSmall', text: 'Looking for places…' })]);
    show('Places', [list]);
    current = 'places';
    var here = new URL(location.href).searchParams.get('place') || 'bar';
    var go = function (id) {
      var url = new URL(location.href);
      if (id === 'bar') { url.searchParams.delete('place'); url.searchParams.delete('room'); }
      else { url.searchParams.set('place', id); url.searchParams.set('room', id === 'home' ? 'home' : 'place-' + id); }
      location.href = url.toString();
    };
    var draw = function (ids) {
      list.textContent = '';
      ids.forEach(function (id) {
        var b = el('button', { class: id === here ? 'on' : '', onclick: function () { if (id !== here) go(id); } },
          [el('span', { text: id === 'bar' ? 'The bar' : id === 'home' ? 'The test home' : id }), el('span', { text: id === here ? 'here' : '' })]);
        b.dataset.place = id;
        list.appendChild(b);
      });
    };
    draw(['bar', 'home']);
    fetch('../studio/api.php?live', { cache: 'no-store' }).then(function (r) { return r.json(); }).then(function (live) {
      if (!live || !(live.version > 0)) return;
      return fetch('../studio/api.php?bundle=' + live.version).then(function (r) { return r.json(); }).then(function (b) {
        var ids = ['bar', 'home'];
        Object.keys((b && b.files) || {}).forEach(function (p) {
          var m = /^places\/([a-z0-9-]+)\.json$/.exec(p);
          if (m && ids.indexOf(m[1]) < 0) ids.push(m[1]);
        });
        if (current === 'places') draw(ids);
      });
    }).catch(function () {});
  }

  // Your credits (cloud/ledger.js): shown when you're logged in.
  // Credits as people read them ("5", "47.50"), with the coin (bar/web/icons/credit.png).
  function credits(cents) { return cents % 100 === 0 ? String(cents / 100) : (cents / 100).toFixed(2); }
  function coin(cents) {
    return el('span', { class: 'mCredits' }, [el('img', { src: 'icons/credit.png', alt: '', class: 'mCoin', draggable: 'false' }),
      el('b', { text: credits(cents) })]);
  }
  function walletLine() {
    var line = el('div', { class: 'mSmall', style: 'text-align:center;margin:0 0 8px;font-size:15px;' });
    var A = window.bar3dAccount, N = window.bar3dNet;
    var token = A && A.token ? A.token() : '';
    var base = N && N.cloudHttp ? N.cloudHttp() : '';
    if (!token) { line.textContent = 'Log in (Profile) to earn credits in games.'; return line; }
    if (!base) return line;
    line.textContent = 'Credits: …';
    fetch(base + '/bank', { method: 'POST', body: JSON.stringify({ a: 'me', token: token }),
                            headers: { 'Content-Type': 'text/plain' }, cache: 'no-store' })
      .then(function (r) { return r.json(); })
      .then(function (w) {
        if (!w || !w.ok) { line.textContent = ''; return; }
        line.textContent = '';
        line.appendChild(coin(w.balance));
        line.appendChild(el('span', { text: ' credits  ·  earned today ' + credits(w.earnedToday) + ' of ' + credits(w.cap) }));
      })
      .catch(function () { line.textContent = ''; });
    return line;
  }

  function emotesPanel() {
    var names = info.emotes || ['Wave', 'Dance', 'Clap', 'Cheer', 'Bow', 'Point', 'Shrug', 'Laugh', 'Salute', 'Thumbs up', 'Facepalm'];
    show('Emotes', [el('div', { class: 'mGrid' }, names.map(function (n, i) {
      return el('button', { class: 'mTile', onclick: function () { cmd('emote', i + 1); close(); } },
                [icon('emote_' + (EMOTE_ICONS[i] || 'wave')), n]);
    })), el('div', { class: 'mSmall', text: 'Others see your emote. Dancing goes on until you walk.' })]);
    current = 'emotes';
  }

  // ---- Avatar: presets and the customizer (bar/avatar.h AvatarStyle) ----
  // The 17 choices, in the game's order.
  var AV = { body: 0, skin: 1, hair: 2, hairColor: 3, hiColor: 4, hi: 5, top: 6, topColor: 7, topColor2: 8, topPattern: 9,
             bottom: 10, bottomColor: 11, bottomColor2: 12, bottomPattern: 13, shoes: 14, eyes: 15, facial: 16 };
  var AV_DEFAULT = [0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 1];
  var AV_TABS = [
    ['Presets', null],
    ['Body', [['Body', 'body', 'opt', ['Masculine', 'Feminine']], ['Skin', 'skin', 'skin'], ['Eyes', 'eyes', 'eyes'],
              ['Facial hair', 'facial', 'opt', ['None', 'Stubble', 'Beard']]]],
    ['Hair', [['Style', 'hair', 'opt', ['Curly', 'Short', 'Long', 'Bun', 'Bald', 'Ponytail', 'Bob', 'Mohawk', 'Afro', 'Buzz']],
              ['Colour', 'hairColor', 'hair'], ['Highlights', 'hi', 'opt', ['None', 'Tips', 'Streaks', 'Roots']],
              ['Highlight colour', 'hiColor', 'hair']]],
    ['Top', [['Style', 'top', 'opt', ['T-shirt', 'Long sleeve', 'Tank top', 'Hoodie', 'Jacket']], ['Colour', 'topColor', 'cloth'],
             ['Pattern', 'topPattern', 'opt', ['Solid', 'Stripes', 'Gradient', 'Two-tone']], ['Second colour', 'topColor2', 'cloth']]],
    ['Bottom', [['Style', 'bottom', 'opt', ['Jeans', 'Shorts', 'Skirt', 'Joggers']], ['Colour', 'bottomColor', 'cloth'],
                ['Pattern', 'bottomPattern', 'opt', ['Solid', 'Stripes', 'Gradient', 'Two-tone']],
                ['Second colour', 'bottomColor2', 'cloth'], ['Shoes', 'shoes', 'cloth']]],
  ];
  var avatarTab = 'Presets';
  function avatarStyle() {
    var st = (info.avatarStyle && info.avatarStyle.length === AV_DEFAULT.length) ? info.avatarStyle.slice() : AV_DEFAULT.slice();
    return st;
  }
  function avatarPanel() {
    var custom = info.look === -1;
    var tabs = el('div', { class: 'avTabs' }, AV_TABS.map(function (t) {
      return el('button', { class: 'avTab' + (avatarTab === t[0] ? ' on' : ''), text: t[0], onclick: function () {
        avatarTab = t[0];
        avatarPanel();
      } });
    }));
    var body = [];
    var tab = AV_TABS.filter(function (t) { return t[0] === avatarTab; })[0] || AV_TABS[0];
    if (!tab[1]) {
      var names = info.looks || ['Josh'];
      body.push(el('div', { class: 'mGrid' }, names.map(function (n, i) {
        var swatch = el('span', { class: 'avDot', style: 'background:' + (LOOK_COLORS[i] || '#888') });
        return el('button', { class: 'mTile' + (!custom && S.get('look') === i ? ' on' : ''), onclick: function () {
          S.set('look', i);  // remembered for next time; the game changes it from the command
          cmd('look', i);
          info.look = i;
          avatarPanel();
        } }, [swatch, n]);
      })));
      body.push(el('button', { class: 'mBtn', text: custom ? 'Edit my own look' : 'Make my own look', onclick: function () {
        avatarTab = 'Body';
        if (!custom) setStyle(avatarStyle());
        avatarPanel();
      } }));
    } else {
      var st = avatarStyle();
      var pal = info.avatarPalette || {};
      tab[1].forEach(function (row) {
        var field = AV[row[1]], kind = row[2];
        // Second colours only matter with a pattern; highlight colour only with highlights.
        if (row[1] === 'topColor2' && !st[AV.topPattern]) return;
        if (row[1] === 'bottomColor2' && !st[AV.bottomPattern]) return;
        if (row[1] === 'hiColor' && !st[AV.hi]) return;
        body.push(el('h3', { text: row[0] }));
        var items;
        if (kind === 'opt') {
          items = row[3].map(function (n, i) {
            return el('button', { class: 'avChip' + (st[field] === i ? ' on' : ''), text: n, onclick: function () { pick(field, i); } });
          });
        } else {
          items = (pal[kind] || []).map(function (c, i) {
            return el('button', { class: 'avSwatch' + (st[field] === i ? ' on' : ''), style: 'background:' + c,
                                  'aria-label': row[0] + ' ' + (i + 1), onclick: function () { pick(field, i); } });
          });
        }
        body.push(el('div', { class: kind === 'opt' ? 'avChips' : 'avSwatches' }, items));
      });
      body.push(el('div', { class: 'mSmall', text: 'Drag on the game to turn around. Everyone sees your look.' }));
    }
    show('Avatar', [tabs, el('div', { class: 'avBody' }, body)]);
    root.classList.add('avatarEdit');
    if (current !== 'avatar') cmd('avatarPreview', 1);
    current = 'avatar';
  }
  function setStyle(st) {
    info.avatarStyle = st;
    info.look = -1;
    cmd('avatarStyle', 0, 0, JSON.stringify(st));
  }
  function pick(field, value) {
    var st = avatarStyle();
    st[field] = value;
    // A beard on the feminine body by default goes away; pick it again if wanted.
    if (field === AV.body && value === 1 && st[AV.facial] === 1) st[AV.facial] = 0;
    setStyle(st);
    avatarPanel();
  }

  function profilePanel() {
    close();
    var a = window.bar3dAccount;
    if (a && a.openProfile) a.openProfile();
    else toast('Profiles need the multiplayer server.');
  }

  // Send report: the log and performance numbers go to the server; the code
  // it answers with is what you paste in the chat (index.html bar3dReport).
  function reportBox() {
    var out = el('div', { class: 'mReport' });
    out.style.cssText = 'font-size:22px;letter-spacing:3px;margin:6px 0;-webkit-user-select:text;user-select:text;';
    var btn = el('button', { class: 'mBtn', text: 'Send report', onclick: function () {
      if (!window.bar3dReport) return;
      btn.disabled = true;
      out.textContent = 'Sending…';
      bar3dReport.send(false).then(function (code) {
        out.textContent = 'Code: ' + code;
      }).catch(function (e) {
        out.textContent = 'Could not send: ' + e.message;
      }).then(function () { btn.disabled = false; });
    } });
    var box = el('div', {});
    box.appendChild(btn);
    box.appendChild(out);
    return box;
  }

  function settingsPanel() {
    show('Settings', [
      el('h3', { text: 'Controls' }),
      slider('Look speed', 'lookSpeed', 0.3, 2.5, 0.1, function (v) { return v.toFixed(1) + 'x'; }),
      toggle('Invert look up/down', 'invertLook'),
      el('h3', { text: 'Screen' }),
      toggle('Show name tags', 'showNames'),
      toggle('Performance overlay', 'perf'),
      el('h3', { text: 'Graphics' }),
      choice('Quality', 'quality', ['Auto', 'High', 'Medium', 'Low']),
      choice('Lighting', 'lighting', ['New', 'Classic']),
      el('div', { class: 'mSmall', text: 'Auto draws a little less sharp when your phone is struggling, and sharp again when it isn\u2019t.' }),
      choice('Mirror', 'mirrorQ', ['Full', 'Simple']),
      el('div', { class: 'mSmall', text: 'Simple: a softer reflection without small things far away (people and things near the glass stay).' }),
      slider('Field of view', 'fov', 50, 100, 1, function (v) { return Math.round(v) + '\u00b0'; }),
      el('h3', { text: 'Sound' }),
      slider('Music', 'musicVol', 0, 1, 0.05, pct),
      slider('Effects', 'sfxVol', 0, 1, 0.05, pct),
      slider('Piano', 'pianoVol', 0, 1, 0.05, pct),
      slider('Voices', 'voiceVol', 0, 2, 0.05, pct),
      el('h3', { text: 'Help Claude fix lag and bugs' }),
      reportBox(),
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
                [el('span', { text: (i < 10 ? (i + 1) + '. ' : 'MY SONG: ') + n }), el('span', { text: '▶' })]);
    }));
    var file = el('input', { type: 'file', accept: '.mid,.midi,audio/midi,audio/x-midi', id: 'midiPicker', style: 'display:none' });
    file.onchange = function () {
      Array.prototype.forEach.call(file.files, function (f) {
        cmd('pianoAdd', 0, 0, f.name.replace(/\.[^.]+$/, ''), URL.createObjectURL(f));
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

  // The racing rigs: pick which game to play.
  function simPanel() {
    show('Racing rig', [
      el('div', { class: 'mSmall', text: 'Pick a game. You stay in the bar meanwhile: the others see you sitting at the rig.' }),
      el('button', { class: 'mBtn', text: 'Race: pick a car, race the other rig or bots', onclick: function () { close(); cmd('simRace'); } }),
      el('button', { class: 'mBtn', text: 'Skid pad: try the new driving physics (Ridgeback GS)', onclick: function () { close(); cmd('simPad'); } }),
      el('button', { class: 'mBtn', text: 'Meridian: an endless drive', onclick: function () { close(); openMeridian(); } }),
      el('button', { class: 'mBtn ghost', text: 'Stand up', onclick: function () { close(); cmd('standUp'); } }),
    ]);
    current = 'sim';
  }

  // Car setup: the Ridgeback GS on the new driving physics (looped/drive),
  // changed live while you drive. Only what you changed is kept (localStorage
  // bar3dCarSetup) and sent; everything else stays stock.
  var CAR_SETUP_KEY = 'bar3dCarSetup';
  var times = function (v) { return Math.round(v * 100) + '%'; };
  var CAR_SETUP = [  // [group, key, label, min, max, step, stock, format]
    ['Tires', 'grip', 'Grip', 0.6, 1.4, 0.02, 1, times],
    ['Springs', 'springF', 'Front springs', 0.5, 2, 0.05, 1, times],
    ['Springs', 'springR', 'Rear springs', 0.5, 2, 0.05, 1, times],
    ['Dampers', 'damperF', 'Front dampers', 0.5, 2, 0.05, 1, times],
    ['Dampers', 'damperR', 'Rear dampers', 0.5, 2, 0.05, 1, times],
    ['Anti-roll bars', 'arbF', 'Front bar (more = less front grip)', 0, 2.5, 0.05, 1, times],
    ['Anti-roll bars', 'arbR', 'Rear bar (more = looser rear)', 0, 2.5, 0.05, 1, times],
    ['Brakes', 'brakeBias', 'Brake bias (front share)', 0.5, 0.75, 0.01, 0.62, times],
    ['Brakes', 'handbrake', 'Handbrake strength', 0.5, 3, 0.05, 1, times],
    ['Differential', 'diffLock', 'Diff lock (0 = open)', 0, 1, 0.02, 0.4, times],
    ['Steering', 'steerRatio', 'Steering ratio (lower = quicker)', 9, 18, 0.1, 12.5, function (v) { return (+v).toFixed(1) + ':1'; }],
    ['Steering', 'steerAssist', 'Steering assist (0 = direct)', 0, 1, 0.05, 1, times],
    ['Steering', 'steerWeight', 'Self-centring force', 0, 2, 0.05, 1, times],
    ['Steering', 'handStrength', 'Your grip on the wheel', 0.4, 2.5, 0.05, 1, times],
    ['Assists', 'tc', 'Traction control (0 = off, drift)', 0, 1, 0.05, 0.2, times],
    ['View', 'cameraDistance', 'Camera distance', 0.7, 1.6, 0.05, 1, times],
  ];
  var CAR_SWITCHES = [  // [key, label, stock]
    ['abs', 'ABS', true], ['telemetry', 'Telemetry (grip circle, tires)', false], ['oldPhysics', 'Old physics (to compare)', false],
  ];
  function carSetupLoad() {
    try {
      var v = JSON.parse(localStorage.getItem(CAR_SETUP_KEY) || '{}');
      return v && typeof v === 'object' && !Array.isArray(v) ? v : {};
    } catch (e) { return {}; }
  }
  function carSetupSend(v) { cmd('carSetup', 0, 0, JSON.stringify(v)); }
  function carSetupPanel() {
    var v = carSetupLoad();
    function save() {
      try { localStorage.setItem(CAR_SETUP_KEY, JSON.stringify(v)); } catch (e) {}
      carSetupSend(v);
    }
    var rows = [el('div', { class: 'mSmall', text: 'Ridgeback GS, new physics. Changes apply at once and are saved on this phone.' })];
    var group = '';
    CAR_SETUP.forEach(function (d) {
      if (d[0] !== group) { group = d[0]; rows.push(el('h3', { text: group })); }
      var key = d[1], cur = key in v ? v[key] : d[6];
      var val = el('span', { class: 'val', text: d[7](cur) });
      var r = el('input', { type: 'range', min: d[3], max: d[4], step: d[5], value: cur });
      r.oninput = function () { v[key] = +r.value; val.textContent = d[7](+r.value); save(); };
      rows.push(el('div', { class: 'mRow' }, [el('label', { text: d[2] }), r, val]));
    });
    rows.push(el('h3', { text: 'Switches' }));
    CAR_SWITCHES.forEach(function (d) {
      var key = d[0], on = key in v ? !!v[key] : d[2];
      var b = el('button', { class: 'mToggle' + (on ? ' on' : '') });
      b.onclick = function () { on = !on; v[key] = on; b.classList.toggle('on', on); save(); };
      rows.push(el('div', { class: 'mRow' }, [el('label', { text: d[1] }), b]));
    });
    rows.push(el('button', { class: 'mBtn ghost', text: 'Reset to stock', onclick: function () { v = {}; save(); carSetupPanel(); } }));
    rows.push(el('button', { class: 'mBtn', text: 'Back to driving', onclick: close }));
    show('Car setup', rows);
    current = 'carSetup';
  }

  // Full-screen games over the bar: Meridian (the user's driving game,
  // bar/projects/meridian/, from a racing rig) and the arcade machines (bar/projects/platformer/ at arcade/platformer/:
  // the platformer, the user's "Test 2"). The bar keeps its connection; its
  // music and sounds go quiet (voice chat stays on) and it stops drawing
  // until you come back.
  var GAMES = {
    meridian: { src: 'meridian/index.html', title: 'Meridian', after: function () { simPanel(); } },
    platformer: { src: 'arcade/platformer/index.html', title: 'Platformer', after: null },
  };
  var meridian = null;  // the game that has the screen ({frame, back, kind})
  function openGame(kind) {
    var g = GAMES[kind];
    if (meridian || !g) return;
    var A = window.bar3dAudio;
    if (A) ['music', 'sfx', 'piano'].forEach(function (b) { A.setBusVolume(b, 0); });
    var frame = el('iframe', { src: g.src + '?v=' + (document.getElementById('build') || {}).textContent,
                              allow: 'autoplay; fullscreen; gamepad', title: g.title });
    frame.style.cssText = 'position:fixed;inset:0;width:100%;height:100%;border:0;z-index:2000;background:#000;';
    var back = el('button', { text: '\u2190 Back to the bar', onclick: closeGame });
    // Top middle: the games' own displays use the top corners.
    back.style.cssText = 'position:fixed;z-index:2001;left:50%;transform:translateX(-50%);top:calc(8px + env(safe-area-inset-top));' +
      'padding:6px 12px;border-radius:10px;border:1px solid rgba(255,255,255,.3);background:rgba(10,8,16,.55);color:#fff;font:600 13px system-ui;';
    document.body.appendChild(frame);
    document.body.appendChild(back);
    meridian = { frame: frame, back: back, kind: kind };
    cmd('meridianOpen');
    if (window.bar3dLog) bar3dLog.add('info', g.title + ': opened.');
  }
  function closeGame() {
    if (!meridian) return;
    var g = GAMES[meridian.kind];
    meridian.frame.remove();
    meridian.back.remove();
    meridian = null;
    applyVolumes();
    cmd('meridianClosed');
    if (window.bar3dLog) bar3dLog.add('info', g.title + ': closed, back in the bar.');
    if (g.after) g.after();  // (Meridian: still sitting at the rig: race, drive again or stand up)
  }
  function openMeridian() { openGame('meridian'); }
  function closeMeridian() { closeGame(); }

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
      return el('button', { onclick: function () { cmd('make', i); close(); } }, [el('span', { text: n }), (function () { var i = icon('menu_bartender'); i.style.cssText = 'width:24px;height:24px;margin:0'; return i; })()]);
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
      var micBtn = el('button', { id: 'hudMic', text: 'MIC OFF' });
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
      var micText = '', micClass = null;
      setInterval(function () {  // the page is only touched when the state changes
        var V = window.bar3dVoice;
        if (!V) return;
        var on = V.micOn(), live = on && V.gateOpen();
        var text = on ? (live ? 'TALKING' : 'MIC ON') : 'MIC OFF';
        var cls = on ? (live ? 'live' : '') : 'muted';
        if (text !== micText) micBtn.textContent = micText = text;
        if (cls !== micClass) micBtn.className = micClass = cls;
      }, 150);
    }
    S.onChange(applyVolumes);
    applyVolumes();
    // Your saved car setup goes to the game (again a little later, in case
    // the game wasn't listening yet).
    var setup = carSetupLoad();
    if (Object.keys(setup).length) {
      carSetupSend(setup);
      setTimeout(function () { carSetupSend(carSetupLoad()); }, 4000);
    }
  }
  // Volumes follow the settings (not while Meridian has the screen).
  function applyVolumes() {
      var A = window.bar3dAudio;
      if (!A || meridian) return;
      A.setBusVolume('music', S.get('musicVol'));
      A.setBusVolume('sfx', S.get('sfxVol'));
      A.setBusVolume('piano', S.get('pianoVol'));
      A.setBusVolume('voice', S.get('voiceVol'));
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init); else init();

  return {
    // What the game says is going on: an object whose keys replace the old ones.
    setInfo: function (changes) {
      if (typeof changes === 'string') {
        try { changes = JSON.parse(changes); } catch (e) {
          if (window.bar3dLog) bar3dLog.add('error', 'Menu: bad info from the game: ' + e.message);
          return;
        }
      }
      for (var k in changes) info[k] = changes[k];
    },
    toast: toast,
    open: function (name) {
      var panels = { main: mainPanel, emotes: emotesPanel, avatar: avatarPanel, settings: settingsPanel, voice: voicePanel,
                     piano: pianoPanel, bartender: bartenderPanel, help: helpPanel, sim: simPanel, carSetup: carSetupPanel };
      if (name === 'bartenderShift') { info.shift = 1; name = 'bartender'; }
      if (GAMES[name] && name !== 'meridian') { close(); openGame(name); return; }  // an arcade machine
      if (panels[name]) panels[name]();
    },
    isOpen: function () { return !!current; },
    meridianOpen: function () { return !!meridian; },
    openMeridian: openMeridian,
    closeMeridian: closeMeridian,
    gameOpen: function () { return meridian ? meridian.kind : ''; },
    closeGame: closeGame,
    info: function () { return info; },  // tests
    close: close,
    command: cmd,
  };
})();
