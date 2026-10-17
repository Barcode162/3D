// Voice chat (and fast data) between players, phone to phone with WebRTC.
// The relay (server/relay.php) only carries the handshake: each pair of
// players swaps one offer and one answer through it (bar3dNet.sendSignal),
// then audio and data go straight between them (via Google's public STUN
// server to find a way through home routers).
//
// Audio: Opus, asked for at up to 510 kbps ("studio quality", which sounds
// lossless; WebRTC has no truly lossless codec). Your mic goes through a
// noise gate: it only sends while you're louder than the threshold you set.
// Voices are placed in 3D at each player's head (louder when close),
// unless you turn that off. A data channel next to the audio carries quick
// events (piano notes) with less delay than the relay.
window.bar3dVoice = (function () {
  'use strict';
  var S = window.bar3dSettings;
  var ICE = [{ urls: 'stun:stun.l.google.com:19302' }, { urls: 'stun:stun1.l.google.com:19302' }];
  var peers = {};         // id -> peer
  var mic = null;         // {stream, source, gain, analyser, gate, dest, track}
  var talking = false;    // push to talk held
  var quietTicks = 99, gateIsOpen = false, lastDb = -100;
  var monitorNode = null;
  var started = false;

  function info(t) { if (window.Module && Module.print) Module.print('[info] ' + t); }
  function net() { return window.bar3dNet; }
  function ctx() { return window.bar3dAudio && bar3dAudio.context(); }

  // ---- Microphone with gain, level meter and noise gate ----
  function constraints() {
    return { audio: { echoCancellation: !!S.get('echoCancel'), noiseSuppression: !!S.get('noiseSuppress'),
                      autoGainControl: !!S.get('autoGain'), channelCount: S.get('stereo') ? 2 : 1,
                      sampleRate: 48000 }, video: false };
  }
  function micStart() {
    if (mic) return Promise.resolve();
    var c = ctx();
    if (!c) return Promise.reject(new Error('Tap the screen once first (sound needs a tap).'));
    if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) {
      return Promise.reject(new Error('This browser has no microphone access here (it needs https).'));
    }
    return navigator.mediaDevices.getUserMedia(constraints()).then(function (stream) {
      var m = { stream: stream };
      m.source = c.createMediaStreamSource(stream);
      m.gain = c.createGain();
      m.gain.gain.value = S.get('micGain');
      m.analyser = c.createAnalyser();
      m.analyser.fftSize = 1024;
      m.gate = c.createGain();
      m.gate.gain.value = 0;
      m.dest = c.createMediaStreamDestination();
      m.source.connect(m.gain);
      m.gain.connect(m.analyser);
      m.gain.connect(m.gate);
      m.gate.connect(m.dest);
      m.track = m.dest.stream.getAudioTracks()[0];
      m.buf = new Float32Array(m.analyser.fftSize);
      mic = m;
      if (c.state !== 'running') c.resume();
      for (var id in peers) attachTrack(peers[id]);
      info('Voice: microphone on.');
    });
  }
  function micOff() {
    if (mic) {
      mic.stream.getTracks().forEach(function (t) { t.stop(); });
      try { mic.source.disconnect(); mic.gate.disconnect(); } catch (e) {}
      mic = null;
      for (var id in peers) attachTrack(peers[id]);
      info('Voice: microphone off.');
    }
    setMonitor(false);
    return Promise.resolve();
  }
  // Runs ~50 times a second: measures the mic and opens/closes the gate.
  setInterval(function () {
    if (!mic) { gateIsOpen = false; lastDb = -100; return; }
    mic.gain.gain.value = S.get('micGain');
    mic.analyser.getFloatTimeDomainData(mic.buf);
    var sum = 0;
    for (var i = 0; i < mic.buf.length; i++) sum += mic.buf[i] * mic.buf[i];
    var rms = Math.sqrt(sum / mic.buf.length);
    lastDb = rms > 0 ? 20 * Math.log10(rms) : -100;
    var loud = lastDb > S.get('micGate');
    // Hold the gate open for 15 checks (0.3 s) after the last loud one, so
    // words don't get cut; counted in checks, so slowed-down timers (a
    // background tab) don't make it flicker.
    if (S.get('pushToTalk')) quietTicks = talking ? 0 : 99;
    else quietTicks = loud ? 0 : quietTicks + 1;
    var open = quietTicks < 15;
    if (open !== gateIsOpen) {
      gateIsOpen = open;
      var c = ctx();
      if (c) mic.gate.gain.setTargetAtTime(open ? 1 : 0, c.currentTime, open ? 0.005 : 0.03);
    }
  }, 20);

  function setMonitor(on) {
    var c = ctx();
    if (monitorNode) { try { monitorNode.disconnect(); } catch (e) {} monitorNode = null; }
    if (on && mic && c) {
      monitorNode = c.createGain();
      monitorNode.gain.value = 0.9;
      mic.gate.connect(monitorNode);
      monitorNode.connect(c.destination);
    }
  }

  // Records a few seconds of what others would hear (after the gate) and
  // plays it back.
  function testMic(seconds, onStage) {
    return micStart().then(function () {
      if (!window.MediaRecorder) throw new Error('This browser cannot record. Use "Hear myself live" instead.');
      return new Promise(function (resolve, reject) {
        var chunks = [], rec;
        try { rec = new MediaRecorder(mic.dest.stream); } catch (e) { reject(e); return; }
        rec.ondataavailable = function (e) { if (e.data && e.data.size) chunks.push(e.data); };
        rec.onstop = function () {
          onStage('Playing it back...');
          var blob = new Blob(chunks, { type: rec.mimeType || 'audio/mp4' });
          var a = new Audio(URL.createObjectURL(blob));
          a.onended = function () { resolve(); };
          a.onerror = function () { reject(new Error('Could not play the recording.')); };
          a.play().catch(reject);
        };
        rec.start();
        var left = seconds;
        onStage('Recording... ' + left);
        var t = setInterval(function () {
          left--;
          if (left > 0) { onStage('Recording... ' + left); return; }
          clearInterval(t);
          rec.stop();
        }, 1000);
      });
    });
  }

  // ---- Peers ----
  // Asks Opus for the best quality in the SDP (and stereo if wanted).
  function tuneSdp(sdp) {
    var m = /a=rtpmap:(\d+) opus\/48000/i.exec(sdp);
    if (!m) return sdp;
    var pt = m[1];
    var params = 'useinbandfec=1;usedtx=0;maxplaybackrate=48000';
    if (S.get('hq')) params += ';maxaveragebitrate=510000;cbr=0';
    if (S.get('stereo')) params += ';stereo=1;sprop-stereo=1';
    var re = new RegExp('a=fmtp:' + pt + ' ([^\\r\\n]*)');
    if (re.test(sdp)) {
      return sdp.replace(re, function (all, existing) {
        var keep = existing.split(';').filter(function (p) {
          return !/^(useinbandfec|usedtx|maxplaybackrate|maxaveragebitrate|cbr|stereo|sprop-stereo)=/.test(p.trim());
        });
        return 'a=fmtp:' + pt + ' ' + keep.concat([params]).join(';');
      });
    }
    return sdp.replace(new RegExp('(a=rtpmap:' + pt + ' opus/48000[^\\r\\n]*)'), '$1\r\na=fmtp:' + pt + ' ' + params);
  }

  function attachTrack(p) {
    var tr = p.transceiver || (p.pc.getTransceivers && p.pc.getTransceivers()[0]);
    if (!tr) return;
    p.transceiver = tr;
    try { tr.direction = 'sendrecv'; } catch (e) {}
    tr.sender.replaceTrack(mic ? mic.track : null).catch(function () {});
    applyBitrate(p);
  }
  function applyBitrate(p) {
    var tr = p.transceiver;
    if (!tr || !tr.sender.getParameters) return;
    var params = tr.sender.getParameters();
    if (!params.encodings || !params.encodings.length) params.encodings = [{}];
    params.encodings[0].maxBitrate = S.get('hq') ? 510000 : 64000;
    tr.sender.setParameters(params).catch(function () {});
  }

  // Waits until ICE candidates are gathered (or 2.5 s), so one message
  // carries everything the other side needs.
  function gathered(pc) {
    return new Promise(function (resolve) {
      if (pc.iceGatheringState === 'complete') { resolve(); return; }
      var done = function () { if (pc.iceGatheringState === 'complete') { clearTimeout(t); resolve(); } };
      var t = setTimeout(resolve, 2500);
      pc.addEventListener('icegatheringstatechange', done);
    });
  }

  function makePeer(id, initiator) {
    var pc = new RTCPeerConnection({ iceServers: ICE });
    var p = { id: id, pc: pc, initiator: initiator, channel: null, audio: null, panner: null, gain: null, analyser: null,
              buf: null, pos: [0, 0, 0], boost: false, created: performance.now(), state: 'connecting' };
    peers[id] = p;
    pc.ontrack = function (e) { playRemote(p, e.streams[0] || new MediaStream([e.track])); };
    pc.ondatachannel = function (e) { setChannel(p, e.channel); };
    pc.onconnectionstatechange = function () {
      p.state = pc.connectionState;
      if (pc.connectionState === 'connected') { info('Voice: connected to ' + nameOf(id) + '.'); applyBitrate(p); }
      if (pc.connectionState === 'failed') { info('Voice: could not connect to ' + nameOf(id) + ' (network).'); drop(id); }
    };
    if (initiator) {
      p.transceiver = pc.addTransceiver('audio', { direction: 'sendrecv' });
      attachTrack(p);
      setChannel(p, pc.createDataChannel('bar', { ordered: true }));
      pc.createOffer().then(function (o) { return pc.setLocalDescription({ type: 'offer', sdp: tuneSdp(o.sdp) }); })
        .then(function () { return gathered(pc); })
        .then(function () { net().sendSignal(id, { t: 'offer', sdp: pc.localDescription.sdp }); })
        .catch(function (e) { info('Voice: offer failed: ' + e); });
    }
    return p;
  }

  function setChannel(p, ch) {
    p.channel = ch;
    ch.onmessage = function (e) { if (net()) net().receiveText(p.id, e.data); };
  }

  function playRemote(p, stream) {
    var c = ctx();
    if (!p.audio) {
      p.audio = new Audio();
      p.audio.autoplay = true;
      p.audio.setAttribute('playsinline', '');
    }
    p.audio.srcObject = stream;
    var use3d = c && S.get('voice3d');
    p.audio.muted = !!use3d;  // the 3D path plays it; the element keeps the stream flowing
    p.audio.volume = Math.min(1, S.get('voiceVol'));
    p.audio.play().catch(function () {});
    if (c) {
      try {
        var src = c.createMediaStreamSource(stream);
        p.analyser = c.createAnalyser();
        p.analyser.fftSize = 512;
        p.buf = new Float32Array(p.analyser.fftSize);
        src.connect(p.analyser);
        if (use3d) {
          p.gain = c.createGain();
          p.panner = c.createPanner();
          p.panner.panningModel = 'HRTF';
          p.panner.distanceModel = 'inverse';
          p.panner.refDistance = 1.5;
          p.panner.rolloffFactor = 1.1;
          src.connect(p.gain);
          p.gain.connect(p.panner);
          p.panner.connect(window.bar3dAudio.bus('voice'));
          place3d(p);
        }
      } catch (e) {
        p.audio.muted = false;  // fall back to plain playback
      }
    }
  }

  function place3d(p) {
    if (!p.panner) return;
    var pn = p.panner;
    if (pn.positionX) { pn.positionX.value = p.pos[0]; pn.positionY.value = p.pos[1]; pn.positionZ.value = p.pos[2]; }
    else pn.setPosition(p.pos[0], p.pos[1], p.pos[2]);
    // On the stage mic: heard across the whole bar.
    pn.refDistance = p.boost ? 25 : 1.5;
  }

  function drop(id) {
    var p = peers[id];
    if (!p) return;
    try { p.pc.close(); } catch (e) {}
    if (p.audio) { p.audio.srcObject = null; }
    delete peers[id];
  }

  function nameOf(id) {
    var n = net();
    if (!n) return id;
    var idx = n.indexOf(id);
    return idx ? (n.name(idx) || id) : id;
  }

  function onSignal(from, data) {
    if (data.t === 'offer') {
      if (peers[from]) drop(from);  // they restarted: start over
      var p = makePeer(from, false);
      var pc = p.pc;
      pc.setRemoteDescription({ type: 'offer', sdp: data.sdp })
        .then(function () { attachTrack(p); return pc.createAnswer(); })
        .then(function (a) { return pc.setLocalDescription({ type: 'answer', sdp: tuneSdp(a.sdp) }); })
        .then(function () { return gathered(pc); })
        .then(function () { net().sendSignal(from, { t: 'answer', sdp: pc.localDescription.sdp }); })
        .catch(function (e) { info('Voice: answer failed: ' + e); });
    } else if (data.t === 'answer') {
      var q = peers[from];
      if (q && q.pc.signalingState === 'have-local-offer') {
        q.pc.setRemoteDescription({ type: 'answer', sdp: data.sdp }).catch(function (e) { info('Voice: ' + e); });
      }
    }
  }

  // Keeps a connection to everyone in the room: the player with the lower
  // id calls; a call that hasn't connected in 20 s is tried again.
  // Voice starts once you've entered the bar (C++ calls entered()), and
  // connects to at most one new player per check, so joining a busy room
  // doesn't start every connection at once.
  var hasEntered = false;
  function maintain() {
    var n = net();
    if (!hasEntered || !n || !window.RTCPeerConnection || !n.myId) return;
    var made = false;
    if (!started) {
      started = true;
      n.onSignal(onSignal);
    }
    var me = n.myId(), present = {};
    n.peers().forEach(function (id) {
      present[id] = true;
      var p = peers[id];
      if (p && p.state !== 'connected' && performance.now() - p.created > 20000 && p.initiator) { drop(id); p = null; }
      if (!p && me < id && !made) { makePeer(id, true); made = true; }
    });
    for (var id in peers) if (!present[id]) drop(id);
  }
  setInterval(maintain, 400);

  S.onChange(function (k) {
    if (k === 'voiceVol' || k === '*') {
      for (var id in peers) if (peers[id].audio) peers[id].audio.volume = Math.min(1, S.get('voiceVol'));
    }
  });

  function level(analyser, buf) {
    if (!analyser) return 0;
    analyser.getFloatTimeDomainData(buf);
    var sum = 0;
    for (var i = 0; i < buf.length; i++) sum += buf[i] * buf[i];
    var db = 20 * Math.log10(Math.sqrt(sum / buf.length) + 1e-9);
    return Math.max(0, Math.min(1, (db + 60) / 45));
  }

  return {
    entered: function () { hasEntered = true; maintain(); },
    micStart: micStart,
    micOff: micOff,
    micOn: function () { return !!mic; },
    restartMic: function () { if (mic) { micOff(); micStart().catch(function () {}); } },
    applySettings: function () {
      for (var id in peers) {
        applyBitrate(peers[id]);
        if (peers[id].audio && peers[id].audio.srcObject) playRemote(peers[id], peers[id].audio.srcObject);
      }
      info('Voice: quality settings apply to new connections fully; rejoin the room to renegotiate.');
    },
    setTalking: function (on) { talking = on; },
    gateOpen: function () { return gateIsOpen; },
    inputDb: function () { return lastDb; },
    testMic: testMic,
    setMonitor: setMonitor,
    monitoring: function () { return !!monitorNode; },
    statusText: function () {
      var ids = Object.keys(peers), on = ids.filter(function (id) { return peers[id].state === 'connected'; });
      if (!net() || !net().peers().length) return 'Nobody else is here yet.';
      return 'Connected to ' + on.length + ' of ' + net().peers().length + ' player(s)' + (mic ? '' : ' (your mic is off: you can hear them)');
    },
    // From C++: where a player's head is (by player index), and if they're on the stage mic.
    place: function (index, x, y, z, boost) {
      var n = net();
      if (!n) return;
      var p = peers[n.idOf(index)];
      if (!p) return;
      p.pos = [x, y, z];
      p.boost = !!boost;
      place3d(p);
    },
    // Every connected player's level: [[index, level 0..1], ...] (read once
    // per frame by web/bridge.js).
    levels: function () {
      var n = net(), out = [];
      if (!n) return out;
      for (var id in peers) {
        var p = peers[id];
        if (!p.analyser) continue;
        var idx = n.indexOf(id);
        if (idx >= 0) out.push([idx, level(p.analyser, p.buf)]);
      }
      return out;
    },
    peerLevel: function (index) {
      var n = net();
      var p = n && peers[n.idOf(index)];
      return p ? level(p.analyser, p.buf) : 0;
    },
    myLevel: function () { return mic && gateIsOpen ? Math.max(0, Math.min(1, (lastDb + 60) / 45)) : 0; },
    _pcs: function () { return Object.keys(peers).map(function (id) { return peers[id].pc; }); },
    // Quick events over the data channel; false if there's no open channel.
    sendData: function (id, text) {
      var p = peers[id];
      if (!p || !p.channel || p.channel.readyState !== 'open') return false;
      try { p.channel.send(text); return true; } catch (e) { return false; }
    },
  };
})();
