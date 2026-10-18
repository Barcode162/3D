// Thin bridge from the C++ multiplayer code (engine/net.cpp) to the room on
// Cloudflare (cloud/room.js; protocol in docs/PROTOCOL.md). One WebSocket
// carries everything:
// - your state (where you are, what you're doing) as a small binary STATE,
//   sent up to 20 times a second and only when it changed;
// - everyone else's states, arriving in the room's TICKs (20 a second while
//   someone moves) stamped with the server's clock;
// - the shared world (glasses, jukebox, mirrors...): every write goes through
//   the room, which numbers it and sends it to everyone, so all phones end up
//   with the same value;
// - claims: the room decides who sits where (one person per seat);
// - quick events (piano keys, sounds), in order, stamped by the room;
// - voice chat handshakes (SIGNAL).
// The binary layouts are encoded/decoded by proto.js (the same code the room
// runs, copied into the site by the build). What the numbers mean, smoothing
// and drawing are all decided in C++.
//
// Where the room is: config.json (written by the deploy). Page options:
// ?room=name (default "main"), ?name=Josh, ?cloud=ws://localhost:8787 (local
// tests; only localhost and the hosts below), ?cloud=off or ?net=off. On
// localhost multiplayer is off unless ?cloud= is given (tests stay alone).
window.bar3dNet = (function () {
  'use strict';
  var PROTO = 1;
  // The accounts and reports servers (index.html and account.js take these
  // from serverUrl(): "...relay.php" → "...account.php" / "...report.php").
  var DEFAULT_SERVER = 'https://joshuavelasco.com/bar/relay.php';
  // Exact hosts a ?cloud= override may point at (never a suffix: anyone can
  // make a *.workers.dev name).
  var ALLOWED_HOSTS = ['joshuavelasco.com'];
  var ALLOWED_CLOUD_HOSTS = ['bar3d.joshuavelasco60.workers.dev'];
  var SEND_MS = 50;           // STATE at most 20 times a second
  var KEEPALIVE_MS = 5000;    // a plain "ping" when nothing else was sent
  var DEAD_MS = 12000;        // nothing at all received this long: the socket is dead
  var RETRY_MS = [250, 1000, 2000, 4000, 8000];
  var GAP_MS = 150;           // a player who was still this long gets a "hold" sample first

  function hostOf(value) {
    try { return new URL(value, location.href).hostname; } catch (e) { return null; }
  }
  function isLocal(h) { return h === 'localhost' || h === '127.0.0.1'; }
  // Whether a login token may be sent to this URL: same origin or allowlisted.
  function tokenOk(value) {
    try {
      var u = new URL(value, location.href);
      return u.origin === location.origin || isLocal(u.hostname) ||
        ALLOWED_HOSTS.indexOf(u.hostname) >= 0 || ALLOWED_CLOUD_HOSTS.indexOf(u.hostname) >= 0;
    } catch (e) { return false; }
  }
  function param(name) {
    var m = new RegExp('[?&]' + name + '=([^&]*)').exec(location.search);
    return m ? decodeURIComponent(m[1]) : null;
  }
  function store(key, value) {
    try { if (value === undefined) return localStorage.getItem(key); localStorage.setItem(key, value); } catch (e) {}
    return null;
  }
  function info(text) {
    if (window.Module && Module.print) Module.print('[info] ' + text);
  }

  var local = isLocal(location.hostname);
  var cloudParam = param('cloud'), netParam = param('net');
  if (cloudParam !== null && cloudParam !== 'off') {
    var ch = hostOf(cloudParam);
    if (!ch || !(isLocal(ch) || ALLOWED_CLOUD_HOSTS.indexOf(ch) >= 0)) {
      console.log('Multiplayer: ignored server option (' + cloudParam + ')');
      cloudParam = null;
    }
  }
  var disabled = netParam === 'off' || cloudParam === 'off' || (local && !cloudParam);
  // The accounts server: next to the game on IONOS, else joshuavelasco.com.
  var server = /github\.io$/.test(location.hostname) ? DEFAULT_SERVER : local ? '' : 'relay.php';
  var room = (param('room') || 'main').toLowerCase().replace(/[^a-z0-9-]/g, '').slice(0, 32) || 'main';
  var name = param('name');
  if (name) store('bar3dName', name); else name = store('bar3dName');
  if (!name) name = 'Guest ' + (100 + Math.floor(Math.random() * 900));
  var buildCode = (function () { var b = document.getElementById('build'); return b ? b.textContent.replace(/^build\s*/, '').slice(0, 16) : ''; })();

  // ---- Connection state ----
  var P = null;               // proto.js (window.bar3dProto)
  var url = '', ws = null, open = false, started = false;
  var status = disabled ? 0 : 1;  // 0 off, 1 connecting, 2 connected, 3 offline
  var why = '';               // last reason we're not connected (for the log)
  var stopped = false;        // no automatic reconnect (full, new version, rate limit...)
  var retries = 0, retryTimer = 0;
  var mySlot = -1, resume = '', helloTok = '';
  var lastRecv = 0, lastSend = 0;

  // ---- Your state ----
  var state = null;           // latest 20 numbers from C++
  var awaySlot = -1, awayBit = 0, away = false;
  var stateSeq = 0, lastStateHex = '', stateHz = 20, lastStateAt = 0;

  // ---- Others ----
  var players = {};           // pid -> {slot, index, name, acct, av, away, last: {t, s}}
  var bySlot = {};            // slot -> pid
  var nextIndex = 1, gen = 0, rosterVer = 0;
  var packets = [];           // new states not yet taken by C++
  var eventsIn = [], eventSeq = {};  // per slot: newest event seq seen
  var signalHandlers = [];

  // ---- Shared world ----
  var worldOut = {}, worldIn = [];
  var pending = {};           // key -> our writes not yet echoed back
  var foreign = {};           // key -> someone else's write arrived since our last one
  var eventsOut = [];
  var claims = {};            // key -> slot
  var intentId = 1, intentsOut = [], results = {};  // request id -> {ok, why, by}

  // ---- Clock (server ms = performance.now() + clockOffset) ----
  var clockOffset = null, clockRtt = 1e9, pingTimer = 0, pingsLeft = 0, lastPingAt = 0;

  function pad(n) { return n < 10 ? '0' + n : '' + n; }
  function pidOf(slot) { return bySlot[slot] || ''; }
  function serverNowMs() { return clockOffset === null ? 0 : performance.now() + clockOffset; }

  function send(data) {
    if (!ws || !open) return false;
    try { ws.send(data); lastSend = performance.now(); return true; } catch (e) { return false; }
  }
  function sendText(letter, body) {
    var t = P.encodeText(letter, body, 'toServer');
    if (t === null) { console.log('Multiplayer: not sent (invalid ' + letter + ')', body); return false; }
    return send(t);
  }
  function ticket() {
    var acct = window.bar3dAccount;
    return acct && acct.token && acct.token() ? acct.token() : '';
  }

  // ---- Connecting ----
  function connect() {
    if (stopped || !url) return;
    clearTimeout(retryTimer);
    status = status === 2 ? 1 : status;
    try { ws = new WebSocket(url.replace(/\/$/, '') + '/room/' + room); } catch (e) { ws = null; lost('could not open'); return; }
    ws.binaryType = 'arraybuffer';
    var me = ws;
    ws.onopen = function () {
      if (ws !== me) return;
      open = true;
      lastRecv = performance.now();
      helloTok = ticket();
      var hello = { proto: PROTO, build: buildCode, name: name.slice(0, 24) || 'Guest', look: state ? Math.max(0, Math.min(255, state[7] | 0)) : 0 };
      if (resume) hello.resume = resume;
      if (helloTok) hello.ticket = helloTok;
      sendText('H', hello);
    };
    ws.onmessage = function (e) { if (ws === me) receive(e.data); };
    ws.onclose = function (e) { if (ws === me) closed(e.code, e.reason); };
    ws.onerror = function () {};
  }

  function closed(code, reason) {
    ws = null;
    open = false;
    clearInterval(pingTimer);
    var wasOnline = status === 2;
    if (code === 4001) return stop('Room full (' + (reason || '16 players') + ').');
    if (code === 4002) { stop('A new version is out: reloading when it\'s ready.'); waitForNewVersion(); return; }
    if (code === 4003) return stop('This bar is open in another tab.');
    if (code === 4005) return stop('Disconnected (too many messages).');
    if (code === 4004) {
      // Idle too long: rejoin on the next tap (no automatic retry).
      stop(/away/.test(reason || '') ? 'Away for a while: tap to rejoin.' : 'Connection went quiet: tap to rejoin.');
      resume = '';
      var again = function () {
        document.removeEventListener('pointerdown', again, true);
        stopped = false;
        retries = 0;
        info('Multiplayer: rejoining...');
        connect();
      };
      document.addEventListener('pointerdown', again, true);
      return;
    }
    lost(wasOnline ? 'connection lost' : 'can\'t reach the room');
  }

  function lost(reason) {
    why = reason;
    if (status === 2) info('Multiplayer: ' + reason + ', reconnecting...');
    var delay = RETRY_MS[Math.min(retries, RETRY_MS.length - 1)];
    if (++retries >= 4 && status !== 3) { status = 3; info('Multiplayer: offline (' + reason + '); still trying.'); }
    else if (status !== 3) status = 1;
    retryTimer = setTimeout(connect, delay);
  }

  function stop(text) {
    stopped = true;
    status = 3;
    why = text;
    info('Multiplayer: ' + text);
  }

  // After a "new version" close: wait until the site has the new build, then
  // reload once (GitHub Pages can serve the old page for a few minutes).
  function waitForNewVersion() {
    var check = function () {
      fetch('version.txt?t=' + Date.now(), { cache: 'no-store' }).then(function (r) { return r.text(); }).then(function (v) {
        v = v.trim();
        if (v && buildCode && v !== buildCode) location.reload();
        else setTimeout(check, 30000);
      }).catch(function () { setTimeout(check, 30000); });
    };
    setTimeout(check, 5000);
  }

  // ---- Receiving ----
  function receive(data) {
    lastRecv = performance.now();
    if (typeof data !== 'string') {
      var d = P.decodeBinary(data);
      if (d.ok && d.type === 'tick') onTick(d.msg);
      return;
    }
    if (data === 'pong') return;  // keepalive answer
    var m = P.decodeText(data, 'toClient');
    if (!m.ok) { console.log('Multiplayer: dropped a message (' + m.why + ')'); return; }
    var b = m.body;
    switch (m.letter) {
      case 'W': onWelcome(b); break;
      case 'J': addPlayer(b); break;
      case 'L': dropPlayer(b.slot); break;
      case 'F': onProfile(b); break;
      case 'P': onPong(b); break;
      case 'K': onKv(b.w, false); break;
      case 'E': onEvents(b.e); break;
      case 'D': onDelta(b.d); break;
      case 'R': results[b.r] = b; break;
      case 'G': {
        var from = pidOf(b.from);
        if (from) signalHandlers.forEach(function (h) { try { h(from, b.data); } catch (e) {} });
        break;
      }
      default: break;
    }
  }

  function onWelcome(b) {
    var resumed = !!resume && mySlot === b.slot;
    mySlot = b.slot;
    resume = b.resume;
    retries = 0;
    stopped = false;
    if (status !== 2) info('Multiplayer: connected to room "' + b.room + '" as ' + name + ' (Cloudflare, slot ' + b.slot + ').');
    status = 2;
    why = '';
    // Who's here now: anyone we knew who isn't any more has left.
    var here = {};
    b.roster.forEach(function (p) { if (p.slot !== mySlot) here[p.slot] = true; });
    Object.keys(bySlot).forEach(function (s) { if (!here[s]) dropPlayer(+s); });
    b.roster.forEach(function (p) { if (p.slot !== mySlot) addPlayer(p); });
    claims = {};
    for (var k in b.claims) claims[k] = b.claims[k];
    // The world as the room has it. Our own entries are kept as we have them
    // (they're ours, and at least as new).
    pending = {};
    foreign = {};
    onKv(b.kv || [], resumed);
    if (!resumed) { stateSeq = 0; lastStateHex = ''; }
    lastStateHex = '';  // send our state again at once
    // Clock: a short burst of pings, then one every 10 s.
    clockRtt = 1e9;
    pingsLeft = 5;
    clearInterval(pingTimer);
    pingTimer = setInterval(function () {
      if (pingsLeft > 0) { pingsLeft--; ping(); }
      else if (performance.now() - lastPingAt > 10000) ping();
    }, 200);
    ping();
  }

  function ping() { lastPingAt = performance.now(); sendText('P', { c: lastPingAt }); }

  function onPong(b) {
    var now = performance.now();
    var rtt = now - b.c;
    if (rtt < 0 || rtt > 10000) return;
    clockRtt *= 1.02;  // slowly forget old samples
    if (rtt <= clockRtt) {
      clockRtt = rtt;
      clockOffset = b.s + rtt / 2 - now;
    }
  }

  function addPlayer(p) {
    if (p.slot === mySlot) return;
    var pid = bySlot[p.slot];
    var known = pid && players[pid];
    if (!known) {
      pid = 's' + pad(p.slot) + '_' + (++gen);
      known = players[pid] = { slot: p.slot, index: nextIndex++, name: p.name, acct: p.acct || '', av: p.av || 0, away: !!p.away, last: null };
      bySlot[p.slot] = pid;
      info('Multiplayer: ' + p.name + ' joined.');
    } else {
      known.name = p.name;
      known.acct = p.acct || '';
      known.av = p.av || 0;
      known.away = !!p.away;
    }
    rosterVer++;
  }

  function dropPlayer(slot) {
    var pid = bySlot[slot];
    if (!pid) return;
    info('Multiplayer: ' + players[pid].name + ' left.');
    delete players[pid];
    delete bySlot[slot];
    delete eventSeq[slot];
    rosterVer++;
  }

  function onProfile(b) {
    if (b.slot === mySlot) return;
    var p = players[pidOf(b.slot)];
    if (!p) return;
    p.name = b.name;
    p.acct = b.acct || '';
    p.av = b.av || 0;
    rosterVer++;
  }

  // Avatar block + activity (PROTOCOL §3) ↔ the 20 numbers C++ uses
  // (app/players.h slot list).
  var FLAG_BARTENDER = 1, FLAG_MIC = 2, FLAG_AWAY = 4;  // C++ kFlags bits
  function encodeState(s) {
    var flags = s[17] | 0, voice = +s[8] || 0;
    var avatar = {
      x: +s[0] || 0, y: +s[1] || 0, z: +s[2] || 0, yaw: +s[3] || 0, pitch: +s[4] || 0,
      anim: clampInt(s[9], 0, 255), animTick: (s[10] | 0) & 0xffff, held: clampInt(s[16], 0, 255), look: clampInt(s[7], 0, 255),
      flags: (voice > 0.05 ? 1 : 0) | (flags & FLAG_AWAY ? 2 : 0) | (flags & FLAG_BARTENDER ? 4 : 0) | (flags & FLAG_MIC ? 8 : 0),
    };
    // Activity TLV kind 1 (the bar's activity): activity u8, param i16,
    // voice u8, 5 × f32 activity data = 24 bytes.
    var b = new DataView(new ArrayBuffer(26));
    b.setUint8(0, 1);
    b.setUint8(1, 24);
    b.setUint8(2, clampInt(s[5], 0, 255));
    b.setInt16(3, clampInt(s[6], -32768, 32767), true);
    b.setUint8(5, clampInt(Math.round(voice * 255), 0, 255));
    for (var i = 0; i < 5; i++) b.setFloat32(6 + i * 4, isFinite(s[11 + i]) ? +s[11 + i] : 0, true);
    var hex = '';
    for (var j = 0; j < 26; j++) { var x = b.getUint8(j); hex += (x < 16 ? '0' : '') + x.toString(16); }
    return { avatar: avatar, act: hex };
  }
  function clampInt(v, lo, hi) { v = Math.round(+v || 0); return v < lo ? lo : v > hi ? hi : v; }

  function decodeState(avatar, act) {
    var s = [avatar.x, avatar.y, avatar.z, avatar.yaw > Math.PI ? avatar.yaw - 2 * Math.PI : avatar.yaw, avatar.pitch,
             0, 0, avatar.look, avatar.flags & 1 ? 0.5 : 0, avatar.anim, avatar.animTick, 0, 0, 0, 0, 0, avatar.held,
             (avatar.flags & 4 ? FLAG_BARTENDER : 0) | (avatar.flags & 8 ? FLAG_MIC : 0) | (avatar.flags & 2 ? FLAG_AWAY : 0)];
    // Walk the TLV for kind 1.
    var bytes = [];
    for (var i = 0; i + 1 < act.length; i += 2) bytes.push(parseInt(act.substr(i, 2), 16));
    var p = 0;
    while (p + 2 <= bytes.length) {
      var kind = bytes[p], n = bytes[p + 1];
      if (kind === 1 && n === 24 && p + 2 + n <= bytes.length) {
        var dv = new DataView(new Uint8Array(bytes.slice(p + 2, p + 2 + n)).buffer);
        s[5] = dv.getUint8(0);
        s[6] = dv.getInt16(1, true);
        s[8] = dv.getUint8(3) / 255;
        for (var k = 0; k < 5; k++) s[11 + k] = dv.getFloat32(4 + k * 4, true);
      }
      p += 2 + n;
    }
    return s;  // the 18 numbers of app/players.h (kCount)
  }

  function pushPacket(p, t, s) {
    packets.push({ index: p.index, t: t, s: s });
    if (packets.length > 512) packets.splice(0, packets.length - 512);
    p.last = { t: t, s: s };
  }

  function onTick(t) {
    t.players.forEach(function (row) {
      var p = players[pidOf(row.slot)];
      if (!p) return;
      var s = decodeState(row.avatar, row.act);
      if (p.away) s[17] = s[17] | FLAG_AWAY;
      // After a still spell, start the move from where they stood (C++ plays
      // back between samples; without this the first step would be smeared
      // over the whole quiet time).
      if (p.last && t.serverMs - p.last.t > GAP_MS) pushPacket(p, t.serverMs - SEND_MS, p.last.s);
      pushPacket(p, t.serverMs, s);
    });
  }

  function onDelta(list) {
    list.forEach(function (d) {
      var field = d[1], value = d[2], by = d[3];
      if (field.slice(0, 6) === 'claim.') {
        var key = field.slice(6);
        if (value < 0) delete claims[key]; else claims[key] = value;
      } else if (field === 'away') {
        var p = players[pidOf(by)];
        if (!p) return;
        p.away = !!value;
        // Tell C++ through their state: same numbers, away bit changed.
        if (p.last) {
          var s = p.last.s.slice();
          s[17] = value ? (s[17] | FLAG_AWAY) : (s[17] & ~FLAG_AWAY);
          pushPacket(p, Math.max(p.last.t + 1, serverNowMs()), s);
        }
        rosterVer++;
      } else if (field === 'net.stateHz') {
        stateHz = Math.max(1, Math.min(20, value | 0));
        info('Multiplayer: the server is busy today, sending less often.');
      }
    });
  }

  // World writes from the room, in order. Our own writes come back too: we
  // skip them (we already have them) unless someone else's write to the same
  // key arrived after ours was sent; then our echo is the room's final word
  // and is applied, so every phone ends on the same value.
  function onKv(list, skipOwn) {
    list.forEach(function (e) {
      var key = e[1], by = e[2], values = e[3];
      if (by === mySlot && mySlot >= 0) {
        if (skipOwn) return;  // a resumed session: we already have our own writes
        if (pending[key] > 0) pending[key]--;
        if (!pending[key] && foreign[key]) {
          foreign[key] = false;
          worldIn.push([key, 0, values]);
        }
        return;
      }
      if (pending[key] > 0) foreign[key] = true;
      worldIn.push([key, by >= 0 && by < mySlot ? 1 : 0, values]);
    });
  }

  function onEvents(list) {
    list.forEach(function (e) {
      var slot = e[1], seq = e[2];
      if (eventSeq[slot] !== undefined && seq <= eventSeq[slot]) return;  // a repeat
      eventSeq[slot] = seq;
      var p = players[pidOf(slot)];
      if (p) eventsIn.push([p.index, e[3], e[4]]);
    });
  }

  // ---- Sending (every 50 ms) ----
  function tick() {
    if (!open || mySlot < 0) {
      if (open && performance.now() - lastRecv > DEAD_MS) { try { ws.close(); } catch (e) {} }
      return;
    }
    var now = performance.now();
    if (now - lastRecv > DEAD_MS && !document.hidden) {
      info('Multiplayer: no answer from the room, reconnecting...');
      var dead = ws;
      ws = null;
      open = false;
      try { dead.close(); } catch (e) {}
      closed(1006, 'dead');
      return;
    }
    // Logged in or out since connecting: tell the room.
    var tok = ticket();
    if (tok !== helloTok) { helloTok = tok; sendText('R', { ticket: tok || 'guest' }); }
    if (state && !away && now - lastStateAt >= 1000 / stateHz - 5) {
      var enc = encodeState(state);
      var hex = JSON.stringify(enc.avatar) + enc.act;
      if (hex !== lastStateHex) {
        stateSeq = (stateSeq + 1) & 0xffff;
        var bin = P.encodeState({ seq: stateSeq, avatar: enc.avatar, act: enc.act });
        if (bin && send(bin)) { lastStateHex = hex; lastStateAt = now; }
      }
    }
    var keys = Object.keys(worldOut);
    if (keys.length) {
      var w = {};
      keys.slice(0, 32).forEach(function (k) { w[k] = worldOut[k]; delete worldOut[k]; });
      if (sendText('K', { w: w })) {
        for (var k in w) { pending[k] = (pending[k] || 0) + 1; foreign[k] = false; }
      }
    }
    if (eventsOut.length) sendText('E', { e: eventsOut.splice(0, 32) });
    while (intentsOut.length) {
      if (!sendText('I', intentsOut[0])) break;
      intentsOut.shift();
    }
    if (now - lastSend > KEEPALIVE_MS) send('ping');
  }

  // ---- Background / foreground ----
  document.addEventListener('visibilitychange', function () {
    if (document.hidden && !away) {
      away = true;
      if (state && awaySlot >= 0) {
        var s = state.slice();
        s[awaySlot] = s[awaySlot] | awayBit;
        var enc = encodeState(s);
        stateSeq = (stateSeq + 1) & 0xffff;
        var bin = P && P.encodeState({ seq: stateSeq, avatar: enc.avatar, act: enc.act });
        if (bin) send(bin);
      }
      if (P) sendText('A', { on: 1 });
    } else if (!document.hidden && away) {
      away = false;
      lastStateHex = '';
      if (open) { sendText('A', { on: 0 }); pingsLeft = 3; }
      else if (!stopped && started) { retries = 0; connect(); }
    }
  });

  function leave() { if (ws) { try { ws.close(1000, 'left'); } catch (e) {} } }

  function begin() {
    P = window.bar3dProto;
    if (!P) { status = 3; info('Multiplayer: proto.js is missing.'); return; }
    window.addEventListener('pagehide', leave);
    setInterval(tick, SEND_MS);
    if (cloudParam) { url = cloudParam; connect(); return; }
    fetch('config.json?t=' + Date.now(), { cache: 'no-store' }).then(function (r) { return r.json(); }).then(function (c) {
      if (!c || !c.ws) { status = 0; info('Multiplayer: no room server in this build.'); return; }
      url = c.ws;
      connect();
    }).catch(function () { status = 3; info('Multiplayer: couldn\'t read config.json.'); });
  }

  return {
    // Starts sending/receiving. Returns false when multiplayer is off.
    start: function () {
      if (disabled) return false;
      if (!started) { started = true; begin(); }
      return true;
    },
    status: function () { return status; },
    statusReason: function () { return why; },
    room: function () { return room; },
    // Your latest state (C++ sends it every frame; the room gets it when it
    // changed, at most 20 times a second).
    setState: function (t, values) { if (!away) state = values; },
    // Which state number carries the "away" bit (set once by C++).
    awayFlag: function (slot, bit) { awaySlot = slot; awayBit = bit; },
    // Moves new states to C++: each packet is [player, t, count, values...]
    // as doubles (t = server ms). Returns how many packets were written.
    take: function (heap, start, capacity, maxValues) {
      var n = 0, at = start;
      while (packets.length && n < capacity) {
        var p = packets.shift(), count = Math.min(p.s.length, maxValues);
        heap[at] = p.index; heap[at + 1] = p.t; heap[at + 2] = count;
        for (var i = 0; i < count; i++) heap[at + 3 + i] = +p.s[i] || 0;
        at += 3 + maxValues;
        n++;
      }
      return n;
    },
    // Everyone else in the room, for the game (through the page mailbox):
    // [[index, name, account, picture revision, sorts before me], ...].
    rosterVersion: function () { return rosterVer; },
    rosterJson: function () {
      return JSON.stringify(Object.keys(players).map(function (pid) {
        var p = players[pid];
        return [p.index, p.name || '', p.acct || '', p.av || 0, p.slot < mySlot ? 1 : 0];
      }));
    },
    present: function (index) {
      for (var pid in players) if (players[pid].index === index) return true;
      return false;
    },
    name: function (index) {
      for (var pid in players) if (players[pid].index === index) return players[pid].name;
      return '';
    },
    account: function (index) {
      for (var pid in players) if (players[pid].index === index) return players[pid].acct || '';
      return '';
    },
    avatarRev: function (index) {
      for (var pid in players) if (players[pid].index === index) return players[pid].av || 0;
      return 0;
    },
    // Where the accounts and reports servers are (see DEFAULT_SERVER).
    serverUrl: function () { return server; },
    names: function () { return Object.keys(players).map(function (k) { return players[k].name; }); },
    tokenOk: tokenOk,
    myId: function () { return mySlot >= 0 ? 's' + pad(mySlot) : ''; },
    peers: function () { return Object.keys(players); },
    idOf: function (index) {
      for (var pid in players) if (players[pid].index === index) return pid;
      return '';
    },
    indexOf: function (pid) { return players[pid] ? players[pid].index : 0; },
    isLower: function (index) {
      for (var pid in players) if (players[pid].index === index) return players[pid].slot < mySlot;
      return false;
    },
    // Voice chat handshakes (web/voice.js), through the room.
    sendSignal: function (to, data) {
      var p = players[to];
      if (p && P) sendText('G', { to: p.slot, data: data });
    },
    onSignal: function (handler) { signalHandlers.push(handler); },
    // Quick events (piano notes, sounds) to everyone, through the room (in
    // order, stamped with the server's clock).
    sendEvent: function (type, values) {
      eventsOut.push([String(type).slice(0, 16), values.slice(0, 16).map(function (v) { return isFinite(v) ? +v : 0; })]);
      if (eventsOut.length > 64) eventsOut.shift();
    },
    // Old path: events over voice chat's data channel (not sent any more).
    receiveText: function () {},
    takeEvents: function () {
      var out = JSON.stringify(eventsIn);
      eventsIn = [];
      return out;
    },
    count: function () { return Object.keys(players).length; },
    // Shared world: queue a change to `key` (sent on the next tick).
    setWorld: function (key, values) {
      worldOut[key] = values.slice(0, 20).map(function (v) { return isFinite(v) ? Math.round(v * 10000) / 10000 : 0; });
    },
    // Changes since the last call, as JSON: [[key, writerSortsBeforeMe, [values...]], ...]
    takeWorld: function () {
      var out = JSON.stringify(worldIn);
      worldIn = [];
      return out;
    },
    // Claims (the room decides): ask for a key (a seat...), let it go, and
    // who has it: -1 nobody, 0 you, else that player's index.
    claim: function (key) {
      var i = intentId++;
      intentsOut.push({ i: i, k: 'claim', key: key });
      return i;
    },
    release: function (key) {
      var i = intentId++;
      intentsOut.push({ i: i, k: 'release', key: key });
      return i;
    },
    claimOwner: function (key) {
      if (!(key in claims)) return -1;
      if (claims[key] === mySlot) return 0;
      var p = players[pidOf(claims[key])];
      return p ? p.index : -1;
    },
    // The room's answer to a claim: 1 yes, 0 no, -1 not yet.
    result: function (id) {
      var r = results[id];
      if (!r) return -1;
      delete results[id];
      return r.ok;
    },
    // The server's clock in seconds (shared by everyone), 0 if not known yet.
    serverTime: function () { return clockOffset === null ? 0 : (performance.now() + clockOffset) / 1000; },
  };
})();
