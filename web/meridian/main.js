import * as THREE from 'three';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/addons/postprocessing/UnrealBloomPass.js';
import { OutputPass } from 'three/addons/postprocessing/OutputPass.js';
import { ShaderPass } from 'three/addons/postprocessing/ShaderPass.js';
import { Road, DS, LANE, ROAD_HALF, SHOULDER, BIOMES, biomeAt, clamp, lerp, smoothstep } from './road.js';
import { Car } from './car.js';
import { AudioEngine } from './audio.js';
import { World, buildCarMesh, loadModels, Trail, GradeShader } from './world.js';
const models = await loadModels();

const G = 9.81;
const $ = (id) => document.getElementById(id);

// ---------- renderer ----------
const canvas = $('c');
const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, powerPreference: 'high-performance' });
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));
renderer.setSize(innerWidth, innerHeight);
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.0;
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.info.autoReset = false;

const road = new Road(20261016);
const world = new World(renderer, road, models);
const scene = world.scene;
const camera = new THREE.PerspectiveCamera(60, innerWidth / innerHeight, 0.1, 4000);

const composer = new EffectComposer(renderer);
composer.addPass(new RenderPass(scene, camera));
const bloom = new UnrealBloomPass(new THREE.Vector2(innerWidth, innerHeight), 0.5, 0.55, 1.35);
composer.addPass(bloom);
composer.addPass(new OutputPass());
const grade = new ShaderPass(GradeShader);
composer.addPass(grade);

const carMesh = buildCarMesh(models.car, 'pearl');
scene.add(carMesh.root);
const trails = [new Trail(scene, 0xff3040, 34), new Trail(scene, 0xff3040, 34)];

const car = new Car();
const audio = new AudioEngine();

// ---------- state ----------
const settings = { camera: 'chase', units: 'mph', assist: 'rails', hud: 'on', paint: 'pearl', guide: 'on' };
const state = {
  mode: 'title', // title | drive | paused
  time: 0, flow: 0, score: 0, dist: 0, streak: 0, bestStreak: 0,
  idx: 0, d: 0, onRoad: true, offroad: false, shoulder: false,
  steerRateF: 0, lastSteer: 0, biomeIndex: -1,
  corner: null, offTimer: 0, stuckTimer: 0, resetting: false, manual: false,
  tier: 0, auto: false,
  intent: 0, lineD: -LANE, governing: false, dRateF: 0, weaveT: 0, wallHit: 0,
};
const input = { steer: 0, throttle: 0, brake: 0, hand: false };
const keys = new Set();

function placeCar(sampleIndex, speed = 0, lane = -LANE) {
  const p = road.get(sampleIndex);
  car.reset(p.x + p.cz * lane, p.z - p.sx * lane, p.h, speed);
  state.idx = sampleIndex;
  trails.forEach((t) => t.clear());
  input.steer = 0; state.lastSteer = 0;
  state.lineD = lane; state.intent = 0; state.railI = 0; state.latI = 0;
  pose.init = false;
}

// ---------- input ----------
addEventListener('keydown', (e) => {
  if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' '].includes(e.key)) e.preventDefault();
  const k = e.key.toLowerCase();
  if (e.repeat) { keys.add(k); return; }
  keys.add(k);
  if (k === 'escape' || k === 'p') togglePause();
  if (k === 'c') cycleCamera();
  if (k === 'h') cycleHud();
  if (k === 'm') { const m = audio.toggleMute(); toast(m ? 'Music muted' : 'Music on'); }
  if (k === 'r' && state.mode === 'drive') softReset();
  if (k === '`' || k === 'f3') $('debug').classList.toggle('hidden');
  if (k === 'enter' && state.mode === 'title') startDrive();
});
addEventListener('keyup', (e) => keys.delete(e.key.toLowerCase()));
addEventListener('blur', () => keys.clear());

// touch
const isTouch = matchMedia('(pointer: coarse)').matches;
const touch = { steer: 0, gas: false, brake: false, id: null, x0: 0 };
if (isTouch) {
  const zone = $('tSteer'), knob = $('tKnob');
  zone.addEventListener('pointerdown', (e) => { touch.id = e.pointerId; touch.x0 = e.clientX; zone.setPointerCapture(e.pointerId); });
  zone.addEventListener('pointermove', (e) => { if (e.pointerId !== touch.id) return; const dx = e.clientX - touch.x0; touch.steer = clamp(-dx / 90, -1, 1); knob.style.transform = `translateX(calc(-50% + ${clamp(dx, -90, 90)}px))`; });
  const end = (e) => { if (e.pointerId !== touch.id) return; touch.id = null; touch.steer = 0; knob.style.transform = 'translateX(-50%)'; };
  zone.addEventListener('pointerup', end); zone.addEventListener('pointercancel', end);
  const hold = (el, key) => {
    el.addEventListener('pointerdown', (e) => { e.preventDefault(); touch[key] = true; el.classList.add('held'); el.setPointerCapture(e.pointerId); });
    const up = () => { touch[key] = false; el.classList.remove('held'); };
    el.addEventListener('pointerup', up); el.addEventListener('pointercancel', up);
  };
  hold($('tGas'), 'gas'); hold($('tBrake'), 'brake');
  $('tPause').addEventListener('click', togglePause);
}

