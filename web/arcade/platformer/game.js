/* =========================================================
   dHP — 3D First-Person Room
   game.js — steps 1-12 : jump, pillar, pack, cactus, interact
   ========================================================= */

import * as THREE from 'https://unpkg.com/three@0.161.0/build/three.module.js';

/* BLOCK 01 — TUNE knobs */
const TUNE = {
  ROOM: 10,
  WALL_H: 4,
  WALL_T: 0.2,
  LINE_W: 0.05,

  EYE: 1.6,
  RADIUS: 0.3,
  WALK_SPEED: 3.0,
  RUN_SPEED: 6.0,
  GRAVITY: -18,
  JUMP_VEL: 7.4,
  LAND_DIP: 0.14,
  LAND_K: 12,

  LAND_SHAKE_AMP: 0.025,
  LAND_SHAKE_FREQ: 22,
  LAND_SHAKE_DECAY: 9,

  ZONE: 0.06,
  ACCEL_K: 16,
  LOOK_SENS: 0.0032,
  PITCH_MAX: 1.45,
  STICK_K_DOWN: 22,
  STICK_K_UP: 18,

  FOV_BASE: 90,
  FOV_RUN: 110,
  FOV_K_IN: 6.5,
  FOV_K_OUT: 3.5,
  FOV_MAG_START: 0.1,
  FOV_SPRINT_BUMP: 0.075,

  TILT_PER_MPS: 0.018,
  TILT_K_IN: 14,
  TILT_K_OUT: 4.0,

  SHAKE_DEADZONE: 0.55,
  SHAKE_AMP_PITCH: 0.012,
  SHAKE_AMP_YAW:   0.014,
  SHAKE_AMP_ROLL:  0.016,

  CAM_K: 16,
  BOB_AMP: 0.05,
  BOB_SWAY: 0.7,
  BOB_RATE: 1.4,
  BOB_K: 9,
  BOB_CURVE: 2.0,
};

/* BLOCK 02 — DOM refs */
const canvas     = document.getElementById('scene');
const elStick    = document.getElementById('stick');
const elKnob     = document.getElementById('knob');
const elLook     = document.getElementById('look');
const elJump     = document.getElementById('btnJump');
const elSprint   = document.getElementById('btnSprint');
const elUse      = document.getElementById('btnInteract');
const elFlash    = document.getElementById('flash');

/* BLOCK 28 — HUD state + refs */
const elHpFill = document.getElementById('hpFill');
const elMpFill = document.getElementById('mpFill');
const elSpFill = document.getElementById('spFill');

elFlash.style.opacity = '1';

const state = {
  hp: 50,  hpMax: 100,
  mp: 100, mpMax: 100,
  sp: 100, spMax: 100,
};

let flashAmp   = 0;
let flashColor = '255,255,255';

let hpPulseT = 0;
let lastHp   = state.hp;

function applyHUD() {
  if (state.hp < lastHp) hpPulseT = 1;
  lastHp = state.hp;
  elHpFill.style.width = (state.hp / state.hpMax * 100) + '%';
  elMpFill.style.width = (state.mp / state.mpMax * 100) + '%';
  elSpFill.style.width = (state.sp / state.spMax * 100) + '%';
}
applyHUD();

elHpFill.style.transition = 'width 0.25s ease-out, filter 0.12s ease-out';
elMpFill.style.transition = 'width 0.25s ease-out';
elSpFill.style.transition = 'width 0.25s ease-out';

/* BLOCK 03 — Renderer */
const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: true,
  powerPreference: 'high-performance',
});
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 0.85;

/* BLOCK 04 — Scene + Camera */
const scene = new THREE.Scene();
scene.background = new THREE.Color(0xc9d4de);

const camera = new THREE.PerspectiveCamera(
  TUNE.FOV_BASE,
  window.innerWidth / window.innerHeight,
  0.05,
  500
);
camera.rotation.order = 'YXZ';

/* BLOCK 05 — Room : floor + walls + grid */
const room = new THREE.Group();
scene.add(room);

