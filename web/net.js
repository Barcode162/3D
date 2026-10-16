// Thin bridge from the C++ multiplayer code (engine/net.cpp) to the relay
// server (server/relay.php on the IONOS web hosting). About 10 times a
// second it sends your latest state and receives everyone else's. What the
// numbers mean, smoothing and drawing are all decided in C++.
//
// Page options: ?room=name (default "main"), ?name=Josh (your name tag,
// remembered), ?server=https://.../relay.php, ?server=off.
window.bar3dNet = (function () {
  'use strict';
  // Where the relay lives when the game is served from GitHub Pages. When
  // the page itself is on the IONOS site, relay.php sits right next to it.
  var DEFAULT_SERVER = 'https://joshuavelasco.com/bar/relay.php';
  var SEND_INTERVAL_MS = 100;

  function param(name) {
    var m = new RegExp('[?&]' + name + '=([^&]*)').exec(location.search);
    return m ? decodeURIComponent(m[1]) : null;
  }
  function store(key, value) {
    try { if (value === undefined) return localStorage.getItem(key); localStorage.setItem(key, value); } catch (e) {}
    return null;
  }

  var server = param('server');
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
  var clockOffset = null, clockRtt = 1e9;

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
    if (acct && acct.token()) body.tok = acct.token();  // the relay shows your account's name and picture
    var sending = worldOut;
    worldOut = {};
    if (Object.keys(sending).length) body.w = sending;
    var sentAt = performance.now();
    post(body, function (reply) {
      inFlight = false;
      if (!reply) {
        // Put unsent changes back, unless a newer one for the same key came.
        for (var k in sending) if (!(k in worldOut)) worldOut[k] = sending[k];
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
      Object.keys(players).forEach(function (pid) {
        if (seen[pid]) return;
        info('Multiplayer: ' + players[pid].name + ' left.');
        delete players[pid];
        delete lastSent[pid];
      });
    });
  }

  function leave() {
    if (!server || !started) return;
    try {
      navigator.sendBeacon(server, JSON.stringify({ room: room, id: id, leave: 1 }));
    } catch (e) {}
  }

  return {
    // Starts sending/receiving. Returns false when multiplayer is off.
    start: function () {
      if (!server || started) return !!server;
      started = true;
      setInterval(tick, SEND_INTERVAL_MS);
      window.addEventListener('pagehide', leave);
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
