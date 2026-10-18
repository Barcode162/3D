# Bar3D room protocol (v1)

How a phone talks to its room on Cloudflare (`cloud/room.js`, one Durable Object per
room). Written in restructure step 3.1 from `docs/RESTRUCTURE.md` §2.5, which explains
*why*; this file is the exact *what*. Two codecs implement it and must agree:
`cloud/proto.js` (server, also used by the Node test tools) and `engine/netproto.*`
(the game). Both are tested on the same golden vectors in `proto/vectors.json`:
`node tools/proto_test.mjs` and the native `proto_test` (in `tools/native/run.sh`).

**Changing anything here:** change both codecs and add or update vectors in the same
commit. Hex in `proto/vectors.json` is the contract; never regenerate it blindly from one
codec, check the bytes by hand against the tables below.

## 1. Connection
- URL: `wss://<worker host>/room/<name>`, `<name>` = `[a-z0-9-]{1,32}`. Only `main`
  keeps storage; other rooms erase themselves when their last socket closes.
- The browser sends its `Origin`; the Worker allows exact origins only (§7).
- The client sends **HELLO** first. A socket that sends nothing else within 5 s is closed.
- Server answers with **WELCOME** (or closes with a code from §5).
- One WebSocket carries everything. WebSocket is ordered and reliable; "unreliable"
  data (STATE, PROP) is *latest wins*: a newer sample replaces an older one.

## 2. Frames
Two kinds of frame, told apart by the WebSocket frame type:
- **Binary** frames (high rate): byte 0 is the message type (§3).
- **Text** frames (low rate): character 0 is the message type letter, the rest is one
  JSON object: `P{"c":1234.5}`. Receivers peek one character to route (the page's
  JavaScript only routes `G` signal and `F` profile itself; everything else goes to the
  game as-is).

Rules for every frame:
- Little-endian, no padding. A binary frame must be exactly as long as its contents say:
  short, long (trailing bytes), unknown type, out-of-range counts or non-finite floats →
  the frame is rejected (dropped; the server counts it as abuse, §7).
- Text: valid JSON object after the letter, every number finite, every field in range
  (tables in §4). Unknown *keys* are ignored (room for optional additions). An unknown
  *type letter* is rejected by the codec; receivers drop it silently.
- Size: client → server ≤ 4096 bytes (SIGNAL ≤ 16384). Server → client ≤ 65536.

Rounding: every quantization is `floor(v * scale + 0.5)` (not `Math.round`/`lround`,
which disagree on negative halves), then clamped to the field's range. Encoders reject
NaN or infinity instead of sending it.

## 3. Binary messages

### 3.1 Avatar block (16 bytes)
| Offset | Type | Field | Encoding |
|---|---|---|---|
| 0 | i16 ×3 | `x, y, z` | eye position, centimetres (±327.67 m) |
| 6 | u16 | `yaw` | radians wrapped to 0..2π, × 65536 / 2π (65536 → 0) |
| 8 | i16 | `pitch` | radians clamped to ±π/2, × 10000 |
| 10 | u8 | `anim` | pose/emote id (0 = none; app table) |
| 11 | u16 | `animTick` | server tick when `anim` started, low 16 bits |
| 13 | u8 | `held` | held item (0 = none; drink type + 1) |
| 14 | u8 | `look` | avatar look index |
| 15 | u8 | `flags` | bit 0 talking, bit 1 away, bit 2 bartender on shift, bit 3 on the stage mic; bits 4-7 must be 0 |

Decoded: `x,y,z` = value / 100; `yaw` = value × 2π / 65536 (0 ≤ yaw < 2π); `pitch` =
value / 10000.

### 3.2 Activity data (TLV)
After an avatar block: `u8 len` (0..48) then `len` bytes made of entries
`{u8 kind, u8 n, n bytes}`. The entries must fill `len` exactly. The server never reads
inside an entry; it relays the bytes. Kinds are defined by the game (one per activity,
e.g. race steering, darts aim) and listed here when they are added.

| Kind | Meaning | Added |
|---|---|---|
| (none yet) | | |

### 3.3 Prop sample (18 bytes)
| Offset | Type | Field | Encoding |
|---|---|---|---|
| 0 | u16 | `id` | prop net id (hash of its layout id, §2.3 of RESTRUCTURE) |
| 2 | i16 ×3 | `pos` | millimetres (±32.767 m) |
| 8 | u32 | `quat` | smallest three: bits 30-31 = index of the largest component; the other three in index order, 10 bits each (bits 20-29, 10-19, 0-9), each `(c / 0.70710678 × 0.5 + 0.5) × 1023`; the largest is made positive before packing (q and −q are the same rotation) |
| 12 | i16 ×3 | `vel` | centimetres per second |

Decoding the quaternion: each small `c = (u / 1023 − 0.5) × 2 × 0.70710678`; if
`c0² + c1² + c2² > 1` the frame is rejected; the largest = `sqrt(1 − sum)`. Encoders
normalize the input and reject a zero or non-finite quaternion.

### 3.4 Types
| Byte | Name | Direction | Layout |
|---|---|---|---|
| `0x01` | STATE | client → server | `u8 type, u16 seq, avatar block, activity` (20..68 bytes) |
| `0x02` | PROP | client → server (lease owner only) | `u8 type, u8 count (1..8), count × prop sample` |
| `0x81` | TICK | server → client | `u8 type, u32 tick, f64 serverMs, u8 n (0..16), n × {u8 slot (0..15, no repeats), avatar block, activity}, u8 m (0..32), m × prop sample` |

`seq` counts the sender's STATE messages (wraps at 65536) so the server can drop an
older one that overtook a newer one after a reconnect. TICK carries only the players and
props that changed since the previous TICK.

