// The lounge chessboard, full screen (J101): the board flat over the game,
// like the chess sites, in themes. Each theme is a calm pair of square
// colours far enough apart in lightness that the squares always read, pieces
// drawn to match (their fill, edge and finish) and a smooth background.
// The game keeps the rules and the room's game (bar/projects/chess): it sends
// the position here ("chess" page operation, bridge.js) and takes your moves
// back (commands chessMove, chessNew, chessFull, chessLeave). The theme is
// kept on this phone.
window.bar3dChess = (function () {
  'use strict';
  var root = null, st = null, sel = -1, shownLast = '', boardEl = null;

  // Square colours (light, dark), pieces (white: fill, edge; black: fill,
  // edge; the shine across them), the background, and the marks.
  var THEMES = [
    { id: 'walnut', name: 'Walnut', light: '#ead9b8', dark: '#a87d56', bg: 'radial-gradient(120% 90% at 50% 20%, #4a3324 0%, #241911 70%)',
      w: ['#fbf4e4', '#4a3424'], b: ['#2e2219', '#f0e2c8'], shine: 0.35, style: 'classic', ink: '#f1e4cb', mark: 'rgba(246,206,98,.45)' },
    { id: 'sage', name: 'Sage', light: '#e7ecdb', dark: '#8ea889', bg: 'linear-gradient(160deg, #2f4136 0%, #1a2620 100%)',
      w: ['#fdfbf3', '#33473a'], b: ['#22362b', '#e8efdc'], shine: 0.3, style: 'classic', ink: '#e7efe0', mark: 'rgba(250,236,140,.45)' },
    { id: 'dusk', name: 'Dusk', light: '#d0d3e6', dark: '#727aa0', bg: 'linear-gradient(170deg, #34375a 0%, #1b1c31 100%)',
      w: ['#f7f6ff', '#2c2f52'], b: ['#20233f', '#dcdff5'], shine: 0.4, style: 'round', ink: '#e4e6fa', mark: 'rgba(255,214,140,.42)' },
    { id: 'coral', name: 'Coral', light: '#f4e4da', dark: '#c58d7c', bg: 'radial-gradient(110% 80% at 50% 10%, #5a3530 0%, #2a1715 75%)',
      w: ['#fffaf6', '#5a2e2a'], b: ['#47201f', '#f8e3d8'], shine: 0.35, style: 'classic', ink: '#f6e5dc', mark: 'rgba(255,224,150,.45)' },
    { id: 'slate', name: 'Slate', light: '#dde3e6', dark: '#7f919b', bg: 'linear-gradient(165deg, #3a464d 0%, #1c2327 100%)',
      w: ['#ffffff', '#2c3439'], b: ['#262c30', '#e6ecef'], shine: 0.45, style: 'round', ink: '#e8eef0', mark: 'rgba(160,220,255,.4)' },
    { id: 'ink', name: 'Night', light: '#6a7187', dark: '#3d4357', bg: 'radial-gradient(120% 90% at 50% 0%, #2a2f40 0%, #0f1117 80%)',
      w: ['#f2efe6', '#181a22'], b: ['#141720', '#9aa3bd'], shine: 0.5, style: 'round', ink: '#cfd5e6', mark: 'rgba(255,200,120,.4)' },
  ];
  var theme = THEMES[0];
  try { var saved = localStorage.getItem('bar3dChess.theme'); THEMES.forEach(function (t) { if (t.id === saved) theme = t; }); } catch (e) {}

  // Piece outlines, 100 x 100, standing on y = 90. Two sets: classic
  // (Staunton-like) and round (soft, modern).
  var BASE = 'M22 90 h56 v-6 q0 -5 -6 -6 h-44 q-6 1 -6 6 z ';
  var SHAPES = {
    classic: {
      1: BASE + 'M36 78 q2 -14 8 -22 q-9 -4 -9 -13 a15 15 0 1 1 30 0 q0 9 -9 13 q6 8 8 22 z',
      2: BASE + 'M32 78 q0 -14 10 -24 q-6 -3 -12 -2 q-6 1 -9 -5 q-2 -6 5 -11 l16 -14 q2 -8 9 -10 l2 6 q14 2 20 18 q6 16 3 42 z M50 30 a2.5 2.5 0 1 0 0.1 0 z',
      3: BASE + 'M35 78 q3 -14 9 -22 q-10 -6 -10 -18 q0 -14 16 -24 q16 10 16 24 q0 12 -10 18 q6 8 9 22 z M50 9 a4 4 0 1 0 0.1 0 z M47 30 l9 -9 l3 3 l-9 9 z',
      4: BASE + 'M31 78 l4 -34 h-6 v-18 h9 v7 h6 v-7 h12 v7 h6 v-7 h9 v18 h-6 l4 34 z',
      5: BASE + 'M32 78 q4 -18 4 -30 l-12 -20 l12 8 l4 -18 l6 16 l4 -20 l4 20 l6 -16 l4 18 l12 -8 l-12 20 q0 12 4 30 z M24 22 a4 4 0 1 0 0.1 0 z M76 22 a4 4 0 1 0 0.1 0 z M50 4 a4 4 0 1 0 0.1 0 z M36 13 a3.5 3.5 0 1 0 0.1 0 z M64 13 a3.5 3.5 0 1 0 0.1 0 z',
      6: BASE + 'M32 78 q4 -18 3 -28 q-10 -2 -10 -12 q0 -10 16 -10 h4 v-8 h-6 v-6 h6 v-6 h8 v6 h6 v6 h-6 v8 h4 q16 0 16 10 q0 10 -10 12 q-1 10 3 28 z',
    },
    round: {
      1: BASE + 'M37 78 q0 -16 7 -22 a16 16 0 1 1 12 0 q7 6 7 22 z',
      2: BASE + 'M33 78 q-2 -22 8 -34 q-8 2 -14 -3 q-3 -5 3 -10 q12 -10 22 -16 q16 -4 22 16 q5 18 1 47 z M54 27 a3 3 0 1 0 0.1 0 z',
      3: BASE + 'M35 78 q2 -16 8 -22 q-12 -8 -10 -22 q2 -14 17 -22 q15 8 17 22 q2 14 -10 22 q6 6 8 22 z M50 6 a5 5 0 1 0 0.1 0 z',
      4: BASE + 'M32 78 q3 -16 3 -34 q-7 -2 -7 -10 v-12 q0 -3 3 -3 h38 q3 0 3 3 v12 q0 8 -7 10 q0 18 3 34 z',
      5: BASE + 'M32 78 q4 -16 5 -32 q-10 -4 -12 -16 a6 6 0 0 1 10 -4 q4 6 7 4 q2 -10 8 -14 q6 4 8 14 q3 2 7 -4 a6 6 0 0 1 10 4 q-2 12 -12 16 q1 16 5 32 z M50 8 a5 5 0 1 0 0.1 0 z',
      6: BASE + 'M32 78 q4 -16 4 -30 q-10 -4 -10 -14 q0 -10 16 -10 h4 v-7 h-5 a3 3 0 0 1 0 -6 h5 v-5 a4 4 0 0 1 8 0 v5 h5 a3 3 0 0 1 0 6 h-5 v7 h4 q16 0 16 10 q0 10 -10 14 q0 14 4 30 z',
    },
  };

  function pieceSvg(code) {
    var black = code & 8, kind = code & 7, colors = black ? theme.b : theme.w;
    var id = 'g' + theme.id + (black ? 'b' : 'w');
    return '<svg viewBox="0 0 100 100" width="100%" height="100%"><defs><linearGradient id="' + id + '" x1="0" y1="0" x2="1" y2="1">' +
      '<stop offset="0" stop-color="#fff" stop-opacity="' + theme.shine + '"/><stop offset=".55" stop-color="#fff" stop-opacity="0"/>' +
      '</linearGradient></defs>' +
      '<path d="' + SHAPES[theme.style][kind] + '" fill="' + colors[0] + '" stroke="' + colors[1] + '" stroke-width="3" stroke-linejoin="round" fill-rule="evenodd"/>' +
      '<path d="' + SHAPES[theme.style][kind] + '" fill="url(#' + id + ')" fill-rule="evenodd"/></svg>';
  }

  function el(tag, cls, text) { var e = document.createElement(tag); if (cls) e.className = cls; if (text) e.textContent = text; return e; }
  function cmd(name, a) { if (window.bar3dBridge) bar3dBridge.command(name, a || 0, 0, '', ''); }

  function style() {
    if (document.getElementById('chessStyle')) return;
    var s = document.createElement('style');
    s.id = 'chessStyle';
    s.textContent = [
      '#chessFull{position:fixed;inset:0;z-index:55;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:12px;',
      '  font:600 14px system-ui,sans-serif;padding:env(safe-area-inset-top) 12px env(safe-area-inset-bottom);user-select:none;-webkit-user-select:none;}',
      '#chessFull .cfStatus{font-size:19px;letter-spacing:.02em;text-align:center;min-height:24px;}',
      '#chessFull .cfSub{opacity:.75;font-weight:500;font-size:13px;text-align:center;min-height:17px;}',
      '#chessFull .cfBoard{position:relative;border-radius:10px;overflow:hidden;box-shadow:0 18px 50px rgba(0,0,0,.45),0 0 0 1px rgba(255,255,255,.06);}',
      '#chessFull .cfSq{position:absolute;}',
      '#chessFull .cfSq i{position:absolute;font-style:normal;font-size:10px;font-weight:700;opacity:.7;}',
      '#chessFull .cfPiece{position:absolute;transition:transform .22s ease;pointer-events:none;filter:drop-shadow(0 2px 2px rgba(0,0,0,.35));}',
      '#chessFull .cfDot{position:absolute;border-radius:50%;pointer-events:none;}',
      '#chessFull .cfKeys{display:flex;gap:8px;flex-wrap:wrap;justify-content:center;}',
      '#chessFull button{font:600 14px system-ui,sans-serif;border:0;border-radius:10px;padding:11px 14px;background:rgba(255,255,255,.12);color:inherit;}',
      '#chessFull .cfThemes{display:flex;gap:8px;justify-content:center;flex-wrap:wrap;}',
      '#chessFull .cfThemes button{padding:6px 8px;display:flex;align-items:center;gap:6px;font-size:12px;}',
      '#chessFull .cfThemes button.on{outline:2px solid currentColor;}',
      '#chessFull .cfSw{width:16px;height:16px;border-radius:4px;display:inline-block;}',
    ].join('\n');
    document.head.appendChild(s);
  }

  function open() {
    style();
    if (!root) {
      root = el('div');
      root.id = 'chessFull';
      document.body.appendChild(root);
    }
    if (window.bar3dModal) bar3dModal.open('chess', function () { cmd('chessFull', 0); });
  }
  function close() {
    if (root) { root.remove(); root = null; }
    boardEl = null;
    sel = -1;
    if (window.bar3dModal) bar3dModal.closed('chess');
  }

  function render() {
    if (!st) return;
    open();
    root.style.background = theme.bg;
    root.style.color = theme.ink;
    root.innerHTML = '';
    root.appendChild(el('div', 'cfStatus', st.status));
    root.appendChild(el('div', 'cfSub', st.msg || ((st.me ? 'You are black' : 'You are white') + ' · move ' + st.move)));
    var size = Math.floor(Math.min(window.innerWidth - 24, window.innerHeight - 260) / 8) * 8;
    var q = size / 8;
    boardEl = el('div', 'cfBoard');
    boardEl.style.width = boardEl.style.height = size + 'px';
    var flip = st.me === 1;
    var at = function (sq) { var f = sq % 8, r = sq >> 3; return flip ? [(7 - f) * q, r * q] : [f * q, (7 - r) * q]; };
    var targets = {};
    st.moves.forEach(function (m) { if (m[0] === sel) targets[m[1]] = true; });
    for (var sq = 0; sq < 64; sq++) {
      var p = at(sq), lightSq = ((sq % 8) + (sq >> 3)) % 2 === 1;
      var d = el('div', 'cfSq');
      d.style.cssText = 'left:' + p[0] + 'px;top:' + p[1] + 'px;width:' + q + 'px;height:' + q + 'px;background:' + (lightSq ? theme.light : theme.dark);
      if (sq === st.last[0] || sq === st.last[1]) d.style.boxShadow = 'inset 0 0 0 ' + q + 'px ' + theme.mark;
      if (sq === sel) d.style.boxShadow = 'inset 0 0 0 ' + q + 'px rgba(255,255,255,.28)';
      if (sq === st.check) d.style.background = 'radial-gradient(circle, rgba(230,60,50,.85) 0%, rgba(230,60,50,.35) 55%, ' + (lightSq ? theme.light : theme.dark) + ' 75%)';
      // The files and ranks on the edge squares, in the other square colour.
      var edgeRank = flip ? 7 : 0, edgeFile = flip ? 7 : 0;
      if ((sq >> 3) === edgeRank) { var fl = el('i', '', 'abcdefgh'[sq % 8]); fl.style.cssText = 'right:3px;bottom:1px;color:' + (lightSq ? theme.dark : theme.light); d.appendChild(fl); }
      if ((sq % 8) === edgeFile) { var rk = el('i', '', String((sq >> 3) + 1)); rk.style.cssText = 'left:3px;top:1px;color:' + (lightSq ? theme.dark : theme.light); d.appendChild(rk); }
      d.dataset.sq = sq;
      boardEl.appendChild(d);
      if (targets[sq]) {
        var dot = el('div', 'cfDot'), cap = st.b[sq] !== 0;
        var r = cap ? q * 0.46 : q * 0.15;
        dot.style.cssText = 'left:' + (p[0] + q / 2 - r) + 'px;top:' + (p[1] + q / 2 - r) + 'px;width:' + 2 * r + 'px;height:' + 2 * r + 'px;' +
          (cap ? 'border:' + Math.max(3, q * 0.08) + 'px solid rgba(0,0,0,.28);box-sizing:border-box;' : 'background:rgba(0,0,0,.25);');
        boardEl.appendChild(dot);
      }
    }
    // The pieces; the one that just moved slides in from where it was.
    var lastKey = st.last.join(',') + '/' + st.move + '/' + st.turn;
    for (sq = 0; sq < 64; sq++) {
      if (!st.b[sq]) continue;
      var pc = el('div', 'cfPiece');
      pc.innerHTML = pieceSvg(st.b[sq]);
      var to = at(sq);
      pc.style.cssText = 'left:0;top:0;width:' + q + 'px;height:' + q + 'px;transform:translate(' + to[0] + 'px,' + to[1] + 'px)';
      if (sq === st.last[1] && lastKey !== shownLast && st.last[0] >= 0) {
        var from = at(st.last[0]);
        pc.style.transition = 'none';
        pc.style.transform = 'translate(' + from[0] + 'px,' + from[1] + 'px)';
        (function (e, t) { requestAnimationFrame(function () { requestAnimationFrame(function () {
          e.style.transition = ''; e.style.transform = 'translate(' + t[0] + 'px,' + t[1] + 'px)'; }); }); })(pc, to);
      }
      boardEl.appendChild(pc);
    }
    shownLast = lastKey;
    boardEl.onclick = function (e) {
      var r = boardEl.getBoundingClientRect();
      var f = Math.floor((e.clientX - r.left) / q), rr = Math.floor((e.clientY - r.top) / q);
      var sqr = flip ? (rr * 8 + (7 - f)) : ((7 - rr) * 8 + f);
      if (sqr < 0 || sqr > 63) return;
      if (sel >= 0 && st.moves.some(function (m) { return m[0] === sel && m[1] === sqr; })) {
        cmd('chessMove', sel * 64 + sqr);
        sel = -1;
        return;
      }
      sel = st.moves.some(function (m) { return m[0] === sqr; }) ? sqr : -1;
      render();
    };
    root.appendChild(boardEl);
    var keys = el('div', 'cfKeys');
    var key = function (label, fn) { var b = el('button', '', label); b.onclick = fn; keys.appendChild(b); };
    key('Leave', function () { cmd('chessLeave'); });
    if (st.canNew) key('New game', function () { cmd('chessNew'); });
    key('3D view', function () { cmd('chessFull', 0); });
    root.appendChild(keys);
    var themes = el('div', 'cfThemes');
    THEMES.forEach(function (t) {
      var b = el('button', t === theme ? 'on' : '');
      var sw = el('span', 'cfSw');
      sw.style.background = 'linear-gradient(135deg,' + t.light + ' 50%,' + t.dark + ' 50%)';
      b.appendChild(sw);
      b.appendChild(document.createTextNode(t.name));
      b.onclick = function () {
        theme = t;
        try { localStorage.setItem('bar3dChess.theme', t.id); } catch (e) {}
        render();
      };
      themes.appendChild(b);
    });
    root.appendChild(themes);
  }

  window.addEventListener('resize', function () { if (root) render(); });

  return {
    // From the game: the position, or null (the board closed or back in 3D).
    state: function (s) {
      if (!s) { st = null; close(); return; }
      if (st && (st.turn !== s.turn || st.move !== s.move)) sel = -1;
      st = s;
      render();
    },
    themes: THEMES,
  };
})();