let padPrev = {};
function readInput(dt) {
  const kb = {
    left: keys.has('a') || keys.has('arrowleft'), right: keys.has('d') || keys.has('arrowright'),
    up: keys.has('w') || keys.has('arrowup'), down: keys.has('s') || keys.has('arrowdown'), hand: keys.has(' '),
  };
  let pad = null;
  const pads = navigator.getGamepads ? navigator.getGamepads() : [];
  for (const p of pads) if (p && p.connected) { pad = p; break; }

  let steerT = 0, thrT = 0, brkT = 0, hand = kb.hand, analog = false;
  if (pad) {
    const ax = pad.axes[0] || 0;
    const dz = 0.06;
    const a = Math.abs(ax) < dz ? 0 : (Math.abs(ax) - dz) / (1 - dz);
    const curved = Math.sign(ax) * Math.pow(a, 1.35);
    const rt = pad.buttons[7] ? pad.buttons[7].value : 0, lt = pad.buttons[6] ? pad.buttons[6].value : 0;
    if (Math.abs(curved) > 0.001 || rt > 0.02 || lt > 0.02) analog = true;
    steerT = -curved; thrT = rt; brkT = lt;
    if (pad.buttons[0]?.pressed || pad.buttons[1]?.pressed) hand = true;
    const pressed = (i) => pad.buttons[i]?.pressed && !padPrev[i];
    if (pressed(9)) togglePause();
    if (pressed(3)) cycleCamera();
    if (pressed(2) && state.mode === 'drive') softReset();
    if (pressed(0) && state.mode === 'title') startDrive();
    padPrev = {}; pad.buttons.forEach((b, i) => (padPrev[i] = b.pressed));
  }
  if (isTouch && (touch.id !== null || touch.gas || touch.brake)) {
    analog = true; steerT = touch.steer; thrT = touch.gas ? 1 : 0; brkT = touch.brake ? 1 : 0;
  }
  const kbSteer = (kb.left ? 1 : 0) - (kb.right ? 1 : 0);
  const kbActive = kb.left || kb.right || kb.up || kb.down;

  if (analog && !kbActive) {
    // pads: progressive, a touch of speed sensitivity unless assists are off
    const lim = settings.assist === 'off' ? 1 : clamp(2.3 * car.gripSteer(car.u) / car.maxSteer(car.u), 0.4, 1);
    input.steer = lerp(input.steer, steerT * lim, 1 - Math.exp(-dt * 22));
    input.throttle = thrT; input.brake = brkT;
  } else {
    // keyboard: eased, speed-sensitive steering so taps feel like smooth wheel movements
    const u = Math.abs(car.u);
    const lim = clamp((settings.assist === 'off' ? 2.1 : 1.75) * car.gripSteer(car.u) / car.maxSteer(car.u), 0.16, 1);
    const target = kbSteer * lim;
    const rate = kbSteer !== 0 ? (Math.sign(target) !== Math.sign(input.steer) && Math.abs(input.steer) > 0.05 ? 9 : 6.5) : 7;
    input.steer = lerp(input.steer, target, 1 - Math.exp(-dt * rate));
    input.throttle = lerp(input.throttle, kb.up ? 1 : 0, 1 - Math.exp(-dt * (kb.up ? 4 : 8)));
    input.brake = lerp(input.brake, kb.down ? 1 : 0, 1 - Math.exp(-dt * (kb.down ? 6 : 10)));
  }
  // raw steering intent (used by the Rails assist to move the guide line)
  if (analog && !kbActive) state.intent = lerp(state.intent, steerT, 1 - Math.exp(-dt * 14));
  else state.intent = lerp(state.intent, kbSteer, 1 - Math.exp(-dt * (kbSteer !== 0 ? 9 : 10)));
  input.hand = hand;
}

