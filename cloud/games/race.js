// The racing rigs' lobby and races, run by the room (upgrade plan item 1:
// two players on the two rigs race each other, Mario Kart style). Each phone
// drives its own car (instant response); the room decides who is on which
// rig, which car they picked, when everyone is ready, when the lights go out
// (one server time for both) and the results. Pure functions on a plain
// object, so cloud/room.js stays small and tools/room_test.mjs can test it.
//
// Shared with the phones as the room-owned world key `rc` (16 numbers):
//   [phase, goAt (server s), laps, seed,
//    rig 0: slot, car, paint, rim, state, time (s),
//    rig 1: slot, car, paint, rim, state, time (s)]
// phase 0 = lobby, 1 = a race is on. slot -1 = nobody on that rig.
// state 0 = in the garage, 1 = ready, 2 = racing, 3 = finished (time = their
// race time), 4 = didn't finish.

export const CARS = 3, PAINTS = 12, RIMS = 8;
export const LAPS = 3;
export const LIGHTS_MS = 1500 + 5 * 900 + 400;  // from "race on" to the lights' last one (the C++ grid)
const LEAD_MS = 1200;                          // network slack before the lights start
const AFTER_FIRST_MS = 60 * 1000;              // the others get this long once someone finished
const MAX_RACE_MS = 20 * 60 * 1000;
const MIN_LAP_MS = 25 * 1000;                  // no lap of the ring is faster than this

export const GARAGE = 0, READY = 1, RACING = 2, FINISHED = 3, DNF = 4;

export function empty() {
  return { phase: 0, goAt: 0, laps: LAPS, seed: 0, firstFinish: 0, rigs: [null, null] };
}

export function values(r) {
  const out = [r.phase, r.goAt / 1000, r.laps, r.seed];
  for (const e of r.rigs) {
    if (!e) out.push(-1, 0, 0, 0, 0, 0);
    else out.push(e.slot, e.car, e.paint, e.rim, e.state, Math.round(e.time) / 1000);
  }
  return out;
}

// Back from the saved world key (after the room slept).
export function restore(v) {
  const r = empty();
  if (!Array.isArray(v) || v.length < 16) return r;
  r.phase = v[0] | 0;
  r.goAt = v[1] * 1000;
  r.laps = v[2] | 0 || LAPS;
  r.seed = v[3] | 0;
  for (let k = 0; k < 2; k++) {
    const b = 4 + k * 6;
    if (v[b] < 0) continue;
    r.rigs[k] = { slot: v[b] | 0, car: v[b + 1] | 0, paint: v[b + 2] | 0, rim: v[b + 3] | 0, state: v[b + 4] | 0, time: v[b + 5] * 1000 };
  }
  return r;
}

const int = (x, n) => (Number.isInteger(x) && x >= 0 && x < n ? x : null);