const matSurface = new THREE.MeshStandardMaterial({
  color: 0xd9d9d9,
  roughness: 0.95,
  metalness: 0.0,
});

const floor = new THREE.Mesh(new THREE.PlaneGeometry(20, 20), matSurface);
floor.rotation.x = -Math.PI / 2;
floor.receiveShadow = true;
room.add(floor);

const wallGeoX = new THREE.BoxGeometry(20 + TUNE.WALL_T * 2, TUNE.WALL_H, TUNE.WALL_T);
const wallGeoZ = new THREE.BoxGeometry(TUNE.WALL_T, TUNE.WALL_H, 20 + TUNE.WALL_T * 2);

const wallDefs = [
  { geo: wallGeoX, pos: [0, TUNE.WALL_H / 2, -10] },
  { geo: wallGeoX, pos: [0, TUNE.WALL_H / 2,  10] },
  { geo: wallGeoZ, pos: [-10, TUNE.WALL_H / 2, 0] },
  { geo: wallGeoZ, pos: [ 10, TUNE.WALL_H / 2, 0] },
];

for (const def of wallDefs) {
  const w = new THREE.Mesh(def.geo, matSurface);
  w.position.set(def.pos[0], def.pos[1], def.pos[2]);
  w.castShadow = true;
  w.receiveShadow = true;
  room.add(w);
}

const matLine = new THREE.MeshBasicMaterial({ color: 0x000000 });
const lineGeoX = new THREE.BoxGeometry(20, 0.004, TUNE.LINE_W);
const lineGeoZ = new THREE.BoxGeometry(TUNE.LINE_W, 0.004, 20);
const LINE_Y = 0.003;

for (let i = -10; i <= 10; i++) {
  const a = new THREE.Mesh(lineGeoX, matLine);
  a.position.set(0, LINE_Y, i);
  room.add(a);

  const b = new THREE.Mesh(lineGeoZ, matLine);
  b.position.set(i, LINE_Y, 0);
  room.add(b);
}

/* BLOCK 06 — Lighting */
const hemi = new THREE.HemisphereLight(0xbcd2e8, 0xd9d9d9, 0.55);
scene.add(hemi);

const ambient = new THREE.AmbientLight(0xffffff, 0.15);
scene.add(ambient);

const sun = new THREE.DirectionalLight(0xfff3e0, 0.85);
sun.position.set(60, 90, 40);
sun.target.position.set(0, 0, 0);
sun.castShadow = true;
sun.shadow.mapSize.set(2048, 2048);
sun.shadow.camera.near = 40;
sun.shadow.camera.far = 220;
sun.shadow.camera.left = -18;
sun.shadow.camera.right = 18;
sun.shadow.camera.top = 18;
sun.shadow.camera.bottom = -18;
sun.shadow.bias = -0.0008;
sun.shadow.normalBias = 0.02;
sun.shadow.camera.updateProjectionMatrix();
scene.add(sun);
scene.add(sun.target);

/* BLOCK 26 — Center pillar + ramp */
const PILLAR = { W: 3.0, D: 3.0, H: 1.4 };
const RAMP   = { RUN: 2.0, W: 1.5 };

const matBlock = new THREE.MeshStandardMaterial({
  color: 0xc4c4c4,
  roughness: 0.9,
  metalness: 0.0,
});

const pillar = new THREE.Mesh(
  new THREE.BoxGeometry(PILLAR.W, PILLAR.H, PILLAR.D),
  matBlock
);
pillar.position.set(0, PILLAR.H / 2, 0);
pillar.castShadow = true;
pillar.receiveShadow = true;
room.add(pillar);

const rampSlant = Math.hypot(RAMP.RUN, PILLAR.H);
const rampAngle = Math.atan2(PILLAR.H, RAMP.RUN);
const ramp = new THREE.Mesh(
  new THREE.BoxGeometry(RAMP.W, 0.12, rampSlant),
  matBlock
);
ramp.position.set(0, PILLAR.H / 2, PILLAR.D / 2 + RAMP.RUN / 2);
ramp.rotation.x = rampAngle;
ramp.castShadow = true;
ramp.receiveShadow = true;
room.add(ramp);