// ---------- Rails assist: you pick the line, the car holds it ----------
const RAIL_LIM = ROAD_HALF - 1.15;
function railsOn() { return settings.assist === 'rails' && state.mode === 'drive' && !state.auto; }
function railTarget(look, lineD) {
  // point on the road `look` metres ahead at lateral offset lineD
  const f = state.idx + look / DS, i = Math.floor(f), a = f - i;
  const p = road.get(i), q = road.get(i + 1);
  const x = lerp(p.x, q.x, a), z = lerp(p.z, q.z, a), cz = lerp(p.cz, q.cz, a), sx = lerp(p.sx, q.sx, a);
  return { x: x + cz * lineD, z: z - sx * lineD };
}
function rails(dt) {
  const u = Math.max(car.u, 0);
  // steering input slides the guide line across the road; let go and it settles into the nearest lane
  const it = state.intent;
  const lineRate = 4.5 + u * 0.16;
  const lineWas = state.lineD;
  if (Math.abs(it) > 0.04) state.lineD += it * lineRate * dt;
  else {
    const home = state.lineD > 0 ? LANE : -LANE;
    state.lineD += clamp(home - state.lineD, -1, 1) * Math.min(1, dt * 0.9);
  }
  state.lineD = clamp(state.lineD, -RAIL_LIM, RAIL_LIM);
  const lineVel = (state.lineD - lineWas) / Math.max(dt, 1e-4);
  // path follower: road-curvature feed-forward + critically damped lateral correction (no overshoot, no weave)
  const ue = Math.max(car.u, 6);
  const pc = road.get(state.idx), pv = road.get(state.idx + Math.round((u * 0.55) / DS));
  const kLine = pv.k / Math.max(0.3, 1 - pv.k * state.lineD);
  const rel = Math.atan2(Math.sin(car.yaw - pc.h), Math.cos(car.yaw - pc.h));
  const dRate = car.u * Math.sin(rel) + car.v * Math.cos(rel);
  const err = state.lineD - state.d;
  const active = Math.abs(it) > 0.05;
  const wn = active ? 2.4 : 1.6, zeta = 1.0;
  state.latI = clamp((state.latI || 0) + err * dt, -1.5, 1.5);
  // lateral-velocity feed-forward: the car moves the instant you steer, the line just tells it where to stop
  const aMax = active ? 7.5 : 3.5;
  const aLat = clamp(wn * wn * err + 0.9 * state.latI - 2 * zeta * wn * (dRate - lineVel), -aMax, aMax);
  const kCmd = kLine + aLat / (ue * ue);
  const rErr = car.u * kCmd - car.r;
  state.railI = clamp((state.railI || 0) + rErr * dt, -0.25, 0.25);
  const delta = Math.atan(kCmd * car.L) + clamp((rErr * 2.2 + state.railI * 2.0) * car.L / ue, -0.1, 0.1);
  const target = clamp(delta / car.maxSteer(car.u), -1, 1);
  input.steer = lerp(input.steer, target, 1 - Math.exp(-dt * 20));
  // corner-speed governor: never arrive faster than the tyres can carry
  const kmax = road.maxCurvatureAhead(state.idx, 24 + u * 3.2);
  const vt = Math.sqrt(0.78 * G / Math.max(kmax, 1e-4));
  state.governing = car.u > vt + 0.5;
  if (state.governing) {
    input.throttle = Math.min(input.throttle, 0.1);
    input.brake = Math.max(input.brake, clamp((car.u - vt) * 0.09, 0, 0.45));
  } else if (car.u > vt - 2) {
    input.throttle = Math.min(input.throttle, clamp((vt - car.u) * 0.35, 0, 1));
  }
  input.hand = false;
}
// soft wall: the car can never leave the asphalt in Rails mode
function railWall() {
  const l = road.locate(car.x, car.z, state.idx), p = l.p;
  const lim = ROAD_HALF - 0.95;
  if (Math.abs(l.d) <= lim) return;
  const s = Math.sign(l.d), ex = l.d - s * lim;
  car.x -= p.cz * ex; car.z += p.sx * ex;
  const rel = Math.atan2(Math.sin(car.yaw - p.h), Math.cos(car.yaw - p.h));
  const dRate = car.u * Math.sin(rel) + car.v * Math.cos(rel);
  if (dRate * s > 0) { car.yaw = p.h + rel * 0.92; car.v *= 0.8; car.r *= 0.85; }
  state.wallHit = 1;
}

// ---------- guide line ----------
const GUIDE_N = 56;
const guideGeo = new THREE.BufferGeometry();
guideGeo.setAttribute('position', new THREE.BufferAttribute(new Float32Array(GUIDE_N * 2 * 3), 3));
guideGeo.setAttribute('fade', new THREE.BufferAttribute(new Float32Array(GUIDE_N * 2), 1));
{ const si = new Float32Array(GUIDE_N * 2); for (let k = 0; k < GUIDE_N * 2; k++) si[k] = k >> 1; guideGeo.setAttribute('sidx', new THREE.BufferAttribute(si, 1)); }
{
  const idx = [];
  for (let k = 0; k < GUIDE_N - 1; k++) { const a = k * 2; idx.push(a, a + 1, a + 2, a + 1, a + 3, a + 2); }
  guideGeo.setIndex(idx);
}
const guideMat = new THREE.ShaderMaterial({
  transparent: true, depthWrite: false, toneMapped: false,
  uniforms: { col: { value: new THREE.Color(0x9fe9ff) }, op: { value: 0.0 }, time: { value: 0 } },
  vertexShader: 'attribute float fade; attribute float sidx; varying float vF; varying float vS; void main(){ vF = fade; vS = sidx; gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0); }',
  fragmentShader: 'uniform vec3 col; uniform float op; uniform float time; varying float vF; varying float vS; void main(){ float dash = 0.75 + 0.25 * sin(vS * 0.9 - time * 9.0); gl_FragColor = vec4(col * 1.6, op * vF * dash); }',
});
const guide = new THREE.Mesh(guideGeo, guideMat);
guide.frustumCulled = false; guide.renderOrder = 3;
scene.add(guide);
const _gc = new THREE.Color();
function updateGuide(dt) {
  const show = settings.guide === 'on' && state.mode === 'drive';
  guideMat.uniforms.op.value = lerp(guideMat.uniforms.op.value, show ? 0.7 : 0, 1 - Math.exp(-dt * 4));
  guide.visible = guideMat.uniforms.op.value > 0.01;
  if (!guide.visible) return;
  guideMat.uniforms.time.value += dt;
  const u = Math.max(car.u, 2);
  const step = clamp(u * 0.075, 0.7, 2.6);
  const pos = guideGeo.attributes.position.array, fade = guideGeo.attributes.fade.array;
  const pts = [];
  const rails = settings.assist === 'rails';
  // predicted path: rails → blend from car into the chosen line; otherwise integrate the current arc
  let x = car.x, z = car.z, h = car.yaw;
  const kappa = car.r / Math.max(car.u, 2);
  const l0 = road.locate(car.x, car.z, state.idx);
  const look = 8 + u * 0.62;
  for (let k = 0; k < GUIDE_N; k++) {
    const s = 2.6 + k * step;
    if (rails) {
      const f = l0.i + (l0.t + s) / DS, i = Math.floor(f), a = f - i;
      const p = road.get(i), q = road.get(i + 1);
      const w = smoothstep(0, look * 1.15, s);
      const d = lerp(l0.d, state.lineD, w);
      const cx = lerp(p.x, q.x, a), czz = lerp(p.z, q.z, a), cz = lerp(p.cz, q.cz, a), sx = lerp(p.sx, q.sx, a);
      pts.push([cx + cz * d, czz - sx * d]);
    } else {
      if (k === 0) { x += Math.sin(h) * 2.6; z += Math.cos(h) * 2.6; }
      else { h += kappa * step; x += Math.sin(h) * step; z += Math.cos(h) * step; }
      pts.push([x, z]);
    }
  }
  const half = 0.17;
  let hint = state.idx;
  for (let k = 0; k < GUIDE_N; k++) {
    const [px, pz] = pts[k];
    const [nx, nz] = pts[Math.min(GUIDE_N - 1, k + 1)];
    const [bx, bz] = pts[Math.max(0, k - 1)];
    let tx = nx - bx, tz = nz - bz; const tl = Math.hypot(tx, tz) || 1; tx /= tl; tz /= tl;
    const lx = tz, lz = -tx;
    const ll = road.locate(px, pz, hint); hint = ll.i;
    const y = road.groundY(px, pz, hint) + 0.05;
    const o = k * 6;
    pos[o] = px + lx * half; pos[o + 1] = y; pos[o + 2] = pz + lz * half;
    pos[o + 3] = px - lx * half; pos[o + 4] = y; pos[o + 5] = pz - lz * half;
    const fv = smoothstep(0, 4, k) * (1 - smoothstep(GUIDE_N * 0.55, GUIDE_N - 1, k));
    fade[k * 2] = fade[k * 2 + 1] = fv;
  }
  guideGeo.attributes.position.needsUpdate = true; guideGeo.attributes.fade.needsUpdate = true;
  // colour: cool when settled, amber when the governor or wall is working
  const warn = state.governing || state.wallHit > 0.2 || (!rails && Math.abs(state.d) > ROAD_HALF - 1);
  _gc.setHex(warn ? 0xffb35c : state.tier >= 3 ? 0xc9ffe8 : 0x9fe9ff);
  guideMat.uniforms.col.value.lerp(_gc, 1 - Math.exp(-dt * 5));
  state.wallHit = Math.max(0, state.wallHit - dt * 2);
}

