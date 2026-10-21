// The room's rules for the shared world: what each key may hold and who may
// change it. Every world write (KV message or `set` intent) goes through
// checkWrite(); a write that breaks a rule is refused and never reaches the
// other players. (cloud/room.js calls this; docs/PROTOCOL.md §4.5 lists it.)
//
// Keys today (written by the game, bar/*.cpp):
//   g<n>   glass n: x,y,z, quaternion x,y,z,w, velocity x,y,z, state
//          (0 moving, 1 held, 2 resting). Written only by the room's own
//          physics (cloud/games/glasses.js); phones use grab/drop/throw intents.
//   jb     jukebox: song (-1 = off), server time it started (s)
//   pso    piano song: song (-1 = none), server time it starts (s)
//   ch     the lounge's chess game: board (8 numbers), turn, castling, en
//          passant, halfmoves, fullmove, last move, two players, move number.
//          Only a player at the board (claim machine.chess.w / .b) writes it.
//   a<n>   player in slot n's custom avatar (17 small numbers, bar/avatar.h
//          AvatarStyle). Only that player writes it.
//   rc     the racing rigs: lobby, picks, start time, results. Written only
//          by the room (cloud/games/race.js); phones use `race` intents.
//   t<net> a thing's state (docs/ENGINE.md E3): state, start (server s).
//          Written only by the room (the `thing` intent runs the bar's rules).
//   dk<n>  drink on counter spot n (0..7): drink type + 1, 0 = empty.
//          Serving one (non-zero) needs the bartender badge; taking one is a
//          `set` intent that expects the drink still there (first one wins).

const ROOM_HALF = 40;      // metres: anything outside isn't in the bar
const MAX_SPEED = 40;      // m/s for a thrown glass
const START_SLACK_S = 15;  // a song start time this far from the room's clock is replaced by it

const isBit = (v) => v === 0 || v === 1;
const isInt = (v, lo, hi) => Number.isInteger(v) && v >= lo && v <= hi;
const inRoom = (x, y, z) => Math.abs(x) <= ROOM_HALF && Math.abs(z) <= ROOM_HALF && y >= -2 && y <= 12;

// The claim that protects a key ('' = none).
export function claimFor(key) {
  const g = /^g(\d{1,2})$/.exec(key);
  return g ? 'glass.' + g[1] : '';
}

// → {ok: true, values} (possibly corrected) or {ok: false, why}.
// ctx: {slot, nowS, owner(claimKey) → slot or undefined, badges: [..], viaIntent}
export function checkWrite(key, values, ctx) {
  if (!Array.isArray(values) || !values.every(Number.isFinite)) return { ok: false, why: 'bad numbers' };
  const v = values;
  let m;
  if ((m = /^g(\d{1,2})$/.exec(key))) {
    if (+m[1] > 31) return { ok: false, why: 'no such glass' };
    if (v.length !== 11) return { ok: false, why: 'a glass is 11 numbers' };
    if (!inRoom(v[0], v[1], v[2])) return { ok: false, why: 'outside the bar' };
    const q = Math.hypot(v[3], v[4], v[5], v[6]);
    if (q < 0.9 || q > 1.1) return { ok: false, why: 'bad rotation' };
    if (Math.hypot(v[7], v[8], v[9]) > MAX_SPEED) return { ok: false, why: 'too fast' };
    if (!isInt(v[10], 0, 2)) return { ok: false, why: 'bad glass state' };
    // The room's physics moves every glass (cloud/games/glasses.js): phones
    // ask with grab / drop / throw intents and never write g<n> themselves.
    return { ok: false, why: 'the room moves the glasses' };
  }
  if (key === 'rc') return { ok: false, why: 'the room runs the race' };
  if (/^t\d{1,5}$/.test(key)) return { ok: false, why: 'things change through the thing intent' };
  if ((m = /^a(\d{1,3})$/.exec(key))) {  // a player's custom avatar: only they write it
    if (+m[1] !== ctx.slot) return { ok: false, why: 'that is someone else\'s avatar' };
    if (v.length !== 17 || !v.every((x) => isInt(x, 0, 63))) return { ok: false, why: 'an avatar is 17 small numbers' };
    return { ok: true, values: v };
  }
  if (key === 'ch') {
    if (v.length !== 17 || !v.every(Number.isInteger)) return { ok: false, why: 'a chess state is 17 whole numbers' };
    if (ctx.owner('machine.chess.w') !== ctx.slot && ctx.owner('machine.chess.b') !== ctx.slot) {
      return { ok: false, why: 'only the players at the chessboard move its pieces' };
    }
    return { ok: true, values: v };
  }
  if (key === 'jb' || key === 'pso') {
    if (v.length !== 2 || !isInt(v[0], -1, 999)) return { ok: false, why: 'bad song' };
    // The room's clock decides when a song starts (a phone with a wrong
    // clock can't make everyone jump).
    const start = v[0] >= 0 && Math.abs(v[1] - ctx.nowS) > START_SLACK_S ? ctx.nowS : v[1];
    return { ok: true, values: [v[0], v[0] >= 0 ? start : 0] };
  }
  if ((m = /^dk([0-7])$/.exec(key))) {
    if (v.length !== 1 || !isInt(v[0], 0, 63)) return { ok: false, why: 'bad drink' };
    if (v[0] === 0 && !ctx.viaIntent) return { ok: false, why: 'take a drink with a set intent' };
    if (v[0] !== 0 && !ctx.badges.some((b) => b === 'bartender' || b === 'owner')) return { ok: false, why: 'only bartenders serve drinks' };
    return { ok: true, values: v };
  }
  return { ok: false, why: 'unknown key' };
}