/* BLOCK 29 — Health pack */
const matPack = new THREE.MeshStandardMaterial({
  color: 0x2ecc71,
  emissive: 0x0a3d1f,
  roughness: 0.4,
  metalness: 0.1,
});

const healthPack = new THREE.Group();

const packBody = new THREE.Mesh(
  new THREE.BoxGeometry(0.45, 0.28, 0.45),
  matPack
);
packBody.castShadow = true;
healthPack.add(packBody);

const matCross = new THREE.MeshBasicMaterial({ color: 0xffffff });
const crossA = new THREE.Mesh(new THREE.BoxGeometry(0.30, 0.02, 0.10), matCross);
const crossB = new THREE.Mesh(new THREE.BoxGeometry(0.10, 0.02, 0.30), matCross);
crossA.position.y = 0.15;
crossB.position.y = 0.15;
healthPack.add(crossA);
healthPack.add(crossB);

healthPack.position.set(0, PILLAR.H + 0.4, 0);
healthPack.visible = false;
room.add(healthPack);

const PACK = {
  range: 0.85,
  heal: 30,
  spawnDelay: 1.2,
  respawn: 20,
};

let packTimer = PACK.spawnDelay;
let packAlive = false;
let packSpin  = 0;

/* BLOCK 31 — Cactus */
const matCactus = new THREE.MeshStandardMaterial({
  color: 0x2e7d32,
  roughness: 0.85,
  metalness: 0.0,
});

const CACTUS_POS = new THREE.Vector3(8, 0, 8);
const CACTUS = { range: 0.9, tick: 0.5, dmg: 2 };

const cactus = new THREE.Group();
cactus.position.copy(CACTUS_POS);

const trunk = new THREE.Mesh(
  new THREE.CylinderGeometry(0.28, 0.32, 1.6, 12),
  matCactus
);
trunk.position.y = 0.8;
trunk.castShadow = true;
trunk.receiveShadow = true;
cactus.add(trunk);

const armGeo = new THREE.CylinderGeometry(0.14, 0.16, 0.6, 10);

const armA = new THREE.Mesh(armGeo, matCactus);
armA.position.set(-0.36, 1.05, 0);
armA.rotation.z = Math.PI / 2.4;
armA.castShadow = true;
cactus.add(armA);

const armB = new THREE.Mesh(armGeo, matCactus);
armB.position.set(0.36, 0.85, 0);
armB.rotation.z = -Math.PI / 2.4;
armB.castShadow = true;
cactus.add(armB);

room.add(cactus);

let cactusTickT   = 0;
let cactusInRange = false;

/* BLOCK 33 — Step 13 : floating particles */
const PARTICLE_COUNT = 40;

const pGeo = new THREE.BufferGeometry();
const pPos = new Float32Array(PARTICLE_COUNT * 3);
const pDrift = new Float32Array(PARTICLE_COUNT);
const pSway = new Float32Array(PARTICLE_COUNT);

for (let i = 0; i < PARTICLE_COUNT; i++) {
  pPos[i*3+0] = (Math.random() - 0.5) * 18;
  pPos[i*3+1] = 0.2 + Math.random() * 1.0;
  pPos[i*3+2] = (Math.random() - 0.5) * 18;
  pDrift[i]   = 0.02 + Math.random() * 0.04;
  pSway[i]    = Math.random() * Math.PI * 2;
}
pGeo.setAttribute('position', new THREE.BufferAttribute(pPos, 3));

const pMat = new THREE.PointsMaterial({
  color: 0xffffff,
  size: 0.05,
  transparent: true,
  opacity: 0.55,
  depthWrite: false,
  sizeAttenuation: true,
});

const particles = new THREE.Points(pGeo, pMat);
particles.frustumCulled = false;
room.add(particles);