// ---------- autopilot (title screen attract mode) ----------
function autopilot(dt) {
  const u = Math.max(car.u, 1);
  const look = 7 + u * 0.55;
  const tgt = road.get(state.idx + Math.round(look / DS));
  const lane = -LANE * 0.6;
  const tx = tgt.x + tgt.cz * lane, tz = tgt.z - tgt.sx * lane;
  const sy = Math.sin(car.yaw), cy = Math.cos(car.yaw);
  const dx = tx - car.x, dz = tz - car.z;
  const lz = dx * sy + dz * cy, lx = dx * cy - dz * sy;
  const curv = 2 * lx / (lx * lx + lz * lz);
  const delta = Math.atan(curv * car.L);
  input.steer = clamp(delta / car.maxSteer(car.u), -1, 1);
  const kmax = road.maxCurvatureAhead(state.idx, 30 + u * 2.5);
  const vt = Math.min(27, Math.sqrt(0.55 * G / Math.max(kmax, 1e-4)));
  input.throttle = clamp((vt - car.u) * 0.25, 0, 0.8);
  input.brake = car.u > vt + 2 ? clamp((car.u - vt) * 0.15, 0, 0.6) : 0;
  input.hand = false;
}

// ---------- flow ----------
const TIERS = [
  { name: 'Calm', at: 0 }, { name: 'Steady', at: 0.2 }, { name: 'Smooth', at: 0.42 }, { name: 'Flow', at: 0.64 }, { name: 'Zen', at: 0.86 },
];
let toastTimer = 0;
function toast(msg, warn = false) {
  const el = $('toast'); el.textContent = msg; el.classList.toggle('warn', warn); el.classList.add('show');
  clearTimeout(toastTimer); toastTimer = setTimeout(() => el.classList.remove('show'), 2200);
}
let biomeTimer = 0;
function showBiome(b) {
  const el = $('biome'); el.innerHTML = `<small>${b.sub}</small>${b.name}`; el.classList.add('show');
  clearTimeout(biomeTimer); biomeTimer = setTimeout(() => el.classList.remove('show'), 4200);
}