## 4. Text messages
`?` = optional. Strings are UTF-8; lengths are in bytes. `slot` is always 0..15.

### 4.1 Client → server
| Letter | Name | Body |
|---|---|---|
| `H` | HELLO | `{proto: int 1..255, build: str ≤16, name: str 1..24, look: int 0..255, resume?: str (32 hex chars), ticket?: str ≤2048}` |
| `I` | INTENT | `{i: int 0..2147483647 (request id), k: str 1..32 (kind), …}` extra keys are the intent's arguments |
| `E` | EVENTS | `{e: [[type: str 1..16, args: [number ≤16]] ≤32]}` |
| `K` | KV (legacy world, until 5.11) | `{w: {key: [number ≤20]}}`, ≤ 32 keys, key `[A-Za-z0-9_]{1,16}` |
| `G` | SIGNAL | `{to: slot, data: any}` (voice chat handshake, ≤ 16 KB frame) |
| `P` | PING | `{c: number}` (client clock, ms) |
| `R` | REAUTH | `{ticket: str 1..2048}` |
| `A` | AWAY | `{on: 0 or 1}` |

### 4.2 Server → client
| Letter | Name | Body |
|---|---|---|
| `W` | WELCOME | `{proto: int, slot, resume: str (32 hex), serverMs: number, tick: int ≥0, tickHz: int 1..60, room: str ≤32, roster: [player ≤16], claims: {key: slot}, sessions: {}, kv?: [kv entry]}` |
| `D` | DELTA | `{d: [[seq: int, field: str ≤48, value: any, by: slot or -1] ≤64]}` |
| `R` | RESULT | `{r: int (request id), ok: 0 or 1, seq?: int, why?: str ≤64, by?: slot}` |
| `E` | EVENTS | `{e: [[tick: int, slot, seq: int, type: str 1..16, args: [number ≤16]] ≤64]}` |
| `K` | KV (legacy world) | `{w: [kv entry ≤64]}`, also echoed to the writer |
| `J` | JOIN | player |
| `L` | LEAVE | `{slot, why?: str ≤64}` |
| `F` | PROFILE | `{slot, name: str 1..24, acct?: str ≤32, av?: int, badges?: [str ≤16], role?: str ≤16}` |
| `G` | SIGNAL | `{from: slot, data: any}` |
| `P` | PONG | `{c: number (echo), s: number (server ms)}` |

- *player* = `{slot, name: str 1..24, look: int 0..255, acct?: str ≤32, av?: int, away?: 0 or 1}`
- *kv entry* = `[seq: int, key: str, by: slot or -1, values: [number ≤20]]`
- `by: -1` means the server itself (layout defaults, timers).

## 5. Close codes
| Code | Meaning | Client does |
|---|---|---|
| 1000 / 1001 | normal close / going away | reconnect with backoff if it wasn't us |
| 4001 | room full (reason text says so) | shows "Room full"; **no auto-retry**, a Retry button |
| 4002 | protocol mismatch (new version) | shows "New version"; **no auto-retry**; polls `version.txt` until the build changes, then reloads once |
| 4003 | replaced (the same resume token connected again) | stops; this tab is a duplicate |
| 4004 | idle/ghost (no traffic 15 s) or AFK (10 min) | shows "Away, tap to rejoin"; no auto-retry |
| 4005 | rate limit / bad frames | shows "Disconnected (too many messages)"; **no auto-retry** |
| other / no close frame | network lost | reconnect: 250 ms, 1, 2, 4, then every 8 s, sending `resume` |

## 6. Rates, clock, reconnect
- STATE 15-20 Hz only while something changes; nothing while idle (the auto-response
  keepalive `ping` text, not a protocol message, answers every 5 s without waking the room).
  At 70% of a daily free quota the server asks for 10 Hz (a DELTA field `net.stateHz`).
- TICK at 20 Hz (`tickHz`) only while something is dirty; stops after 2 s idle.
- Events are batched per net tick; receivers play them at `serverMs + playout` and drop
  events older than 1 s or with a repeated per-sender `seq`.
- Clock: 5 PINGs at connect (keep the lowest round trip), then one every 10 s;
  offset = `s + rtt/2 − clientNow`.
- Reconnect: send HELLO with the `resume` token from WELCOME → same slot and claims for
  30 s after a drop; re-send pending intents with the same `i` (the server answers each
  `slot + i` once).

## 7. Limits (enforced by the server)
- Origins: `https://barcode162.github.io`, `https://joshuavelasco.com`,
  `http://localhost:<any>`, `http://127.0.0.1:<any>`; requests without `Origin` (Node test
  tools) are allowed. Never a suffix match.
- Per IP ≤ 10 WebSocket upgrades per minute. 16 players per room.
- Per socket: 25 messages/s sustained, burst 60, 64 KB/s; intents ≤ 10/s. Over → 4005.

## 8. Versions
- `proto` in HELLO. The server accepts versions N−1..N (one deploy of overlap) and closes
  anything else with 4002. Current: **N = 1**.
- Same version may add: optional keys, new TLV kinds, new DELTA fields, new message types
  that old receivers can drop. Anything else (new required key, changed layout or
  meaning) bumps N.

| Proto | Step | Change |
|---|---|---|
| 1 | 3.1 | first version |

## 9. Room storage `schemaVersion`
The room keeps a `schemaVersion` key; on wake-up a mismatch wipes storage back to layout
defaults (logged, counted on `/health`). Bump it whenever stored keys change shape.

| schemaVersion | Step | Stored keys |
|---|---|---|
| 1 | 3.2a | `schemaVersion`; `kv:<key>` legacy world values (3.2b) |