let pTime = 0;

/* BLOCK 07 — Player state */
const player = {
  pos: new THREE.Vector3(0, 0, 6.5),
  vel: new THREE.Vector3(0, 0, 0),
  yaw: 0,
  pitch: 0,
  grounded: true,
};

/* BLOCK 27 — ground height (pillar + ramp) */
function groundHeightAt(x, z) {
  const hw = PILLAR.W / 2;
  const hd = PILLAR.D / 2;
  if (Math.abs(x) <= hw && Math.abs(z) <= hd) {
    return PILLAR.H;
  }
  if (Math.abs(x) <= RAMP.W / 2 && z > hd && z <= hd + RAMP.RUN) {
    const t = 1 - (z - hd) / RAMP.RUN;
    return PILLAR.H * t;
  }
  return 0;
}

/* BLOCK 08 — Input state + joystick */
const input = {
  stick:  { active: false, id: null, x: 0, y: 0, tx: 0, ty: 0 },
  look:   { id: null, lastX: 0, lastY: 0 },
  sprint: false,
  jumpQueued: false,
};

let stickCenterX = 0;
let stickCenterY = 0;
let stickRadius  = 88;
let knobTravel   = 56;

function stickStart(e) {
  const r = elStick.getBoundingClientRect();
  stickCenterX = r.left + r.width / 2;
  stickCenterY = r.top + r.height / 2;
  stickRadius  = r.width / 2;

  const kr = elKnob.getBoundingClientRect();
  const knobRadius = kr.width / 2;
  knobTravel = Math.max(8, stickRadius - knobRadius);

  input.stick.active = true;
  input.stick.id = e.pointerId;
  try { elStick.setPointerCapture(e.pointerId); } catch (_) {}
  stickMove(e);
}

function stickMove(e) {
  if (!input.stick.active || e.pointerId !== input.stick.id) return;

  let dx = (e.clientX - stickCenterX) / stickRadius;
  let dy = (e.clientY - stickCenterY) / stickRadius;
  const m = Math.hypot(dx, dy);
  if (m > 1) { dx /= m; dy /= m; }

  input.stick.tx = dx;
  input.stick.ty = dy;
}

function stickEnd(e) {
  if (e.pointerId !== input.stick.id) return;
  input.stick.active = false;
  input.stick.id = null;
  input.stick.tx = 0;
  input.stick.ty = 0;
}

elStick.addEventListener('pointerdown', (e) => { e.preventDefault(); stickStart(e); });
elStick.addEventListener('pointermove', (e) => { e.preventDefault(); stickMove(e); });
elStick.addEventListener('pointerup', stickEnd);
elStick.addEventListener('pointercancel', stickEnd);
elStick.addEventListener('lostpointercapture', stickEnd);

/* BLOCK 09 — Look (right drag) */
function lookStart(e) {
  if (input.look.id !== null) return;
  input.look.id = e.pointerId;
  input.look.lastX = e.clientX;
  input.look.lastY = e.clientY;
  try { elLook.setPointerCapture(e.pointerId); } catch (_) {}
}

function lookMove(e) {
  if (e.pointerId !== input.look.id) return;

  const dx = e.clientX - input.look.lastX;
  const dy = e.clientY - input.look.lastY;
  input.look.lastX = e.clientX;
  input.look.lastY = e.clientY;

  player.yaw   -= dx * TUNE.LOOK_SENS;
  player.pitch -= dy * TUNE.LOOK_SENS;
  player.pitch = Math.max(-TUNE.PITCH_MAX, Math.min(TUNE.PITCH_MAX, player.pitch));
}

function lookEnd(e) {
  if (e.pointerId !== input.look.id) return;
  input.look.id = null;
}

elLook.addEventListener('pointerdown', (e) => { e.preventDefault(); lookStart(e); });
elLook.addEventListener('pointermove', (e) => { e.preventDefault(); lookMove(e); });
elLook.addEventListener('pointerup', lookEnd);
elLook.addEventListener('pointercancel', lookEnd);
elLook.addEventListener('lostpointercapture', lookEnd);