function updateFlow(dt) {
  const u = car.u;
  const steerRate = Math.abs(input.steer - state.lastSteer) / Math.max(dt, 1e-3);
  state.lastSteer = input.steer;
  state.steerRateF = lerp(state.steerRateF, steerRate, 1 - Math.exp(-dt * 6));
  const rough = clamp((state.steerRateF - 0.6) / 2.4, 0, 1);
  // straightness: how much you drift sideways relative to the road, and how far off your line you are
  const lp = road.get(state.idx);
  const relY = Math.atan2(Math.sin(car.yaw - lp.h), Math.cos(car.yaw - lp.h));
  const dRate = Math.abs(car.u * Math.sin(relY) + car.v * Math.cos(relY));
  state.dRateF = lerp(state.dRateF, dRate, 1 - Math.exp(-dt * 3));
  const lineRef = settings.assist === 'rails' ? state.lineD : (state.d > 0 ? LANE : -LANE);
  const lineErr = Math.abs(state.d - lineRef);
  const steady = 1 - clamp((state.dRateF - 0.25) / 1.1, 0, 1);
  const centred = 1 - clamp((lineErr - 0.35) / 1.5, 0, 1);
  const slip = Math.abs(car.slipR);
  const sliding = slip > 0.16;
  const brakeJab = input.brake > 0.85 && u > 25;
  let f = state.flow;
  if (state.offroad) {
    f -= dt * 0.28;
  } else if (u > 11) {
    const spd = clamp((u - 11) / 30, 0, 1);
    let gain = dt * (0.012 + 0.026 * spd) * steady * (0.3 + 0.7 * centred) * (1 - rough * 0.6) * (sliding ? 0.25 : 1) * (brakeJab ? 0.4 : 1) * (state.governing ? 0.5 : 1);
    if (state.shoulder) gain *= 0.3;
    f += gain;
    if (state.dRateF > 1.3) {
      f -= dt * 0.03;
      state.weaveT += dt;
      if (state.weaveT > 1.2 && state.mode === 'drive') { toast('Hold your line', true); state.weaveT = -6; }
    } else if (state.weaveT > 0) state.weaveT = Math.max(0, state.weaveT - dt);
    if (state.wallHit > 0.5) f -= dt * 0.06;
    if (rough > 0.6) f -= dt * 0.02;
    if (slip > 0.35) f -= dt * 0.08;
  } else {
    f -= dt * 0.012;
  }
  f -= dt * 0.002 * f;
  state.flow = clamp(f, 0, 1);
  // tier
  let tier = 0; for (let i = 0; i < TIERS.length; i++) if (state.flow >= TIERS[i].at) tier = i;
  if (tier > state.tier && state.mode === 'drive') { toast(TIERS[tier].name); audio.chime(true); }
  state.tier = tier;

  // distance & score
  const ds = Math.max(0, u) * dt;
  if (state.mode === 'drive') {
    state.dist += ds;
    if (!state.offroad) state.streak += ds; else { if (state.streak > 400) toast('Line broken — find it again', true); state.bestStreak = Math.max(state.bestStreak, state.streak); state.streak = 0; }
    state.score += ds * (tier + 1) * 0.1 * (state.offroad ? 0 : 1);
  }

  // corner tracking — reward clean, committed lines
  const p = road.get(state.idx);
  const ka = Math.abs(p.k);
  if (!state.corner && ka > 1 / 190) state.corner = { clean: true, peak: 0, s0: p.s, dir: Math.sign(p.k), minU: u };
  if (state.corner) {
    const c = state.corner;
    if (state.offroad) c.clean = false;
    c.peak = Math.max(c.peak, Math.abs(car.ay) / G);
    c.minU = Math.min(c.minU, u);
    if (ka < 1 / 330 || Math.sign(p.k) !== c.dir) {
      if (state.mode === 'drive' && c.clean && p.s - c.s0 > 50 && c.minU > 12) {
        const g = c.peak;
        const word = g > 0.85 ? 'Committed' : g > 0.6 ? 'Clean line' : 'Tidy';
        toast(`${word} · ${g.toFixed(2)} g`);
        state.flow = clamp(state.flow + 0.02 + g * 0.05, 0, 1);
        state.score += Math.round(g * 120 * (tier + 1));
      }
      state.corner = null;
    }
  }
}

// ---------- physics step ----------
const PHYS_DT = 1 / 240;
function physics(dt) {
  const loc = road.locate(car.x, car.z, state.idx);
  state.idx = loc.i; state.d = loc.d;
  const ad = Math.abs(loc.d);
  const bm = biomeAt(loc.p.s);
  const snowy = (BIOMES[bm.a].name === 'Glacier Pass' ? 1 - bm.t : 0) + (BIOMES[bm.b].name === 'Glacier Pass' ? bm.t : 0);
  state.onRoad = ad <= ROAD_HALF;
  state.shoulder = ad > ROAD_HALF && ad <= SHOULDER + 0.3;
  state.offroad = ad > SHOULDER + 0.3;
  let mu = 1.06, roll = 0.011;
  if (state.shoulder) { mu = 0.92; roll = 0.016; }
  if (state.offroad) { mu = lerp(0.66, 0.5, snowy); roll = 0.055; }
  const p = loc.p;
  const sb = road.slopeBank(loc);
  const rel = Math.cos(car.yaw - p.h);
  const surface = {
    mu, roll,
    slopeG: -G * sb.slope * rel,
    bankG: ad <= SHOULDER ? G * Math.sin(sb.bank) * rel : 0,
  };
  car.step(dt, input, surface, settings.assist !== 'off' || state.mode === 'title');
  if (railsOn()) railWall();
}

function softReset() {
  if (state.resetting) return;
  state.resetting = true;
  $('fade').classList.add('on');
  setTimeout(() => {
    const i = state.idx + 4;
    placeCar(i, 14);
    state.flow *= 0.5;
    $('fade').classList.remove('on');
    state.resetting = false;
  }, 520);
}