// Someone on a rig did something. `holds(rig)` says whether they hold that
// rig's claim (machine.rig0 / machine.rig1). → {ok, why?, changed}
export function act(r, slot, body, now, holds, rand = Math.random) {
  const op = body.op;
  const rig = body.rig === 0 || body.rig === 1 ? body.rig : -1;
  if (rig < 0) return { ok: 0, why: 'bad rig' };
  if (!holds(rig)) return { ok: 0, why: 'not on that rig' };
  let e = r.rigs[rig];
  if (e && e.slot !== slot) e = null;  // (someone else's old entry: replaced below)
  const pick = () => {
    const car = int(body.car, CARS), paint = int(body.paint, PAINTS), rim = int(body.rim, RIMS);
    if (car === null || paint === null || rim === null) return false;
    e.car = car; e.paint = paint; e.rim = rim;
    return true;
  };
  if (op === 'join') {
    // Into the garage (also "race again" after a race). Picks can change here.
    if (!e) e = r.rigs[rig] = { slot, car: 0, paint: 0, rim: 0, state: GARAGE, time: 0 };
    if (e.state === RACING) return { ok: 0, why: 'you are racing' };
    if (!pick()) return { ok: 0, why: 'bad car' };
    e.state = GARAGE;
    e.time = 0;
    return { ok: 1, changed: true };
  }
  if (!e && op === 'ready') e = r.rigs[rig] = { slot, car: 0, paint: 0, rim: 0, state: GARAGE, time: 0 };
  if (!e) return { ok: 0, why: 'not in the garage' };
  if (op === 'ready') {
    if (e.state === RACING) return { ok: 0, why: 'you are racing' };
    if (!pick()) return { ok: 0, why: 'bad car' };
    e.state = READY;
    e.time = 0;
    // "Race the bots": just you, the other rig doesn't hold you up.
    if (body.solo && r.phase === 0) {
      for (const x of r.rigs) if (x && x !== e && x.state === READY) x.state = GARAGE;
      startRace(r, now, rand);
    } else {
      maybeStart(r, now, rand);
    }
    return { ok: 1, changed: true };
  }
  if (op === 'unready') {
    if (e.state !== READY) return { ok: 1, changed: false };
    e.state = GARAGE;
    return { ok: 1, changed: true };
  }
  if (op === 'finish') {
    if (r.phase !== 1 || e.state !== RACING) return { ok: 0, why: 'not racing' };
    const ms = body.ms;
    if (!Number.isFinite(ms) || ms < r.laps * MIN_LAP_MS) return { ok: 0, why: 'too fast' };
    if (now < r.goAt + ms * 0.9 - 2000) return { ok: 0, why: 'too soon' };  // can't finish before the time has passed
    e.state = FINISHED;
    e.time = ms;
    if (!r.firstFinish) r.firstFinish = now;
    endIfDone(r);
    return { ok: 1, changed: true };
  }
  if (op === 'leave') {
    leave(r, slot);
    return { ok: 1, changed: true };
  }
  return { ok: 0, why: 'bad op' };
}

// Everyone on the rigs is ready → the race starts (true if it did).
export function maybeStart(r, now, rand = Math.random) {
  if (r.phase !== 0) return false;
  const on = r.rigs.filter((x) => x);
  if (!on.length || !on.every((x) => x.state === READY)) return false;
  startRace(r, now, rand);
  return true;
}

function startRace(r, now, rand) {
  r.phase = 1;
  r.seed = Math.floor(rand() * 1e6);
  // The lights hold a random moment after the fifth one, like the C++ grid
  // (world.cpp start: 5 * 900 + 400 + seed % 700 ms after the lights start).
  r.goAt = now + LEAD_MS + LIGHTS_MS + (r.seed % 700);
  r.firstFinish = 0;
  for (const e of r.rigs) {
    if (!e) continue;
    if (e.state === READY) { e.state = RACING; e.time = 0; } else if (e.state !== GARAGE) e.state = GARAGE;
  }
}

function endIfDone(r) {
  if (r.phase !== 1) return;
  if (r.rigs.some((e) => e && e.state === RACING)) return;
  r.phase = 0;
  r.firstFinish = 0;
}

// They got up from the rig (or left the bar).
export function leave(r, slot) {
  for (let k = 0; k < 2; k++) if (r.rigs[k] && r.rigs[k].slot === slot) r.rigs[k] = null;
  endIfDone(r);
}

// When the room should look again (0 = never): the stragglers' time limit.
export function deadline(r) {
  if (r.phase !== 1) return 0;
  const cap = r.goAt + MAX_RACE_MS;
  return r.firstFinish ? Math.min(cap, r.firstFinish + AFTER_FIRST_MS) : cap;
}

// Time's up: whoever is still racing didn't finish. → changed?
export function check(r, now) {
  const d = deadline(r);
  if (!d || now < d) return false;
  for (const e of r.rigs) if (e && e.state === RACING) { e.state = DNF; e.time = 0; }
  endIfDone(r);
  return true;
}