/* BLOCK 10 — Jump button */
elJump.addEventListener('pointerdown', (e) => {
  e.preventDefault();
  input.jumpQueued = true;
});

/* BLOCK 11 — Sprint button (hold) + swipe-into-JUMP */
let sprintPointerId = null;
let sprintInJumpZone = false;

function sprintEnd(e) {
  if (e.pointerId !== sprintPointerId) return;
  sprintPointerId = null;
  input.sprint = false;
  sprintInJumpZone = false;
  elSprint.classList.remove('held');
}

function sprintHitJump(e) {
  const r = elJump.getBoundingClientRect();
  return (
    e.clientX >= r.left && e.clientX <= r.right &&
    e.clientY >= r.top  && e.clientY <= r.bottom
  );
}

elSprint.addEventListener('pointerdown', (e) => {
  e.preventDefault();
  if (sprintPointerId !== null) return;
  sprintPointerId = e.pointerId;
  try { elSprint.setPointerCapture(e.pointerId); } catch (_) {}
  input.sprint = true;
  sprintInJumpZone = false;
  elSprint.classList.add('held');
});

elSprint.addEventListener('pointermove', (e) => {
  if (e.pointerId !== sprintPointerId) return;
  const inside = sprintHitJump(e);
  if (inside && !sprintInJumpZone) {
    input.jumpQueued = true;
    sprintInJumpZone = true;
  } else if (!inside) {
    sprintInJumpZone = false;
  }
});

elSprint.addEventListener('pointerup', sprintEnd);
elSprint.addEventListener('pointercancel', sprintEnd);
elSprint.addEventListener('lostpointercapture', sprintEnd);

/* BLOCK 12 — Interact button */
let interactQueued = false;

elUse.addEventListener('pointerdown', (e) => {
  e.preventDefault();
  interactQueued = true;
});

/* BLOCK 13 — Resize + gesture */
function onResize() {
  const w = window.innerWidth;
  const h = window.innerHeight;
  camera.aspect = w / h;
  camera.updateProjectionMatrix();
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.setSize(w, h);
}

window.addEventListener('resize', onResize);
window.addEventListener('orientationchange', () => setTimeout(onResize, 250));

document.addEventListener('gesturestart', (e) => e.preventDefault());
document.addEventListener('touchmove', (e) => {
  if (e.touches.length > 1) e.preventDefault();
}, { passive: false });

/* BLOCK 14 — Camera state vars */
const camBase = new THREE.Vector3(
  player.pos.x,
  player.pos.y + TUNE.EYE,
  player.pos.z
);

let bobPhase  = 0;
let bobAmp    = 0;
let shakeTime = 0;
let fov       = TUNE.FOV_BASE;
let tiltCur   = 0;
let landDip   = 0;

let landShake  = 0;
let landShakeT = 0;

/* BLOCK 15-25, 30, 32 — Frame loop */
const clock = new THREE.Clock();