// ---------- car pose (visual suspension) ----------
const pose = { y: 0, pitch: 0, roll: 0, vp: 0, vr: 0, bp: 0, br: 0, init: false };
const _wp = new THREE.Vector3();
function groundAt(x, z) { return road.groundY(x, z, state.idx); }
function updatePose(dt) {
  const sy = Math.sin(car.yaw), cy = Math.cos(car.yaw);
  const fx = sy, fz = cy, lx = cy, lz = -sy;
  const hf = (groundAt(car.x + fx * 1.24 + lx * 0.79, car.z + fz * 1.24 + lz * 0.79) + groundAt(car.x + fx * 1.24 - lx * 0.79, car.z + fz * 1.24 - lz * 0.79)) / 2;
  const hr = (groundAt(car.x - fx * 1.36 + lx * 0.79, car.z - fz * 1.36 + lz * 0.79) + groundAt(car.x - fx * 1.36 - lx * 0.79, car.z - fz * 1.36 - lz * 0.79)) / 2;
  const hl = (groundAt(car.x + lx * 0.79, car.z + lz * 0.79));
  const hrr = (groundAt(car.x - lx * 0.79, car.z - lz * 0.79));
  const yT = (hf + hr) / 2;
  const pitchT = Math.atan2(hf - hr, 2.6);
  const rollT = Math.atan2(hl - hrr, 1.58);
  if (!pose.init) { pose.y = yT; pose.pitch = pitchT; pose.roll = rollT; pose.init = true; }
  const k = 1 - Math.exp(-dt * (state.offroad ? 14 : 22));
  pose.y = lerp(pose.y, yT, k); pose.pitch = lerp(pose.pitch, pitchT, k); pose.roll = lerp(pose.roll, rollT, k);
  // sprung body: weight transfer you can see
  const tp = clamp(-car.axF * 0.0075, -0.06, 0.06), tr = clamp(car.ay * 0.0085, -0.07, 0.07);
  pose.vp += ((tp - pose.bp) * 120 - pose.vp * 14) * dt; pose.bp += pose.vp * dt;
  pose.vr += ((tr - pose.br) * 110 - pose.vr * 12) * dt; pose.br += pose.vr * dt;
  // off-road jitter
  let jit = 0;
  if (state.offroad && car.speed > 2) jit = (Math.random() - 0.5) * 0.02 * Math.min(1, car.speed / 20);

  const R = carMesh.root;
  R.position.set(car.x, pose.y, car.z);
  R.rotation.set(0, 0, 0);
  R.rotation.order = 'YXZ';
  R.rotation.y = car.yaw;
  R.rotation.x = -pose.pitch;
  R.rotation.z = pose.roll;
  carMesh.body.rotation.set(pose.bp, 0, pose.br + jit);
  carMesh.body.position.y = jit * 2;
  carMesh.wheels.forEach((w) => {
    w.spin.rotation.x = car.wheelRot;
    if (w.front) w.pivot.rotation.y = car.steerAngle;
  });
  // lights
  const braking = input.brake > 0.05 && car.u > 0.5;
  if (carMesh.modeled) carMesh.headMat.emissiveIntensity = settings.camera === 'hood' && state.mode !== 'title' ? 0.35 : 2.2;
  carMesh.tailMat.emissiveIntensity = braking ? 7 : 2.2;
  const night = world.cur.headI;
  carMesh.spot.intensity = 30 + night * 340;
  carMesh.beamMat.uniforms.strength.value = Math.max(0, night - 0.15) * 0.06;
  carMesh.tailGlow.intensity = (braking ? 2.5 : 0.6) * night;
}

// ---------- camera ----------
const camState = { yaw: 0, pos: new THREE.Vector3(), look: new THREE.Vector3(), init: false, orbit: 0, fov: 60 };
const _t = new THREE.Vector3(), _l = new THREE.Vector3();
function updateCamera(dt) {
  const u = car.u;
  // yaw follows direction of travel — slides show the car's angle
  const velYaw = car.speed > 3 ? car.yaw + Math.atan2(car.v, Math.max(Math.abs(car.u), 0.1)) * 0.7 : car.yaw;
  let dy = velYaw - camState.yaw; dy = Math.atan2(Math.sin(dy), Math.cos(dy));
  if (!camState.init) { camState.yaw = velYaw; dy = 0; }
  camState.yaw += dy * (1 - Math.exp(-dt * 3.4));
  const sy = Math.sin(camState.yaw), cy = Math.cos(camState.yaw);
  const cp = carMesh.root.position;
  let dist = 6.6 + clamp(u, 0, 60) * 0.025, h = 2.3;
  if (settings.camera === 'far') { dist = 9.5 + clamp(u, 0, 60) * 0.03; h = 3.1; }
  if (state.mode === 'title') {
    camState.orbit += dt * 0.07;
    const o = Math.sin(camState.orbit) * 0.9 + 0.35;
    const os = Math.sin(camState.yaw + o), oc = Math.cos(camState.yaw + o);
    _t.set(cp.x - os * 10.5, cp.y + 2.2, cp.z - oc * 10.5);
    _l.set(cp.x + sy * 4, cp.y + 0.9, cp.z + cy * 4);
  } else if (settings.camera === 'hood') {
    const s2 = Math.sin(car.yaw), c2 = Math.cos(car.yaw);
    _t.set(cp.x + s2 * 0.75, cp.y + 1.2 + pose.bp * 0.3, cp.z + c2 * 0.75);
    _l.set(cp.x + s2 * 30, cp.y + 0.6, cp.z + c2 * 30);
  } else {
    _t.set(cp.x - sy * dist, cp.y + h, cp.z - cy * dist);
    _l.set(cp.x + sy * 9, cp.y + 0.8, cp.z + cy * 9);
  }
  // keep camera above ground
  const gy = groundAt(_t.x, _t.z) + 0.6;
  if (_t.y < gy) _t.y = gy;
  if (!camState.init) { camState.pos.copy(_t); camState.look.copy(_l); camState.init = true; }
  const hood = settings.camera === 'hood' && state.mode !== 'title';
  if (carMesh.paintMat) {
    const pm = carMesh.paintMat;
    pm.clearcoat = hood ? 0 : 1; pm.roughness = hood ? 0.8 : 0.38; pm.metalness = hood ? 0 : 0.15; pm.envMapIntensity = hood ? 0.35 : 1.0;
    pm.color.setHex(carMesh.paintHex); if (hood) pm.color.multiplyScalar(0.45);
  }
  const kp = hood ? 1 : 1 - Math.exp(-dt * 9), kl = hood ? 1 : 1 - Math.exp(-dt * 12);
  camState.pos.x = lerp(camState.pos.x, _t.x, hood ? 1 : 1 - Math.exp(-dt * 30));
  camState.pos.z = lerp(camState.pos.z, _t.z, hood ? 1 : 1 - Math.exp(-dt * 30));
  camState.pos.y = lerp(camState.pos.y, _t.y, kp);
  camState.look.lerp(_l, kl);
  camera.position.copy(camState.pos);
  if (state.offroad && car.speed > 3) camera.position.y += (Math.random() - 0.5) * 0.03;
  camera.lookAt(camState.look);
  const fovT = (hood ? 64 : 58) + clamp(u, 0, 60) * 0.2 + state.flow * 3;
  camState.fov = lerp(camState.fov, fovT, 1 - Math.exp(-dt * 2));
  camera.fov = camState.fov; camera.updateProjectionMatrix();
}

