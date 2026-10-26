// The crosshair (the user, 2026-10-26 #12): Settings → Pointer. On, it's drawn
// at the centre of the screen and a tap anywhere uses what it points at
// (bar/game.cpp reads the "crosshair" setting). Its look comes from the
// ch* settings (bar/web/menu.js SCHEMA); svg() also draws the editor's preview.
window.bar3dCrosshair = (function () {
  'use strict';
  var S = window.bar3dSettings;
  var COLORS = [
    { name: 'White', hex: '#ffffff' }, { name: 'Warm', hex: '#ffd9a0' }, { name: 'Green', hex: '#5cff7a' },
    { name: 'Cyan', hex: '#4ee6ff' }, { name: 'Yellow', hex: '#ffe14a' }, { name: 'Pink', hex: '#ff5cc8' },
    { name: 'Red', hex: '#ff4a4a' }, { name: 'Black', hex: '#111111' },
  ];
  var el = null;

  function svg(preview) {
    var shape = S.get('chShape'), size = S.get('chSize'), t = S.get('chThick'), gap = S.get('chGap');
    var col = COLORS[S.get('chColor')].hex, op = S.get('chOpacity'), outline = S.get('chOutline');
    var half = size + gap + t + 2, w = half * 2, c = half;
    var parts = [];
    var line = function (x1, y1, x2, y2) { parts.push('<line x1="' + x1 + '" y1="' + y1 + '" x2="' + x2 + '" y2="' + y2 + '"/>'); };
    if (shape === 1 || shape === 2) {
      line(c, c - gap - size, c, c - gap); line(c, c + gap, c, c + gap + size);
      line(c - gap - size, c, c - gap, c); line(c + gap, c, c + gap + size, c);
    }
    if (shape === 3 || shape === 4) parts.push('<circle cx="' + c + '" cy="' + c + '" r="' + (gap + size / 2) + '" fill="none"/>');
    var dot = shape === 0 || shape === 2 || shape === 4
      ? '<circle cx="' + c + '" cy="' + c + '" r="' + Math.max(1, t * (shape === 0 ? 1.2 : 0.8)) + '"/>' : '';
    var group = function (stroke, extra) {
      return '<g stroke="' + stroke + '" fill="' + stroke + '" stroke-width="' + (t + extra) + '" stroke-linecap="round">' +
        parts.join('') + dot.replace('r="', 'stroke-width="' + extra + '" r="') + '</g>';
    };
    var scale = preview ? 2 : 1;
    return '<svg width="' + w * scale + '" height="' + w * scale + '" viewBox="0 0 ' + w + ' ' + w + '" style="opacity:' + op +
      '">' + (outline ? group('rgba(0,0,0,.65)', 2) : '') + group(col, 0) + '</svg>';
  }

  function update() {
    var on = S.get('crosshair');
    if (!on) { if (el) el.style.display = 'none'; return; }
    if (!el) {
      el = document.createElement('div');
      el.id = 'crosshair';
      el.style.cssText = 'position:fixed;left:50%;top:50%;transform:translate(-50%,-50%);pointer-events:none;z-index:5;';
      document.body.appendChild(el);
    }
    el.style.display = 'block';
    el.innerHTML = svg(false);
  }
  S.onChange(function (k) { if (k === '*' || /^(crosshair|ch)/.test(k)) update(); });
  update();
  return { svg: svg, COLORS: COLORS, update: update };
})();