function frame() {
  requestAnimationFrame(frame);

  const dt = Math.min(clock.getDelta(), 0.05);

/* BLOCK 15 — stick smoothing */
  const kStick = 1 - Math.exp(
    -(input.stick.active ? TUNE.STICK_K_DOWN : TUNE.STICK_K_UP) * dt
  );
  input.stick.x += (input.stick.tx - input.stick.x) * kStick;
  input.stick.y += (input.stick.ty - input.stick.y) * kStick;

  elKnob.style.transform =
    'translate(-50%, -50%) translate(' +
    (input.stick.x * knobTravel).toFixed(1) + 'px, ' +
    (input.stick.y * knobTravel).toFixed(1) + 'px)';

/* BLOCK 16 — movement input -> world */
  const rawMag = Math.hypot(input.stick.x, input.stick.y);

  let nx = 0, ny = 0;
  if (rawMag > 0.0001) {
    nx = input.stick.x / rawMag;
    ny = input.stick.y / rawMag;
  }

  const mag = Math.max(
    0,
    Math.min(1, (rawMag - TUNE.ZONE) / (1 - TUNE.ZONE))
  );

  const sinY = Math.sin(player.yaw);
  const cosY = Math.cos(player.yaw);

  const fx = -sinY, fz = -cosY;
  const rx =  cosY, rz = -sinY;

  const fwd    = -ny;
  const strafe =  nx;

  let dirX = fx * fwd + rx * strafe;
  let dirZ = fz * fwd + rz * strafe;
  const dl = Math.hypot(dirX, dirZ);
  if (dl > 0.0001) { dirX /= dl; dirZ /= dl; } else { dirX = 0; dirZ = 0; }

  const maxSpeed    = input.sprint ? TUNE.RUN_SPEED : TUNE.WALK_SPEED;
  const targetSpeed = maxSpeed * Math.pow(mag, 1.5);
  const kAcc        = 1 - Math.exp(-TUNE.ACCEL_K * dt);

  player.vel.x += (dirX * targetSpeed - player.vel.x) * kAcc;
  player.vel.z += (dirZ * targetSpeed - player.vel.z) * kAcc;

/* BLOCK 17 — jump */
  if (input.jumpQueued && player.grounded) {
    player.vel.y = TUNE.JUMP_VEL;
    player.grounded = false;
  }
  input.jumpQueued = false;

/* BLOCK 18 — gravity + landing + shake impulse */
  player.vel.y += TUNE.GRAVITY * dt;
  player.pos.y += player.vel.y * dt;

  const wasGrounded = player.grounded;
  const gY = groundHeightAt(player.pos.x, player.pos.z);

  if (player.pos.y <= gY) {
    if (!wasGrounded && player.vel.y < -2) {
      const impact = Math.min(1, -player.vel.y / 9);
      landDip   = -TUNE.LAND_DIP * impact;
      landShake =  TUNE.LAND_SHAKE_AMP * impact;
      landShakeT = 0;
    }
    player.pos.y = gY;
    player.vel.y = 0;
    player.grounded = true;
  } else {
    player.grounded = false;
  }

  landDip += (0 - landDip) * (1 - Math.exp(-TUNE.LAND_K * dt));

/* BLOCK 19 — landing shake decay */
  let lsPitch = 0, lsYaw = 0, lsRoll = 0;
  if (landShake > 0.0001) {
    landShakeT += dt;
    const t = landShakeT * TUNE.LAND_SHAKE_FREQ;
    lsPitch = Math.sin(t * 1.0) * landShake;
    lsYaw   = Math.sin(t * 1.3) * landShake * 0.7;
    lsRoll  = Math.sin(t * 0.9) * landShake * 0.9;
    landShake *= Math.exp(-TUNE.LAND_SHAKE_DECAY * dt);
  } else {
    landShake = 0;
  }

/* BLOCK 20 — integrate + wall clamp + pillar push-out */
  player.pos.x += player.vel.x * dt;
  player.pos.z += player.vel.z * dt;

  const lim = TUNE.ROOM - TUNE.RADIUS;
  if (player.pos.x >  lim) { player.pos.x =  lim; player.vel.x = 0; }
  if (player.pos.x < -lim) { player.pos.x = -lim; player.vel.x = 0; }
  if (player.pos.z >  lim) { player.pos.z =  lim; player.vel.z = 0; }
  if (player.pos.z < -lim) { player.pos.z = -lim; player.vel.z = 0; }

  const pxs = PILLAR.W / 2 + TUNE.RADIUS;
  const pzs = PILLAR.D / 2 + TUNE.RADIUS;
  if (player.pos.y < PILLAR.H - 0.3 &&
      Math.abs(player.pos.x) < pxs &&
      Math.abs(player.pos.z) < pzs) {
    const dxP = pxs - Math.abs(player.pos.x);
    const dzP = pzs - Math.abs(player.pos.z);
    if (dxP < dzP) {
      player.pos.x += Math.sign(player.pos.x || 1) * dxP;
      player.vel.x = 0;
    } else {
      player.pos.z += Math.sign(player.pos.z || 1) * dzP;
      player.vel.z = 0;
    }
  }

  const hSpeed    = Math.hypot(player.vel.x, player.vel.z);
  const speedNorm = Math.min(1, hSpeed / TUNE.RUN_SPEED);

/* BLOCK 30 — pack spawn + pickup + flash */
  packTimer -= dt;

  if (!packAlive && packTimer <= 0) {
    packAlive = true;
    healthPack.visible = true;
  }

  if (packAlive) {
    packSpin += dt * 1.4;
    healthPack.rotation.y = packSpin;
    healthPack.position.y = PILLAR.H + 0.4 + Math.sin(packSpin * 1.6) * 0.06;

    const dxPk = player.pos.x - healthPack.position.x;
    const dzPk = player.pos.z - healthPack.position.z;
    const dPk  = Math.hypot(dxPk, dzPk);

    if (dPk < PACK.range &&
        player.pos.y > PILLAR.H - 0.4 &&
        state.hp < state.hpMax) {
      state.hp = Math.min(state.hpMax, state.hp + PACK.heal);
      applyHUD();
      flashColor = '80,255,140';
      flashAmp   = 0.7;
      packAlive  = false;
      healthPack.visible = false;
      packTimer  = PACK.respawn;
    }
  }

  flashAmp *= Math.exp(-2.5 * dt);
  elFlash.style.background =
    'radial-gradient(circle at 50% 50%,' +
    'rgba(' + flashColor + ',0) 30%,' +
    'rgba(' + flashColor + ',' + Math.min(1, flashAmp).toFixed(3) + ') 100%)';

/* BLOCK 32 — cactus + run-into + interact */
  const dxC = player.pos.x - CACTUS_POS.x;
  const dzC = player.pos.z - CACTUS_POS.z;
  const dC  = Math.hypot(dxC, dzC);

  const nowInRange = dC < CACTUS.range && player.pos.y < 0.5;

  if (nowInRange && !cactusInRange && hSpeed > 4.0) {
    state.hp = Math.max(0, state.hp - 5);
    applyHUD();
    flashColor = '255,20,20';
    flashAmp   = 0.95;
    cactusTickT = 0;
  }
  cactusInRange = nowInRange;

  if (cactusInRange && state.hp > 0) {
    cactusTickT += dt;
    if (cactusTickT >= CACTUS.tick) {
      cactusTickT -= CACTUS.tick;
      state.hp = Math.max(0, state.hp - CACTUS.dmg);
      applyHUD();
      flashColor = '255,30,30';
      flashAmp   = 0.65;
    }
  } else {
    cactusTickT = 0;
  }

  elUse.style.display = cactusInRange ? 'block' : 'none';

  if (interactQueued) {
    interactQueued = false;
    if (cactusInRange && state.hp > 0) {
      state.hp = Math.max(0, state.hp - 2);
      applyHUD();
      flashColor = '255,40,40';
      flashAmp   = 0.75;
      cactusTickT = 0;
    }
  }

/* BLOCK 34 — Frame : particle drift */
  pTime += dt;
  const pAttr = particles.geometry.attributes.position;
  for (let i = 0; i < PARTICLE_COUNT; i++) {
    let y = pAttr.array[i*3+1] + pDrift[i] * dt;
    if (y > 1.4) y = 0.15;
    pAttr.array[i*3+1] = y;
    pAttr.array[i*3+0] += Math.sin(pTime * 0.6 + pSway[i]) * 0.03 * dt;
    pAttr.array[i*3+2] += Math.cos(pTime * 0.5 + pSway[i]) * 0.03 * dt;
  }
  pAttr.needsUpdate = true;

/* BLOCK 35 — Frame : hp bar pulse decay */
  hpPulseT *= Math.exp(-8 * dt);
  if (hpPulseT > 0.02) {
    elHpFill.style.filter =
      'brightness(' + (1 + hpPulseT * 1.4).toFixed(2) + ')';
  } else if (elHpFill.style.filter) {
    elHpFill.style.filter = '';
  }

/* BLOCK 21 — FOV */
  const fovFac = Math.max(0, Math.min(1,
    (rawMag - TUNE.FOV_MAG_START) / (1 - TUNE.FOV_MAG_START)
  ));

  const sprintBonus = input.sprint ? TUNE.FOV_SPRINT_BUMP * TUNE.FOV_RUN : 0;
  const fovTarget = TUNE.FOV_BASE
                  + (TUNE.FOV_RUN - TUNE.FOV_BASE) * fovFac
                  + sprintBonus;

  const fovK = (fovTarget > fov) ? TUNE.FOV_K_IN : TUNE.FOV_K_OUT;

  fov += (fovTarget - fov) * (1 - Math.exp(-fovK * dt));
  if (Math.abs(fov - camera.fov) > 0.005) {
    camera.fov = fov;
    camera.updateProjectionMatrix();
  }

/* BLOCK 22 — camera tilt */
  const latVel = player.vel.x * rx + player.vel.z * rz;
  const tiltTarget = -latVel * TUNE.TILT_PER_MPS;

  const kTilt = (Math.abs(tiltTarget) > Math.abs(tiltCur))
    ? TUNE.TILT_K_IN
    : TUNE.TILT_K_OUT;

  tiltCur += (tiltTarget - tiltCur) * (1 - Math.exp(-kTilt * dt));

/* BLOCK 23 — running shake */
  const shakeFac = Math.max(0,
    (speedNorm - TUNE.SHAKE_DEADZONE) / (1 - TUNE.SHAKE_DEADZONE)
  );

  let shPitch = 0, shYaw = 0, shRoll = 0;
  if (shakeFac > 0.001) {
    shakeTime += dt;
    const t = shakeTime;
    shPitch = (Math.sin(t * 11.3) * 0.6 + Math.sin(t * 27.7) * 0.4) * shakeFac * TUNE.SHAKE_AMP_PITCH;
    shYaw   = (Math.sin(t * 14.1) * 0.6 + Math.sin(t * 31.3) * 0.4) * shakeFac * TUNE.SHAKE_AMP_YAW;
    shRoll  = (Math.sin(t *  9.7) * 0.6 + Math.sin(t * 23.1) * 0.4) * shakeFac * TUNE.SHAKE_AMP_ROLL;
  }

/* BLOCK 24 — walk bob */
  const bobNorm   = Math.pow(speedNorm, TUNE.BOB_CURVE);
  const bobTarget = (player.grounded ? bobNorm : 0) * TUNE.BOB_AMP;

  bobAmp   += (bobTarget - bobAmp) * (1 - Math.exp(-TUNE.BOB_K * dt));
  bobPhase += hSpeed * dt * TUNE.BOB_RATE;

  const bobY    = Math.sin(bobPhase) * bobAmp;
  const bobX    = Math.sin(bobPhase * 0.5) * bobAmp * TUNE.BOB_SWAY;
  const bobRoll = -Math.sin(bobPhase * 0.5) * bobAmp * 0.5;

/* BLOCK 25 — camera apply + render */
  const kCam = 1 - Math.exp(-TUNE.CAM_K * dt);

  camBase.x += (player.pos.x - camBase.x) * kCam;
  camBase.y += (player.pos.y + TUNE.EYE - camBase.y) * kCam;
  camBase.z += (player.pos.z - camBase.z) * kCam;

  camera.position.set(
    camBase.x + rx * bobX,
    camBase.y + bobY + landDip,
    camBase.z + rz * bobX
  );

  camera.rotation.set(
    player.pitch + shPitch + lsPitch,
    player.yaw   + shYaw   + lsYaw,
    bobRoll      + shRoll  + tiltCur + lsRoll
  );

  renderer.render(scene, camera);
}

frame();