// ---------- HUD ----------
const hud = { el: $('hud'), frame: 0 };
function updateHud() {
  if (++hud.frame % 3) return;
  const mph = settings.units === 'mph';
  const v = Math.abs(car.u) * (mph ? 2.23694 : 3.6);
  $('speed').textContent = Math.round(v);
  $('speedU').textContent = mph ? 'mph' : 'km/h';
  $('gear').textContent = car.u < -0.3 ? 'R' : car.gear;
  const r = clamp((car.rpm - 800) / (car.redline - 800), 0, 1);
  const fg = $('tachFg'); fg.style.strokeDashoffset = 251.3 * (1 - r); fg.classList.toggle('red', car.rpm > 6600);
  $('pThr').style.setProperty('--v', input.throttle.toFixed(2));
  $('pBrk').style.setProperty('--v', input.brake.toFixed(2));
  const conv = mph ? 1 / 1609.34 : 1 / 1000;
  $('dist').textContent = (state.dist * conv).toFixed(1);
  $('streak').textContent = (state.streak * conv).toFixed(1);
  $('distU').textContent = $('streakU').textContent = mph ? 'mi' : 'km';
  $('score').textContent = Math.round(state.score).toLocaleString();
  $('flowTier').textContent = TIERS[state.tier].name;
  $('flowMult').textContent = '×' + (state.tier + 1);
  $('flowFill').style.width = (state.flow * 100).toFixed(1) + '%';
}

// ---------- menus ----------
function setSeg(name, val) {
  settings[name] = val;
  document.querySelectorAll(`.seg[data-setting="${name}"] button`).forEach((b) => b.classList.toggle('on', b.dataset.v === val));
  if (name === 'paint' && carMesh.setPaint) carMesh.setPaint(val);
  if (name === 'hud') { hud.el.classList.toggle('off', val === 'off'); hud.el.classList.toggle('minimal', val === 'minimal'); }
}
document.querySelectorAll('.seg').forEach((seg) => {
  seg.addEventListener('click', (e) => { const b = e.target.closest('button'); if (b) setSeg(seg.dataset.setting, b.dataset.v); });
});
Object.entries(settings).forEach(([k, v]) => setSeg(k, v));
function cycleCamera() { const order = ['chase', 'far', 'hood']; setSeg('camera', order[(order.indexOf(settings.camera) + 1) % 3]); camState.init = false; }
function cycleHud() { const order = ['on', 'minimal', 'off']; setSeg('hud', order[(order.indexOf(settings.hud) + 1) % 3]); }
const vols = () => audio.setVolumes($('musicVol').value / 100, $('sfxVol').value / 100);
$('musicVol').addEventListener('input', vols); $('sfxVol').addEventListener('input', vols);

function togglePause() {
  if (state.mode === 'drive') { state.mode = 'paused'; $('pause').classList.remove('hidden'); audio.setPaused(true); }
  else if (state.mode === 'paused') { state.mode = 'drive'; $('pause').classList.add('hidden'); audio.setPaused(false); clock.getDelta(); }
}
$('resumeBtn').addEventListener('click', togglePause);
$('resetBtn').addEventListener('click', () => { togglePause(); softReset(); });

function startDrive() {
  if (state.mode !== 'title') return;
  audio.init(); vols();
  $('fade').classList.add('on');
  $('title').classList.add('hidden');
  setTimeout(() => {
    world.resetChunks();
    placeCar(6, 0);
    world.updateChunks(state.idx, true);
    Object.assign(state, { mode: 'drive', flow: 0, score: 0, dist: 0, streak: 0, tier: 0, corner: null, biomeIndex: -1 });
    input.steer = 0; input.throttle = 0; input.brake = 0;
    camState.init = false;
    hud.el.style.opacity = '';
    if (isTouch) $('touch').classList.remove('hidden');
    $('fade').classList.remove('on');
  }, 650);
}
$('startBtn').addEventListener('click', startDrive);
hud.el.style.opacity = '0';

