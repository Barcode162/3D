// Thin bridge from the C++ multiplayer code (engine/net.cpp) to the
// multiplayer server. Two ways to reach it:
// - Cloudflare (cloud/worker.js), the main one: one live WebSocket; your
//   state goes out up to 20 times a second (only when it changed) and
//   others' states arrive the moment they're sent. Its address comes from
//   cloud.json, written by the deploy.
// - The PHP relay on IONOS (server/relay.php), the fallback: polled about
//   10 times a second. Used when Cloudflare can't be reached.
// What the numbers mean, smoothing and drawing are all decided in C++.
//
// Page options: ?room=name (default "main"), ?name=Josh (your name tag,
// remembered), ?server=https://.../relay.php (force the relay),
// ?cloud=wss://... (force a Cloudflare address), ?cloud=off, ?server=off.
// Overrides are honoured only for localhost or the exact hosts in
// ALLOWED_HOSTS (so a link can't send your login token somewhere else).
window.bar3dNet = (function () {
  'use strict';
  // Where the relay lives when the game is served from GitHub Pages. When
  // the page itself is on the IONOS site, relay.php sits right next to it.
  var DEFAULT_SERVER = 'https://joshuavelasco.com/bar/relay.php';
  var SEND_INTERVAL_MS = 100;
  // Exact hosts a ?server= override may point at (never a suffix: anyone can
  // make a *.workers.dev name). ?cloud= overrides only accept localhost until
  // the room is deployed (restructure step 3.3 adds its workers.dev host).
  var ALLOWED_HOSTS = ['joshuavelasco.com'];
  var ALLOWED_CLOUD_HOSTS = [];

  function hostOf(value) {
    try { return new URL(value, location.href).hostname; } catch (e) { return null; }
  }
  function isLocal(h) { return h === 'localhost' || h === '127.0.0.1'; }
  // Whether an override value may be used.
  function overrideOk(value, list) {
    if (value === 'off') return true;
    var h = hostOf(value);
    return !!h && (isLocal(h) || list.indexOf(h) >= 0);
  }
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

  var serverParam = param('server'), cloudOverride = param('cloud'), ignored = [];
  if (serverParam !== null && !overrideOk(serverParam, ALLOWED_HOSTS)) {
    ignored.push(serverParam);
    serverParam = null;
  }
  if (cloudOverride !== null && !overrideOk(cloudOverride, ALLOWED_CLOUD_HOSTS)) {
    ignored.push(cloudOverride);
    cloudOverride = null;
  }
  ignored.forEach(function (v) { console.log('Multiplayer: ignored server option (' + v + ')'); });
  var forcedRelay = serverParam !== null;
  var server = serverParam;
  if (server === null) {
    var host = location.hostname;
    server = /github\.io$/.test(host) ? DEFAULT_SERVER : /^localhost$|^127\./.test(host) ? '' : 'relay.php';
  }
  if (server === 'off') server = '';
  var room = (param('room') || 'main').toLowerCase().replace(/[^a-z0-9-]/g, '').slice(0, 32) || 'main';
  var name = param('name');
  if (name) store('bar3dName', name); else name = store('bar3dName');
  if (!name) name = 'Guest ' + (100 + Math.floor(Math.random() * 900));
  var id = 'p' + Math.random().toString(36).slice(2, 10);

  var state = null, stateTime = 0;  // latest from C++
  var players = {};                 // id -> {index, name}
  var nextIndex = 1;
  var packets = [];                 // new states not yet taken by C++
  var lastSent = {};                // id -> t, to forward only new states
  var status = server ? 1 : 0;      // 0 off, 1 connecting, 2 connected, 3 failing
  var inFlight = false, failures = 0, started = false;
  // Shared world (see server/relay.php): our changes waiting to be sent,
  // entries from others not yet taken by C++, the newest revision we have,
  // and the server's clock relative to ours.
  var worldOut = {}, worldIn = [], since = 0;
  // Messages through the relay (voice chat handshakes, events for players
  // without a direct connection), and quick events waiting for C++.
  var msgOut = [], eventsIn = [], signalHandlers = [];
  var clockOffset = null, clockRtt = 1e9;

  // --- Cloudflare (WebSocket) mode.
  var cloudUrl = null, ws = null, wsOpen = false, wsFails = 0, mode = 'relay';
  var lastStateText = '', lastStateSent = 0, pingAt = 0, helloTok = '';
  var CLOUD_SEND_MS = 50;

  function wsSend(obj) {
    if (ws && wsOpen) { try { ws.send(JSON.stringify(obj)); return true; } catch (e) {} }
    return false;
  }
  function addPlayer(p) {
    var known = players[p.id];
    if (!known) {
      known = players[p.id] = { index: nextIndex++, name: p.name };
      info('Multiplayer: ' + p.name + ' joined.');
    }
    known.name = p.name;
    known.acct = p.acct || '';
    known.av = p.av || 0;
    if (p.s && p.s.length && lastSent[p.id] !== p.t) {
      lastSent[p.id] = p.t;
      packets.push({ index: known.index, t: p.t, s: p.s });
    }
  }
  function dropPlayer(pid) {
    if (!players[pid]) return;
    info('Multiplayer: ' + players[pid].name + ' left.');
    delete players[pid];
    delete lastSent[pid];
  }
  function connectCloud() {
    var url = cloudUrl.replace(/\/$/, '') + '/room/' + room + '?id=' + id;
    try { ws = new WebSocket(url); } catch (e) { ws = null; cloudFailed(); return; }
    ws.onopen = function () {
      wsOpen = true;
      wsFails = 0;
      var hello = { hello: 1, name: name, ping: performance.now() };
      var acct = window.bar3dAccount;
      helloTok = acct && acct.token() ? acct.token() : '';
      if (helloTok) hello.tok = helloTok;
      pingAt = performance.now();
      ws.send(JSON.stringify(hello));
      lastStateText = '';
    };
    ws.onmessage = function (e) {
      var msg;
      try { msg = JSON.parse(e.data); } catch (err) { return; }
      if (msg.welcome) {
        if (status !== 2) info('Multiplayer: connected to room "' + room + '" as ' + (msg.name || name) + ' (Cloudflare).');
        status = 2;
        // Anyone we knew who isn't in the room any more has left.
        var here = {};
        msg.players.forEach(function (p) { here[p.id] = true; });
        Object.keys(players).forEach(function (pid) { if (!here[pid]) dropPlayer(pid); });
        msg.players.forEach(addPlayer);
        msg.world.forEach(function (w) { worldIn.push([w[0], w[1] < id ? 1 : 0, w[2]]); });
      }
      if ('pong' in msg) {
        var rtt = performance.now() - msg.pong;
        clockRtt *= 1.01;
        if (rtt < clockRtt) {
          clockRtt = rtt;
          clockOffset = msg.now - (msg.pong + rtt / 2) / 1000;
        }
      }
      if (msg.p) addPlayer(msg.p);
      if (msg.left) dropPlayer(msg.left);
      if (msg.w) msg.w.forEach(function (w) { worldIn.push([w[0], w[1] < id ? 1 : 0, w[2]]); });
      if (msg.m) receiveText(msg.m[0], msg.m[1]);
    };
    ws.onclose = function () {
      var was = wsOpen;
      wsOpen = false;
      ws = null;
      if (was) info('Multiplayer: connection lost, reconnecting...');
      cloudFailed();
    };
  }
  function cloudFailed() {
    if (++wsFails >= 3 && status !== 2 && server) {
      // Never got through: use the relay on IONOS instead.
      info('Multiplayer: Cloudflare unreachable, using the IONOS relay.');
      mode = 'relay';
      setInterval(tick, SEND_INTERVAL_MS);
      return;
    }
    if (wsFails >= 3) status = 3;
    setTimeout(connectCloud, Math.min(8000, 500 * Math.pow(2, wsFails)));
  }
  function cloudTick() {
    if (!wsOpen) return;
    // Logged in or out since connecting: reconnect so the room knows.
    var acct = window.bar3dAccount;
    if ((acct && acct.token() ? acct.token() : '') !== helloTok) { try { ws.close(); } catch (e) {} return; }
    var now = performance.now();
    var out = {};
    if (state) {
      var text = JSON.stringify(state);
      // Only when it changed (or now and then, so others know you're here).
      if (text !== lastStateText || now - lastStateSent > 2000) {
        out.t = stateTime;
        out.s = state;
        lastStateText = text;
        lastStateSent = now;
      }
    }
    if (Object.keys(worldOut).length) {
      out.w = worldOut;
      worldOut = {};
    }
    var msgs = msgOut.splice(0, 40);
    if (msgs.length) out.m = msgs;
    if (now - pingAt > 2000) {
      out.ping = now;
      pingAt = now;
    }
    if (Object.keys(out).length && !wsSend(out)) {
      msgOut = msgs.concat(msgOut);
    }
  }

  function info(text) {
    if (window.Module && Module.print) Module.print('[info] ' + text);
  }

  function post(body, onDone) {
    fetch(server, { method: 'POST', body: JSON.stringify(body), cache: 'no-store',
                    headers: { 'Content-Type': 'text/plain' } })
      .then(function (r) { return r.json(); })
      .then(function (reply) { onDone(reply && reply.ok ? reply : null); })
      .catch(function () { onDone(null); });
  }

  function tick() {
    if (inFlight || !state) return;
    inFlight = true;
    var body = { room: room, id: id, name: name, t: stateTime, s: state, since: since };
    var acct = window.bar3dAccount;
    if (acct && acct.token() && tokenOk(server)) body.tok = acct.token();  // the relay shows your account's name and picture
    var sending = worldOut;
    worldOut = {};
    if (Object.keys(sending).length) body.w = sending;
    var sendingMsgs = msgOut.splice(0, 40);
    if (sendingMsgs.length) body.m = sendingMsgs;
    var sentAt = performance.now();
    post(body, function (reply) {
      inFlight = false;
      if (!reply) {
        // Put unsent changes back, unless a newer one for the same key came.
        for (var k in sending) if (!(k in worldOut)) worldOut[k] = sending[k];
        msgOut = sendingMsgs.concat(msgOut);
        if (++failures === 5) { status = 3; info('Multiplayer: can\'t reach the server (' + server + ').'); }
        return;
      }
      if (typeof reply.rev === 'number') {
        // The fastest round trip tells the server's clock best; slowly
        // forget old samples so a changed network is picked up.
        var rtt = performance.now() - sentAt;
        clockRtt *= 1.01;
        if (rtt < clockRtt) {
          clockRtt = rtt;
          clockOffset = reply.now - (sentAt + rtt / 2) / 1000;
        }
        since = reply.rev;
        reply.world.forEach(function (e) { worldIn.push([e[0], e[1] < id ? 1 : 0, e[2]]); });
      }
      if (status !== 2) info('Multiplayer: connected to room "' + room + '" as ' + name + '.');
      if (status === 3) info('Multiplayer: server reachable again.');
      status = 2;
      failures = 0;
      var seen = {};
      reply.players.forEach(function (p) {
        seen[p.id] = true;
        var known = players[p.id];
        if (!known) {
          known = players[p.id] = { index: nextIndex++, name: p.name };
          info('Multiplayer: ' + p.name + ' joined.');
        }
        known.name = p.name;
        known.acct = p.acct || '';
        known.av = p.av || 0;
        if (lastSent[p.id] === p.t) return;
        lastSent[p.id] = p.t;
        packets.push({ index: known.index, t: p.t, s: p.s });
      });
      (reply.inbox || []).forEach(function (m) { receiveText(m[0], m[1]); });
      Object.keys(players).forEach(function (pid) {
        if (seen[pid]) return;
        info('Multiplayer: ' + players[pid].name + ' left.');
        delete players[pid];
        delete lastSent[pid];
      });
    });
  }

  // A message from another player: a voice handshake ('v:') or an event ('e:').
  function receiveText(from, text) {
    if (text.slice(0, 2) === 'v:') {
      var data;
      try { data = JSON.parse(text.slice(2)); } catch (e) { return; }
      signalHandlers.forEach(function (h) { h(from, data); });
    } else if (text.slice(0, 2) === 'e:') {
      var p = players[from];
      if (!p) return;
      try {
        var ev = JSON.parse(text.slice(2));
        eventsIn.push([p.index, ev[0], ev[1]]);
      } catch (e) {}
    }
  }

  function leave() {
    if (mode === 'cloud') { if (ws) try { ws.close(); } catch (e) {} return; }
    if (!server || !started) return;
    try {
      navigator.sendBeacon(server, JSON.stringify({ room: room, id: id, leave: 1 }));
    } catch (e) {}
  }

  return {
    // Starts sending/receiving. Returns false when multiplayer is off.
    start: function () {
      if (started) return !!(server || cloudUrl !== 'off');
      ignored.forEach(function (v) { info('Multiplayer: ignored server option (' + v + ')'); });
      var cloudParam = cloudOverride;
      if (serverParam === 'off') return false;
      started = true;
      window.addEventListener('pagehide', leave);
      // Cloudflare first (unless the relay was asked for), else the relay.
      var useRelay = function () {
        if (!server) { status = 0; return; }
        mode = 'relay';
        setInterval(tick, SEND_INTERVAL_MS);
      };
      var useCloud = function (url) {
        cloudUrl = url;
        mode = 'cloud';
        status = 1;
        connectCloud();
        setInterval(cloudTick, CLOUD_SEND_MS);
      };
      if (cloudParam === 'off' || forcedRelay) {
        useRelay();
      } else if (cloudParam) {
        useCloud(cloudParam);
      } else {
        fetch('cloud.json', { cache: 'no-store' }).then(function (r) { return r.json(); })
          .then(function (c) { if (c && c.ws) useCloud(c.ws); else useRelay(); })
          .catch(useRelay);
      }
      return true;
    },
    status: function () { return status; },
    room: function () { return room; },
    // Your latest state; t is your clock in ms (sent along so the others
    // can play your movement back smoothly).
    setState: function (t, values) { stateTime = t; state = values; },
    // Moves new states to C++: each packet is [player, t, count, values...]
    // as doubles. Returns how many packets were written.
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
    // Index of each player currently in the room (others only).
    present: function (index) {
      for (var pid in players) if (players[pid].index === index) return true;
      return false;
    },
    name: function (index) {
      for (var pid in players) if (players[pid].index === index) return players[pid].name;
      return '';
    },
    // Their account id ('' = guest) and profile picture revision (0 = none).
    account: function (index) {
      for (var pid in players) if (players[pid].index === index) return players[pid].acct || '';
      return '';
    },
    avatarRev: function (index) {
      for (var pid in players) if (players[pid].index === index) return players[pid].av || 0;
      return 0;
    },
    serverUrl: function () { return server; },
    // Whether a login token may be sent to this URL (same origin or allowlisted).
    tokenOk: tokenOk,
    myId: function () { return id; },
    // Ids of everyone else in the room right now.
    peers: function () { return Object.keys(players); },
    idOf: function (index) {
      for (var pid in players) if (players[pid].index === index) return pid;
      return '';
    },
    indexOf: function (pid) { return players[pid] ? players[pid].index : 0; },
    // True if that player's id sorts before ours (tie-breaks: who wins a seat).
    isLower: function (index) {
      for (var pid in players) if (players[pid].index === index) return pid < id;
      return false;
    },
    // Voice chat handshakes (web/voice.js).
    sendSignal: function (to, data) { if (server || mode === 'cloud') msgOut.push([to, 'v:' + JSON.stringify(data)]); },
    onSignal: function (handler) { signalHandlers.push(handler); },
    // Quick events (piano notes...) to everyone: straight over the voice
    // chat's data connection where there is one, else through the relay.
    sendEvent: function (type, values) {
      var text = 'e:' + JSON.stringify([type, values]);
      var voice = window.bar3dVoice;
      for (var pid in players) {
        if (!(voice && voice.sendData(pid, text))) msgOut.push([pid, text]);
      }
    },
    receiveText: receiveText,
    // Events from others since the last call, as JSON [[player, type, [values]], ...].
    takeEvents: function () {
      var out = JSON.stringify(eventsIn);
      eventsIn = [];
      return out;
    },
    count: function () { return Object.keys(players).length; },
    // Shared world: queue a change to `key` (sent on the next tick).
    setWorld: function (key, values) {
      worldOut[key] = values.map(function (v) { return Math.round(v * 10000) / 10000; });
    },
    // Changes from others since the last call, as JSON:
    // [[key, writerIdIsLowerThanOurs, [values...]], ...]
    takeWorld: function () {
      var out = JSON.stringify(worldIn);
      worldIn = [];
      return out;
    },
    // The server's clock in seconds (shared by everyone), 0 if not known yet.
    serverTime: function () {
      return clockOffset === null ? 0 : performance.now() / 1000 + clockOffset;
    },
  };
})();
