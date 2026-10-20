// Darts, run by the room (the phone only says where it aimed): 301, three
// darts a turn, finish on exactly zero; going below zero or to 1 is a bust
// (the turn doesn't count). Same board and rules as app/darts.cpp, which
// now only draws what the room decides (and scores alone when offline).

export const SPREAD = 0.022;  // metres: how far from the aim a dart lands (on average)
export const START = 301;
const NUMBERS = [20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5];

// Score of a spot on the board (u = right, v = up, metres from the centre,
// as seen by the thrower) → {pts, label}.
export function score(u, v) {
  const d = Math.sqrt(u * u + v * v);
  if (d > 0.170) return { pts: 0, label: 'Miss' };
  if (d < 0.0064) return { pts: 50, label: 'BULL' };
  if (d < 0.016) return { pts: 25, label: '25' };
  let ang = Math.atan2(u, v) * 180 / Math.PI;
  if (ang < 0) ang += 360;
  const n = NUMBERS[Math.floor((ang + 9) / 18) % 20];
  if (d > 0.099 && d < 0.107) return { pts: n * 3, label: 'T' + n };
  if (d > 0.162) return { pts: n * 2, label: 'D' + n };
  return { pts: n, label: String(n) };
}

// Your hand isn't perfect: roughly -1..1, bunched toward 0 (like the C++).
function wobble(rand) {
  let s = 0;
  for (let i = 0; i < 4; i++) s += rand() - 0.5;
  return s;
}

export function newSession() {
  return { left: START, turnStart: START, turn: [], games: 0, best: 0, thrown: 0, gameId: newId() };
}

const newId = () => Math.random().toString(36).slice(2, 10) + Date.now().toString(36);

// What a finished game pays (cents): $50 for the fastest finish (6 darts),
// $2.50 less per extra dart, never under $5.
export function payout(dartsThrown) {
  return Math.max(500, Math.min(5000, 5000 - (dartsThrown - 6) * 250));
}

// One throw at aim (u, v). Changes `s` and returns what happened:
// {u, v, pts, label, left, bust, won, turnOver, turn: [pts...]}.
export function throwDart(s, aimU, aimV, rand = Math.random) {
  // A finished turn (3 darts, a bust or a win) starts a new one.
  if (s.turn.length >= 3 || s.turnOver) {
    s.turn = [];
    s.turnOver = false;
    if (s.left === 0) {  // a new game
      s.left = START;
      s.thrown = 0;
      s.gameId = newId();
    }
    s.turnStart = s.left;
  }
  s.thrown = (s.thrown || 0) + 1;
  const u = clamp(aimU, -0.5, 0.5) + wobble(rand) * SPREAD * 2;
  const v = clamp(aimV, -0.5, 0.5) + wobble(rand) * SPREAD * 2;
  const { pts, label } = score(u, v);
  s.turn.push(pts);
  let bust = false, won = false;
  if (s.left - pts < 0 || s.left - pts === 1) {
    bust = true;
    s.left = s.turnStart;
    s.turnOver = true;
  } else {
    s.left -= pts;
    if (s.left === 0) {
      won = true;
      s.games++;
      s.turnOver = true;
    }
  }
  if (s.turn.length >= 3) s.turnOver = true;
  if (s.turnOver && !bust) s.best = Math.max(s.best, s.turn.reduce((a, b) => a + b, 0));
  return { u: round(u), v: round(v), pts, label, left: s.left, bust: bust ? 1 : 0, won: won ? 1 : 0,
    turnOver: s.turnOver ? 1 : 0, games: s.games, thrown: s.thrown };
}

const clamp = (x, lo, hi) => (x < lo ? lo : x > hi ? hi : x);
const round = (x) => Math.round(x * 10000) / 10000;