// ---------- loop ----------
const clock = new THREE.Clock();
let acc = 0, trailAcc = 0;
const _tl = new THREE.Vector3();
function update(dt) {
  if (!(dt > 0)) return;
  state.time += dt;
  if (state.mode === 'paused') return;
  if (state.mode === 'title' || state.auto) autopilot(dt); else { readInput(dt); if (railsOn()) rails(dt); }
  acc += dt;
  let n = 0;
  while (acc >= PHYS_DT && n < 40) { physics(PHYS_DT); acc -= PHYS_DT; n++; }
  if (n >= 40) acc = 0;
  if (state.mode === 'title' && Math.abs(state.d) > 9) placeCar(state.idx + 2, 18);

  updateFlow(dt);
  // stuck or far away -> reset
  if (state.mode === 'drive') {
    if (Math.abs(state.d) > 30) softReset();
    if (state.offroad && car.speed < 1.5) { state.stuckTimer += dt; if (state.stuckTimer > 3) { state.stuckTimer = 0; softReset(); } } else state.stuckTimer = 0;
  }
  // biome
  const bm = biomeAt(road.get(state.idx).s);
  world.applyBiome(bm);
  const dominant = bm.t < 0.5 ? bm.index : bm.index + 1;
  if (dominant !== state.biomeIndex) {
    if (state.mode === 'drive') showBiome(BIOMES[dominant % BIOMES.length]);
    state.biomeIndex = dominant;
  }
  world.updateChunks(state.idx);

  updatePose(dt);
  updateGuide(dt);
  updateCamera(dt);
  world.followCamera(camera, carMesh.root.position, null, dt);
  world.updateParticles(dt, camera.position, null, state.time);

  // trails at higher flow
  trailAcc += dt;
  const ti = 0.55 * clamp((state.flow - 0.6) / 0.3, 0, 1) * clamp((car.u - 12) / 15, 0, 1);
  trails.forEach((t) => (t.intensity = lerp(t.intensity, ti, 1 - Math.exp(-dt * 2))));
  if (trailAcc > 1 / 60) {
    trailAcc = 0;
    [-0.66, 0.66].forEach((x, i) => { _tl.set(x, 0.8, -2.34); carMesh.body.localToWorld(_tl); trails[i].push(_tl); });
  }
  trails.forEach((t) => t.update());

  // look & feel driven by flow
  const night = world.cur.headI;
  renderer.toneMappingExposure = world.cur.exposure;
  bloom.strength = world.cur.bloom + state.flow * 0.25;
  world.roadMat.emissiveIntensity = 0.02 + night * 0.25 + state.flow * 0.35;
  world.reflMat.emissiveIntensity = 0.8 + night * 2.2;
  world.reflMatR.emissiveIntensity = 0.6 + night * 1.8;
  world.chevMat.emissiveIntensity = 0.25 + night * 0.9;
  world.skyMat.uniforms.time.value = state.time;
  grade.uniforms.time.value = state.time;
  grade.uniforms.ab.value = clamp((car.u - 30) / 40, 0, 1) * 0.006;
  grade.uniforms.vig.value = 0.3 + state.flow * 0.12;

  audio.update({
    rpm: car.rpm, throttle: input.throttle, speed: car.speed, slip: Math.max(Math.abs(car.slipR), Math.abs(car.slipF) * 0.8) + car.spin * 0.3,
    offroad: state.offroad, shoulder: state.shoulder, flow: state.mode === 'drive' ? state.flow : 0.3, active: state.mode !== 'title', paused: state.mode === 'paused',
  });
  if (state.mode !== 'title') updateHud();
}

// debug overlay
const dbg = { el: $('debug'), frames: 0, last: performance.now(), fps: 0 };
function updateDebug() {
  dbg.frames++;
  const now = performance.now();
  if (now - dbg.last > 1000) {
    dbg.fps = dbg.frames * 1000 / (now - dbg.last); dbg.frames = 0; dbg.last = now;
    const i = renderer.info;
    dbg.el.textContent = `${dbg.fps.toFixed(0)} fps · ${i.render.calls} draws · ${(i.render.triangles / 1000).toFixed(0)}k tris · ${world.chunks.size} chunks`;
  }
}

function frame() {
  requestAnimationFrame(frame);
  const dt = Math.min(clock.getDelta(), 1 / 20);
  if (!state.manual) update(dt);
  renderer.info.reset();
  composer.render();
  updateDebug();
}

// init
placeCar(150, 20);
world.updateChunks(state.idx, true);
world.refreshEnv();
requestAnimationFrame(frame);

addEventListener('resize', () => {
  camera.aspect = innerWidth / innerHeight; camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight); composer.setSize(innerWidth, innerHeight);
});

// ---------- test hooks ----------
window.__step = (dt) => { state.manual = true; update(dt); };
window.advanceTime = (ms) => {
  state.manual = true;
  const steps = Math.max(1, Math.round(ms / (1000 / 60)));
  for (let i = 0; i < steps; i++) update(1 / 60);
  composer.render();
};
window.render_game_to_text = () => JSON.stringify({
  coords: 'world xz plane, y up; road runs toward +z; d>0 is left of centerline',
  mode: state.mode,
  car: { x: +car.x.toFixed(2), z: +car.z.toFixed(2), yaw: +car.yaw.toFixed(3), speed_ms: +car.u.toFixed(2), lat_ms: +car.v.toFixed(2), gear: car.gear, rpm: Math.round(car.rpm), slipR: +car.slipR.toFixed(3) },
  road: { sample: state.idx, d: +state.d.toFixed(2), curvature: +road.get(state.idx).k.toFixed(5), onRoad: state.onRoad, offroad: state.offroad },
  input: { steer: +input.steer.toFixed(2), throttle: +input.throttle.toFixed(2), brake: +input.brake.toFixed(2), hand: input.hand },
  flow: +state.flow.toFixed(3), tier: TIERS[state.tier].name, score: Math.round(state.score), dist_m: Math.round(state.dist), streak_m: Math.round(state.streak),
  biome: BIOMES[biomeAt(road.get(state.idx).s).a].name,
});
window.__game = { state, car, road, settings, input, keys, placeCar, startDrive, renderer, composer, scene, camera, world, carMesh };
