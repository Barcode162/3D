# Bar3D restructure plan (written 2026-10-16, build `64d6ef6`)

Written from audits of every system. **This is the top priority until it is done**
(see `PROGRESS.md` → Next up). Racing feel tuning from the user's feedback can
still happen in between, as small steps.
**Reviewed 2026-10-17** (see the last section, "Plan review"): steps split to one-session
size, the execution order fixed (top of §4), wrong references corrected, limits and
security notes added.

## In plain words (for the user)
The bar started as a small test and grew into a big game, so the parts got
tangled: one giant file decides everything, phones argue with each other about
who sits where, and a lot of heavy work happens all at once when the bar loads.
This plan untangles it in small steps. The game keeps working after every step.

What changes for you, in order:
1. **We get eyes on your phone.** A small performance overlay and a "Send report"
   button. You send a short code, Claude reads exactly what lagged on your iPhone.
2. **The start-up lag goes away.** Less work while loading, heavy work spread
   out, no freeze after "Tap to enter", no freeze when a piano song starts.
3. **Smoother basics.** Walking and turning feel smoother, glasses stop wobbling,
   the race car stops juddering.
4. **Move to Cloudflare.** Other players move smoothly with less delay, and if your
   connection drops you come back in the same place. *Your Cloudflare account and
   the two GitHub secrets are already set up, so nothing is needed from you. Claude
   may build the server part earlier, in the background; you won't see it.*
5. **The insides get cleaned up** so new games are easy to add and bugs have
   fewer places to hide.
6. **The server decides shared things.** One person per stool, no double drinks,
   music and piano in sync for both of you, safer logins.
7. **The bar draws faster** (fewer draw calls, cheaper mirror, smaller textures),
   so it stays at 60 fps with friends around.
8. **Racing together**: two players and bots, races shown on the monitors.
9. **One address, always the newest build.**

**What you might notice along the way** (one-off and expected):
- At the move to Cloudflare (step 3.4d) the glasses go back to their normal spots once.
- When the game's address changes you log in again: at 3.5 if you used
  joshuavelasco.com/bar, and at 8.1 (new address) for everyone. The browser keeps
  settings, name and profile per address, so set those again once too.
- After some updates (4.2, 4.3b and several Phase 5 steps) the game says "New version,
  tap to reload". Old and new builds can't play together, so you and your brother
  should both reload.
- Some steps change nothing you can see; the "How to check it worked" list says so.

### Summary table
| Phase | What it fixes | What you'll notice on the phone | Needs from you |
|---|---|---|---|
| 0. Eyes on the phone | We can't see what lags; errors cover the screen | Perf overlay in Settings, "Send report" gives a code; red error badge instead of the log popping open | Send reports when asked (Low Power Mode off) |
| 1. Start-up lag | Freeze while loading and right after entering; piano freeze | Faster, smoother loading; no hitch after "Tap to enter"; mirror and 4K Mania cheaper | Look at a few spots after each step |
| 2. Engine plumbing | Hidden per-frame waste; jiggling glass; race judder | Smoother walking and rotating; still glasses; smooth race car | — |
| 3. Cloudflare room | Laggy, out-of-sync multiplayer; ghosts | Others move smoother (about 0.2 s behind instead of 0.3-0.6 s); reconnects keep your spot | — (account + secrets done; maybe open Workers & Pages once, see 3.3) |
| 4. Clean core | Tangled code, games interfering | Nothing new (everything works as before; a few small bugs fixed) | — |
| 5. Server decides | Double seats, drinks, glasses, jukebox restarts, stuck piano notes | Only one person gets a stool/drink/glass; music and piano in sync for both | **Q2: IONOS MySQL database** (for logins); Q6: TURN key (voice on cellular) |
| 6. Faster drawing | Too many draw calls, heavy mirror, huge textures | Steady 60 fps at the mirror and with friends | Q7: OK for one texture tool |
| 7. Racing together | Races are private | Race your brother + bots; races on the rig monitors | Test with your brother |
| 8. Hosting | Stale versions, two copies of the game | Always the newest version, one address | Q5 |

---

## How to use this plan (for Claude sessions)
- **One step per session** (two if both are small). Read §1, the step, and the
  §2 sections the step links. Don't re-read the whole file.
- **Order:** follow "Execution order" at the top of §4, not the step numbers alone (the
  2026-10-17 review split and moved some steps).
- Line numbers below are from build `64d6ef6` and drift; search by function name. The
  review corrected the references it found wrong.
- **Standard check (STD)** for every step:
  `source /opt/emsdk/emsdk_env.sh && cmake --build build && NODE_PATH=$(npm root -g) node tools/smoke.mjs`
  passes, and you looked at `build/smoke.png`, `build/smoke-walk.png`, `build/smoke-down.png`.
  From step 0.4 on, CI runs the same tests before publishing.
- When a step is done: tick its boxes in §3.2 (items tagged with the step number),
  write `Done in <sha>` under the step heading, update `PROGRESS.md`, record any new
  decision or gotcha in `docs/DECISIONS.md`, update `CLAUDE.md` (Layout / Build and test)
  if you added a folder, tool or test command, commit, push, confirm the deploy
  (`NODE_PATH=$(npm root -g) node tools/check_live.mjs`), and give the user the
  step's **Phone** list with the build code.
- If a step turns out bigger than one session, split it **in this file** first.
- **Visual check (VIS)** for steps that must "look the same": before editing, build the
  current HEAD and capture `build/before/` with `?still` at the standard spots (plus the
  step's test-param shots); after the change capture `build/after/` the same way and run
  `node tools/imgdiff.mjs build/before build/after` → ≤ 1% differing pixels unless the step
  says otherwise. Capture with `OUT=build/before node tools/shots.mjs spots` (all spots,
  `?still`). Standard spots (`tools/lib/spots.mjs`): door `0.4,1.65,5.2,0,0`; mirror
  `6.0,1.65,3.6,90,0`; bartop `-2.5,1.65,-2.6,0,-20`; backbar `-1.0,1.65,-3.2,0,8`; piano
  `-3.6,1.65,3.2,-115,-10`. Two runs of the same build differ by 0.000% (checked in 0.4).
- **Gates** (stop and do something else if not met):
  **G1 met (2026-10-17):** the user created the Cloudflare account and added the GitHub
  secrets `CLOUDFLARE_API_TOKEN` + `CLOUDFLARE_ACCOUNT_ID`. Sessions can't read secrets
  (`gh secret list` returns 403 through the proxy; `gh run list` and `gh run view <id> --log`
  work), so "confirmed" means the `cloudflare-check` CI step (added in 0.4) printed
  "secrets set", a working `wrangler whoami` and an existing workers.dev subdomain.
  **G2** IONOS MySQL secrets exist (5.1a; confirmed the same way, by a CI step that prints
  only set/missing). **G3** user approved `astcenc` (6.8).
  **G4** a baseline phone report exists (after 0.3; ask again after each phase).
  Never spend effort improving the PHP relay: it is retired in 3.5.
- **Admin actions** (D1 create/migrate, `wrangler secret put`, Pages project, R2 bucket, the
  account migration) need the secrets, which exist only in GitHub Actions. Run them through
  `.github/workflows/admin.yml` (`workflow_dispatch`, added in 3.3):
  `gh workflow run admin.yml -f action=<name>`, then read the result with `gh run view <id> --log`.
  If the proxy blocks `gh workflow run`, commit a marker file `admin/request.txt` naming the
  action; a push-triggered step runs it and deletes the file.

---

## 1. Goals and hard rules

### 1.1 Goals
1. **60 fps on iPhone 15 Pro Max** (iOS 26 Safari, Low Power Mode off) everywhere in
   the bar, with 8 players, at the full mirror, in every mini game. Correct (just
   slower) at 30 fps in Low Power Mode.
2. **No hitches**: no frame over 50 ms after "Tap to enter"; during loading no frame
   over 100 ms (the bar keeps moving).
3. **Zero sync issues**: every player sees the same seats, glasses, drinks, music,
   games and lights, and they agree after any reconnect.
4. **One source of truth** for every piece of state (table in §2.3).
5. **Far fewer error paths**: no silent fallbacks, every failure visible and reported.
6. **Keep the from-scratch rule**: no outside engines or runtime libraries. Offline
   tools (Blender, wrangler, Playwright, astcenc once approved) are tools, not runtime.

### 1.2 Frame budget (16.67 ms at 60 Hz)
Main-thread CPU total **≤ 10 ms** (the rest is Safari, GC and compositor slack).
| Phase (see §2.1) | CPU budget | Notes |
|---|---|---|
| Begin frame + bridge in + input poll | 0.3 ms | 1 JS→wasm crossing |
| Net poll + apply inbound | 0.5 ms | 0 JSON parses when nothing arrived |
| Simulation ticks (player, activities, interaction, physics) | 1.5 ms | physics ≤ 0.5 typical; measured 0.05-0.3 now |
| Race sim (only while racing) | 1.0 ms | measured ≈ 0.08 ms for 8 cars |
| Net out | 0.2 ms | only on the 20 Hz net tick |
| Presentation (remote interpolation, posing visible avatars, HUD on change, audio commands) | 1.5 ms | |
| Render build + upload | 0.5 ms | ≤ 3 WriteBuffer calls |
| Encode + submit | 2.0 ms | ≤ 150 draws main + ≤ 60 mirror |
| Bridge flush (DOM batch, net frames, audio commands) | 0.3 ms | 1 wasm→JS crossing |
| Background jobs | ≤ 2 ms in world, ≤ 8 ms loading | deadline-driven |

GPU **≤ 11 ms**: main pass ≤ 7, mirror ≤ 3 (half resolution), UI ≤ 0.5. Race full
screen ≤ 9. Spectator monitors ≤ 0.8.
Counts: ≤ 150 draws main / ≤ 60 mirror / ≤ 100 race; ≤ 350k visible triangles main,
≤ 150k mirror; ≤ 4 dynamic lights per pixel; resident textures ≤ 128 MB interim,
≤ 64 MB with ASTC; render targets ≤ 25 MB; JS↔wasm crossings ≤ 4 per frame.
Loading: "Tap to enter" ≤ 6 s on Wi-Fi first visit, ≤ 3 s cached.
Network: remote motion delay ≤ 200 ms, claim round trip ≤ 150 ms, event playout ≈ 100 ms.

### 1.3 Sync invariants (what "zero sync issues" means)
- **I-1 One writer per fact** (§2.3). A client never writes another player's fact.
- **I-2 The server orders every shared change** with a sequence number and sends it to
  everyone, **the writer included**. Optimistic local changes are replaced by the
  server's answer.
- **I-3 Contested things are claimed** from the server (seats, machines, glasses,
  drinks), never inferred from other players' state.
- **I-4 One clock**: server time. Every replicated sample and event carries server
  time; everything remote plays back on the same delayed timeline.
- **I-5 Momentary things are events** (with a time-to-live) on one ordered path;
  lasting things are state.
- **I-6 One backend per deployment.** Protocol mismatch → "New version, tap to reload".
- **I-7 Identity** = server slot (+ account id). Never a client-chosen id, never a
  vector index (stable entity ids instead).
- **I-8 The server detects ghosts** and frees what they held.
- **I-9 Join/resync = full snapshot + deltas.** Nobody depends on having seen history.
- **I-10 Validate every incoming value** (finite, in range, inside the room) on server
  and client.

### 1.4 Error rules
- **E-1 No silent fallbacks.** Every degraded mode shows on the HUD status and in the log.
- **E-2** Every fetch has a timeout and a retry policy; failures show on the loading
  screen or as a toast.
- **E-3 Fatal** (abort, device lost, startup failed) → overlay "Something broke - tap to
  rejoin" + automatic report.
- **E-4** Logs have levels and rate limits; only fatal errors open the log panel.
- **E-5** URL params that change servers work only for localhost targets or exact
  allowlisted hosts (never a suffix match such as `*.workers.dev`: anyone can create one).
  Tokens and passwords go only to same-origin or allowlisted hosts.
- **E-6** Every system has `reset()`; room switch and reconnect call it.
- **E-7** No `EM_ASM`/`EM_JS` in `app/`. Only `engine/bridge`, `audio`, `net`, `ui`,
  `gpu*`, `fetch` talk to JS.

---

## 2. Target architecture

### 2.1 Frame and tick model
- **rAF** via `emscripten_request_animation_frame_loop` (gives the rAF timestamp).
  One `FrameTime {rafMs, dt (clamped 0.25 s), frameIndex, serverNowMs, audioNow}` is
  built at frame start and passed everywhere. No other `emscripten_get_now()` calls
  in app code.
- **Simulation tick: fixed 60 Hz** (`kSimHz`), accumulator, at most 4 catch-up ticks
  per frame (more is dropped locally; networked sims re-derive their tick from server
  time). Race physics = 4 substeps of each 60 Hz tick (240 Hz).
- **Net tick: 20 Hz** (every 3rd sim tick). Client sends state + batched events on it.
  The server broadcasts one batched TICK at 20 Hz while anything changed.
- **Render** every rAF. Local bodies drawn at `lerp(prev, cur, alpha)`,
  `alpha = accumulator / kStep`. Remote things drawn at `renderServerTime = serverNow - interpDelay`.
- At 30 Hz (Low Power Mode) each frame runs 2 ticks: still correct.
- Hidden tab: rAF stops; on return, catch-up is capped and the client resyncs (§2.5.8).

**Exact order (one table in `app/game.cpp`, each row a system call timed by the profiler):**
| # | Phase | Does | Reads | Writes |
|---|---|---|---|---|
| 0 | BeginFrame | FrameTime; profiler begin; `bridge.readInbox()` (size, visibility, focus, safe area, voice levels, UI commands); `Input::poll()` → immutable `InputFrame` | mailbox | FrameTime, InputFrame |
| 1 | NetApply | `net.poll()` decodes inbound ring; applies in order: WELCOME/snapshot → reducers (RoomState, claims, props, sessions) → profiles → events into `EventQueue` (scheduled by server time) → intent results to ActivityManager | inbound | RoomState, RemotePlayers buffers, EventQueue |
| 2 | Commands | UI commands → typed requests to owners (ActivityManager, Avatar, games) | UI cmds | requests |
| 3 | SimTicks ×N | per tick: LocalPlayer (move, collide, floor) → ActivityManager.tick (active activity: rhythm judge, darts flight, race substeps…) → Interaction (focus; taps consumed on the first tick) → PhysicsWorld.step → PropSync.tick (owner samples). Keeps prev/cur state | InputFrame, RoomState | local sim state |
| 4 | NetOut | on the net tick: STATE (avatar block + activity data) + batched events; intents go out the same frame they are made | local state | outbound ring |
| 5 | Present | RemotePlayers.present(renderServerTime); remote activity visuals; due events → audio/visual; pose visible avatars only; prop visuals; HUD widgets update on change; audio listener/emitters → command buffer | everything | RenderFrame, HUD, audio cmds |
| 6 | Render | upload RenderFrame (one phase) → acquire surface texture late → encode mirror, main, UI → submit | RenderFrame | GPU |
| 7 | BridgeFlush | one call out: net frames, DOM batch (status, stick, log lines, UI state if changed), audio commands | outbox | JS |
| 8 | Jobs | run jobs until `deadline = frameStart + 14 ms - measured`, (loading: +12 ms) | job queue | assets |
| 9 | Profiler end | record phase times, draws, tris, heap, hitches | | profiler ring |

Rule: **apply inbound first, then simulate, then send, then present.** No system calls
another system's internals; cross-system effects go through the event bus (§2.2).

### 2.2 Module boundaries (engine vs app)
`main.cpp` shrinks to about 100 lines: create `Engine` services, create `Game`, start
the loop. The `g_app` god object disappears.

**engine/** (reusable, never includes `app/`):
| Module | Files (existing → new) | Responsibility |
|---|---|---|
| core | `frame_time.h`, `profiler.*`, `jobs.*` (from `gpu_upload.*` queue), `log.*` (ring buffer), `bridge.*` | frame time, profiling, jobs with priorities/deadline, logging, the one JS mailbox |
| gpu | `gpu.*`, `pipeline_cache.*`, `upload_ring.*`, `texture_store.*`, `culling.*`, `render_frame.h` | device/surface/resize/loss, async pipelines, per-frame uploads, textures with budgets, frustum culling |
| assets | `fetch.*`, `gltf.*` (dev/user GLBs only), `model.*`, `pack.*` (baked pack loader) | streaming with retry, sliced parsing |
| input | `input.*`, `gamepad.*` | raw events (with event timestamps) → `InputFrame` |
| audio | `audio.*` → `AudioMixer` (buses, emitters, voice pool, command buffer), `clock.*` (server↔audio time, latency + calibration), `synth.*`, `midi.*` | |
| net | `net.*` v2 (typed messages, slots, intents, events, server time), `netproto.*` (binary codec) | transport is JS (`web/net.js`), logic is C++ |
| sim | `physics.*`, `vehicle.*`, `math.h`, `skeleton.*`, `skinned_mesh.*`, `camera.h`, `material.h` | |
| ui | `ui.*` (typed commands in, UI state out), `text.*`, `font.*`, `billboard.*` | |

**app/** (the bar):
| Module | Files | Responsibility |
|---|---|---|
| Game | `game.*` | owns every system, the phase table, `reset()` on room join/leave |
| World | `layout.*` (from `bar_scene.*`; later loaded from `layout.json`), `entities.*`, `props.*`, `materials.*` | immutable layout, entity registry, physics setup |
| RoomState | `room_state.*` | replicated shared facts; **written only by NetApply** |
| LocalPlayer | `local_player.*` | camera, controller, eye height, `teleport()`, held item |
| RemotePlayers | `players.*` | snapshot buffers on server time, interpolation, profiles cache |
| Activities | `activity.*`, `activities/*.cpp` (seat, piano, rhythm, darts, bartend, race) | §2.4 |
| Interaction | `interaction.*` | one target query + handler table |
| PropSync | `prop_sync.*` (replaces glass part of `shared_world.*`) | leases, streams, interpolation |
| Games | `piano.*`, `darts.*`, `rhythm*.*`, `bartender.*`, `jukebox.*`, `race/*` | game rules only; no networking or rendering calls of their own |
| Presentation | `presentation.*`, `hud.*`, `renderer.*`, `avatar.*` | builds RenderFrame and HUD; only `renderer` touches wgpu |
| Session | `loader.*`, `test_hooks.*`, settings | loading state machine, URL test params parsed once |

`Services` struct (references to gpu, input, mixer, clock, net, ui, jobs, profiler) is
passed to systems explicitly. A tiny **event bus** (fixed-size typed queue, drained at
phase boundaries, no re-entrancy) carries `ActivityEntered/Exited`, `ClaimChanged`,
`ToastRequested`, `SfxRequested{soundId, emitterId}`, `FocusChanged`, `SongStarted`,
`PropOwnerChanged`, `RoomFieldChanged`.

### 2.3 Entities, identity and ownership
- Every seat, interactable, prop, mirror, machine and rig gets a **stable string id**
  in the layout (`seat.bar.3`, `prop.pint.2`, `rig.left`, `machine.rhythm`), hashed to
  a 16-bit **net id** (collision check at startup). `EntityRegistry` maps id ↔ render
  handle ↔ `BodyId` ↔ net key. No vector index ever goes on the wire.
- Components: Transform, Render (cluster/instance), Body, Seat, Interactable,
  Prop (archetype: collision proxy, mass, material, model), Claimable (claim key),
  NetSync policy (`Local | Event | RoomField | Claim | OwnedStream`).
- Layout (immutable), RoomState (server-replicated), LocalState (camera, settings,
  focus, HUD) are separate. Nothing in the layout changes at runtime.

**State ownership (the one source of truth for each fact):**
| Fact | Owner (only writer) | Transport | Rate | Others |
|---|---|---|---|---|
| Your position, yaw, pitch, look, held drink, talking flag | you (LocalPlayer) | STATE (binary) | 15-20 Hz while changing | interpolate on server time |
| Your pose/emote/sip/bartend motion (cosmetic) | you | STATE: anim id + start tick | on change | play from start tick |
| Occupancy: seats, rigs, rhythm cabinet, dartboard, piano bench, bar station (up to 2), stage mic | **server claims** | intent → RESULT; DELTA | on change | read claims, never infer |
| Lights, fire | server RoomState | intent → DELTA | on change | render RoomState |
| Mirror mode | **local setting** (recommended, Q3) | none | — | — |
| Jukebox queue, song, start time | server | intents → DELTA; server alarm advances | on change | stream follower |
| Piano song (id, start, duration, owner) | server session | intent → DELTA | on change | schedule on audio clock |
| Live piano notes, sfx, dart throws, rhythm judgements, bartend steps | the acting player | EVENT (server stamps tick + seq) | batched per net tick | play at ts + playout, drop if > 1 s old |
| Drinks on the counter (8) | server | serve/take intents | on change | |
| Glass at rest | server RoomState (persisted) | DELTA | on change | |
| Moving/held glass | lease owner (granted by server) | PROP samples relayed in TICK | 15 Hz | interpolate; kinematic copy |
| Darts game (players, scores, turn) | server session | throw events; server validates | per throw | everyone runs the same `score()` |
| Rhythm session (song, difficulty, start) | server session; chart is static content | judgement events | | real chart on cabinet |
| Race session (roster, goAt, laps, results) | server | intents/reports | | |
| Own race car | driver | race snapshots | 30 Hz in 2-tick batches | proxies |
| Bots | one authority (host phone; later the server) | race snapshots | 30 Hz | proxies |
| Name, picture rev, badges, role | IONOS (signed ticket) | PROFILE from server | on change | cache |
| Camera, focus, HUD, settings, mirror mode | local | none | | |
| Voice audio | WebRTC peer-to-peer | signaling over the room socket | | |

### 2.4 Activities: one framework for seats and machines
```cpp
struct ActivityDesc {
  ActivityKind kind;                 // kSit, kDrive, kPiano, kRhythm, kDarts, kBartend
  std::array<ClaimKey, 2> claims;    // e.g. "seat.bar.3", "rig.left", "machine.piano"
  Movement movement;                 // Free | Locked | Seated(seatId) | Anchored(anchorId)
  InputMode input;                   // World | Capture (raw presses) | UiOnly
  ViewMode view;                     // World | WorldDimmed | Overlay (no 3D) | Fullscreen (own renderer)
  LeaveRule leave;                   // WalkAway(m) | Button | StandUp
  SpectatorChannel spectate;         // none | darts | rhythm | race | piano
};
class Activity {
  virtual ActivityDesc desc() const = 0;
  virtual void enter(Ctx&) = 0;  virtual void exit(Ctx&, ExitReason) = 0;
  virtual void tick(Ctx&, const InputFrame&, float dt) {}
  virtual void present(Ctx&, Hud&) {}
  virtual void writeNetData(Writer&) const {}   // owner → spectators (in STATE)
  virtual void onEvent(Ctx&, const NetEvent&) {}
};
```
- **ActivityManager** holds exactly one active activity. States: `Idle → Requesting →
  Active → Leaving → Idle`. Entering sends claim intents (§2.5); while `Requesting` the
  avatar starts the sit-down/approach animation (0.25 s, hides the 40-120 ms round
  trip). Grant → `enter()`. Deny → reverse + toast "Taken by Bob". Server revoke
  (lease lost, kicked) → `exit(kRevoked)`.
- Capture, movement lock, view mode, label suppression, HUD and net occupancy are
  **derived** from the active desc. Adding a game = one class + one server validator.
- A transition table decides what may start while something else runs (emote: allowed
  while seated, not during rhythm; "make drink": only on shift; etc.).
- Cosmetic **pose layer** (emotes, sip, bartend steps) is separate from occupancy.
- **Server side:** `GameSession {key, kind, owners[], leaseUntil, seq, data}` with one
  validator module per kind in `cloud/sessions/*.js`. Owner input = events stamped by
  the server. Spectators replay session data + events at `serverNow - interpDelay`
  through a `RemoteActivityView` (cabinet screen, dartboard, monitors, piano keys).
- Per game:
  | Game | Claims | Session data (server) | Events | Spectators see |
  |---|---|---|---|---|
  | Seats/sofas | seat id | — | — | avatar seated (from claim) |
  | Piano | `machine.piano` | song {id, start, duration} | notes, pedal | keys, hands from their notes |
  | Darts | `machine.darts` | game type, players, scores, turn | throw {u, v} | flight + thunk + score |
  | 4K Mania | `machine.rhythm` | song, difficulty, start | judgements | real chart + bursts |
  | Bartender | `station.bar` (2 slots) | counter drinks[8] | steps | animation + sounds |
  | Race | `rig.left` / `rig.right` | roster, goAt, laps, results | contacts, laps | monitors (7.5) |
  | Jukebox | none (room machine) | queue, current, start | — | music |

### 2.5 Networking model (Cloudflare Durable Object rooms)

**2.5.1 Authority.** The Room DO is the authority for: slots and identity, claims and
leases, RoomState fields, game sessions, jukebox timing, event ordering and stamping,
ghost detection. Clients predict: their own movement (fully theirs, server sanity-checks
room bounds/speed), their own race car, a held glass (lease), optimistic toggles with
rollback, the sit/approach animation while a claim is pending.

**2.5.2 Messages (protocol v1; full spec written in step 3.1 as `docs/PROTOCOL.md`).**
High-rate messages are **binary**; low-rate ones are **JSON text** (cheap to debug,
parsed in C++ only when they arrive).
- Client → server: `HELLO {proto, build, resume?, ticket?, name, look}` · `STATE`
  (binary: avatar block ≈ 16 B: pos int16 cm ×3, yaw u16, pitch i16, anim id u8 + start
  tick u16, held u8, look u8, flags u8 (talking, away), optional activity TLV) ·
  `INTENT {i: reqId, k: kind, …}` · `EVENTS [[type, args]…]` (batched per net tick) ·
  `PROP` (binary, lease owner only) · `SIGNAL {to, data}` (voice) · `PING {c}` ·
  `REAUTH {ticket}` · `AWAY {on}`.
- Server → client: `WELCOME {slot, resume, serverMs, tick, tickHz, roster, room, claims,
  sessions}` · `TICK` (binary: tick u32, serverMs, n × {slot, avatar block, TLV},
  m × {propId u16, pos int16 mm ×3, quat smallest-three, vel int16 cm/s ×3}) ·
  `DELTA [[seq, field, value, bySlot]…]` · `RESULT {r, ok, why?, by?, seq}` ·
  `EVENTS [[tick, slot, seq, type, args]…]` · `JOIN/LEAVE/PROFILE` · `SIGNAL {from, data}` ·
  `PONG {c, s}`.
- Close codes: `4001` room full (sent after accept, with reason), `4002` protocol
  mismatch → reload prompt, `4003` replaced (only by a valid resume token), `4004`
  idle/ghost, `4005` rate limit.
- One codec per side: `cloud/proto.js` (server) and `engine/netproto.*` (C++), both
  tested against golden vectors in `proto/vectors.json`. JS in the browser does **not**
  decode game messages; it copies frames into the wasm ring and only peeks the type
  byte for SIGNAL (voice) and PROFILE (menus/cards).

**2.5.3 Snapshot + interpolation.** Each TICK carries the server tick and time.
Remote players and props keep a ring of samples `{serverMs, block}`;
`renderServerTime = serverNow - interpDelay`, `interpDelay = clamp(2 × 50 ms + p95 jitter, 80, 250)`
(starts at 150 ms). Continuous fields interpolate (positions lerp, angles shortest
path, quats nlerp); **discrete fields (anim, held, flags) come from the sample at
render time**, so seat/pose changes line up with the body. Extrapolate up to 100 ms
when the buffer runs dry, then ease back.

**2.5.4 Events.** Envelope `{tick, slot, seq, type, args}`, one ordered path (the
room socket; no WebRTC data channel for game events). Receivers schedule anything
audible on the audio clock at `eventServerMs + playout` (≈ 100 ms, adaptive), drop
events older than 1 s, and keep per-sender seq to drop duplicates.

**2.5.5 Reliable vs "unreliable".** WebSocket is ordered and reliable. "Unreliable"
data is modelled as **latest-wins**: STATE/PROP samples overwrite older ones (server
keeps only the newest per slot/prop; the client inbound ring keeps only the newest
per player after a stall). Events have a TTL. Intents get exactly one RESULT (server
dedupes by `slot + reqId`, so re-sending after reconnect is safe).

**2.5.6 Clock sync.** WELCOME carries `serverMs`. 5 PINGs in a burst at connect,
keep the lowest-RTT sample; then 1 PING every 10 s through the DO (it must answer with
server time, so it can't be an auto-response). Offset = `serverMs + rtt/2 - clientNow`,
min-RTT filter over the last 8 samples, slewed ≤ 2 ms per second after lock (jumps
only if off by > 100 ms). `engine::Clock` maps server ↔ audio time (re-estimated each
second) and adds output latency + a user calibration offset.

**2.5.7 Claims and leases.** Table `key → {slot, sinceMs, leaseUntilMs}`. `claim`
grants if free (or expired), else `{ok:0, why:"taken", by:slot}`. Seats/machines: no
lease while the socket lives; freed on release, socket loss (after the grace period)
or kick. Props: lease renewed by the owner's PROP stream, ends on release, rest, 3 s
silence or disconnect; then the server hands a "settle" lease to the nearest connected
player (or stores the last pose as rest if nobody is there).

**2.5.8 Reconnect and lifecycle.** WELCOME gives a random 128-bit resume token. Socket
lost → client retries at 250 ms, then 1, 2, 4, max 8 s, sending the token: same slot,
claims kept, fresh WELCOME snapshot (client rebuilds remote buffers, re-sends pending
intents with the same reqIds). Server keeps the slot for a **30 s grace** (Q9) showing
the player dimmed ("reconnecting"), then frees slot + claims. Tab hidden → client sends
`AWAY {on:1}`, stops STATE, suspends audio; others see an "away" tag. Visible again →
`AWAY {on:0}` and resync (reconnect if the socket died; iOS often kills it silently).
Client treats "no TICK or PONG for 4 s while visible" as a dead socket.

**2.5.9 Ghosts and idle players.** Idle clients send an auto-response ping every 5 s
(`setWebSocketAutoResponse`), which doesn't wake a hibernated DO. A socket with no
message and no auto-response keepalive (`getWebSocketAutoResponseTimestamp`) for 15 s is
a ghost: close 4004. **No 5 s alarm:** every alarm wakes the DO and counts as a DO
request, and a DO stays in memory about 10 s after work, so a 5 s alarm would keep it
awake whenever anyone is connected (one desktop tab left open overnight ≈ 5,760 alarm
requests and ≈ 3,600 GB-s, 28% of the daily 13,000, per 8 h). Instead: while the DO is
awake anyway, check for ghosts inside message handling; when every socket is idle, set one
long deadline (≥ 60 s) in the alarm scheduler (§2.5.11) that checks the auto-response
timestamps. **AFK rule:** no STATE change, intent or event for 10 min → close 4004
"away, tap to rejoin" (the client shows that button and doesn't reconnect by itself).
Sockets without HELLO are closed after 5 s (a scheduler deadline).

**2.5.10 Limits and abuse.** Origin allowlist of exact origins
(`https://barcode162.github.io`, `https://joshuavelasco.com`, the Pages host after 8.1,
`http://localhost:*` / `http://127.0.0.1:*`); never a suffix match. Requests without an
Origin header (Node test tools) are allowed: the check only stops other websites' pages,
it is not authentication. Per-IP connect limit ≤ 10 upgrades/min (Workers rate-limit
binding if the free plan has it, else a counter in the DO). Per socket token bucket:
25 msgs/s sustained, burst 60, 64 KB/s; frames ≤ 4 KB (SIGNAL ≤ 16 KB); intents ≤ 10/s.
Abuse → close 4005. Every value validated (`cleanNumbers` style: finite, in range, in room
bounds, quaternion normalized). Server permissions come from the verified ticket
(bartender badge, owner role). Only room `main` is persisted; any other `/room/<name>`
calls `deleteAll()` when its last socket leaves, so random room names leave nothing
behind. `/health` is answered from the Worker's Cache API (60 s), so polling it doesn't
cost DO requests. **When a daily free quota runs out** (multiplayer is down until 00:00
UTC): the client shows "Server busy, try later" and retries every 5 min. The client never
retries automatically on 4001 (full), 4005 (rate limit) or 4002 (new version: it polls
`version.txt`/`config.json` until the build changes, then reloads once).

**2.5.11 Server tick, persistence and the one alarm.** In-memory `Map slot → conn`
rebuilt from `getWebSockets()` after hibernation (last STATE kept in the socket
attachment). A `setInterval` 20 Hz broadcast runs only while something is dirty and stops
after 2 s idle, so a room where everyone stands still can hibernate. Persist **only
discrete state** (RoomState, rest poses, sessions, jukebox) to DO storage, and **only the
keys that changed since the last flush**, debounced to at most one flush per 2 s, plus on
last-player-leave. Never persist claims or leases (they belong to live sockets) or moving
poses. Usage counters live in memory and are written with the debounced flush. The free
plan counts **rows written**, not writes (a flush that rewrites RoomState, every rest pose
and every claim can be 20-50 rows): target ≤ 30 rows per minute under a 10 Hz
world-write load. A **`schemaVersion`** key is checked on wake-up; if it doesn't match the
code, `deleteAll()` and start from layout defaults (logged, counted on `/health`). Bump it
whenever stored keys change shape (4.2, 5.3, 5.4, 5.11) and list each bump in
`docs/PROTOCOL.md`. **Alarm scheduler:** a DO has exactly one alarm, and each `setAlarm`
overwrites the last, so every timed job (debounced flush, hello timeout, ghost/idle check,
grace ends, jukebox song end, lease and session timeouts) is a named deadline in one map:
`setAlarm(min(deadlines))`; on alarm, run everything due and reschedule. Nothing else
calls `setAlarm`. `webSocketClose` reciprocates the close.

**2.5.12 Free-plan cost (Workers Free; verify current numbers when implementing 3.2c).**
Limits: ~100k DO requests/day (20 incoming WebSocket messages = 1 request; outgoing
are free), ~13,000 GB-s/day duration (≈ 28 awake-hours at 128 MB), ~100k SQLite rows
written/day, 10 ms CPU per invocation.
| Traffic | Today (planned v0) | Target |
|---|---|---|
| Client sends | 20 Hz while changing + 2 s heartbeat | ≤ 20 msgs/s only while changing (state + events in one message), 0 when idle (auto-response keepalive) |
| Per moving player-hour | ≈ 3,600 requests | ≤ 3,600 worst, ≈ 1,500 typical |
| Storage rows | ≈ 22 rows/s per moving glass | only changed keys, ≤ 1 flush per 2 s, ≤ 30 rows/min under load (rows are what's counted) |
| Duration | never hibernates while anyone is connected | can hibernate when every socket is idle (one ≥ 60 s alarm, not every 5 s); AFK sockets closed after 10 min |

Example: 2 players, 3 h/day, 30 min racing → ≈ 11k requests, ≈ 1,400 GB-s: under 15%
of the free plan. 8 players for 3 h ≈ 45% typical, but ≈ 86% at the 3,600 requests/h
worst case, so at 70% of any daily quota the server tells clients to send STATE at 10 Hz
(R1). `/health` reports today's message, alarm and rows-written counters so a session
can check usage.
Other free-plan limits to respect (verify current numbers in 3.2c): Worker requests
100k/day (WebSocket upgrades, `/health`, reports and TURN-credential calls all count);
5M SQLite rows read/day; D1 free: 5M rows read and 100k rows written per day, 500 MB per
database; R2: 10 GB free, but R2 must be enabled in the dashboard and Cloudflare may ask
for a payment method (check before using it); TURN needs a TURN key made in the dashboard
(the current API token can't create one; Q6).

### 2.6 IONOS / Cloudflare split
| Data / job | Lives on | Path |
|---|---|---|
| Accounts: users, password hashes (bcrypt), sessions, badges, profile fields | **IONOS MySQL** (PHP 8 + PDO, prepared statements, utf8mb4) | `server/account.php` |
| Profile pictures | IONOS, outside the docroot (moved there from `accounts/avatars/` in 5.1b), served by `account.php` with `nosniff` + `Cross-Origin-Resource-Policy: cross-origin` | `?a=avatar&id=&rev=` |
| Login tickets | **signed on IONOS (Ed25519, `sodium_crypto_sign_detached`), verified in the Worker (`crypto.subtle`)** | HELLO / REAUTH |
| Live rooms (slots, claims, RoomState, sessions, events) | Cloudflare Room DO (memory + debounced DO SQLite storage) | WebSocket |
| Game data: best laps, rhythm bests, darts stats, leaderboards | Cloudflare **D1** (from 7.2; until then darts stats and rhythm bests are saved locally), keyed by the IONOS account id (12 hex) | Worker endpoints / DO |
| Perf and crash reports | IONOS `report.php` first (works before Cloudflare; files in a private folder outside the docroot), Worker + D1 later (8.3) | report code |
| Static game | GitHub Pages now → Cloudflare Pages (8.1) | |
| Big uploads (own avatar GLB/VRM later) | Cloudflare **R2** via the Worker, after ticket check | |
| Domains, email, homepage | IONOS | |

- **Ticket:** `base64url(payload).base64url(sig)`, payload `{v, kid, sub, name, av, b:[badges],
  role, iat, exp (15 min)}`. Login returns `{refreshToken, ticket, me}`. The refresh token
  (random, hashed in `sessions`, 30 days, rotated on use) goes only to `account.php`.
  The client refreshes the ticket every 10 min in the background and sends `REAUTH`
  over the open socket (no reconnect).
- **Keys:** PHP creates the keypair only if none exists: `fopen($path, 'x')` under `flock`,
  so two simultaneous first requests can't each write a key and strand tickets signed with
  the lost one. The secret key lives in a private folder outside the docroot (the SFTP root
  is above `home/`); `account.php?a=pubkey` serves the public keys by `kid` (list, so keys
  can rotate). The deploy's Cloudflare steps fetch `?a=pubkey` and pass it to wrangler as
  `ACCOUNT_PUBKEY` automatically (no hand pinning); `/health` reports the kid in use and
  counts tickets with an unknown kid, so a wiped key file shows up instead of everyone
  silently becoming a guest. REAUTH must carry the same `sub` as the socket's identity,
  otherwise the server closes and the client rejoins. If IONOS PHP lacks sodium (0.3's
  `report.php?env=1` tells): HMAC-SHA256 with a shared secret the user adds as a GitHub
  secret (Q2), set via `wrangler secret` + private PHP config.
- IONOS MySQL is only reachable from IONOS PHP, so Cloudflare never queries it. No
  hot-path call goes from Cloudflare to IONOS.
- Tables: `users(id CHAR(12) PK, username UNIQUE, pass_hash, display_name, bio, drink,
  song, color, avatar_rev, details JSON, role, created_at)`, `sessions(token_hash CHAR(64)
  PK, user_id, created_at, last_seen, expires_at)`, `badges(user_id, badge, granted_by,
  granted_at)`, `login_attempts(attempt_key, fails, locked_until)` (`KEY` is reserved in
  MySQL). Tests use PDO SQLite locally; keep the SQL dialect-neutral. SQLite accepts
  reserved words as names, so `account_test` also checks `schema.sql` against a MySQL
  reserved-word list.

### 2.7 Loading, streaming and the start-up lag fix
**Loader stages** (`app/loader.*`, shown on the loading screen, timed by the profiler):
| Stage | Work | Phone target |
|---|---|---|
| Boot | HTML; `<link rel=preload>` for `app.wasm`, `app.js`, the 4 biggest models; version check (1.5 s timeout) **before** injecting `app.js` | 0.3 s |
| Device | wasm compile, adapter, device | 0.6 s |
| Pipelines | every pipeline via `CreateRenderPipelineAsync`, in parallel | overlaps |
| Core | layout, physics, own avatar look first, 3 avatar pattern textures (jobs) | 0.3 s |
| Models | fetched by priority (shell → big zones → props); parse + upload in byte-budgeted jobs | ≈ 3 s Wi-Fi |
| Warm-up | one offscreen 64×64 frame per view mode (main, full mirror, avatars-only mirror) touching every pipeline/material; wait `OnSubmittedWorkDone` (race pipelines compile async in the background, not part of the gate) | 0.2 s |
| Ready | 30 frames in a row under 20 ms (or 2 s timeout) → "Tap to enter" | |
| Enter | tap = audio unlock; join the room; intro. Background (low priority jobs): race track (2.4b), piano samples (1.2b), other avatar looks | |
Nothing is rendered behind the opaque overlay except the warm-up frame.

**"Ready" definition:** files done (or failed non-critical) AND jobs drained AND all
pipelines created AND warm-up submitted and done AND stable frames.

**Jobs:** priority lanes (critical / world / background / idle), cancel tokens, cost
estimate per job kind (moving average), run only while `now + estimate < deadline`.
Every async completion (fetch, image decode, network) only **enqueues** work. Each
job ≤ 2 ms on the phone (≤ 1-2 MB of buffer upload, one texture format, one parse slice).

**Memory:** `-sINITIAL_MEMORY` = measured peak rounded up (likely 192-256 MB),
`-sSTACK_SIZE=1MB`; fetched bytes go once into a C++-owned buffer (not malloc → vector);
image decodes ≤ 4 in flight.

**Textures:** interim budget at export (1024 floor/brick/counter top, 512 default,
256 props; `createImageBitmap` resize cap 1024 as a safety net) → ≈ 4× less than the
231.6 MB today. Final: ASTC (4×4 colour/normal, 6×6 big surfaces), pre-mipped offline,
RGBA8 fallback, ≤ 64 MB resident (needs G3).

**Geometry and LOD:** an offline **bar pack** (`tools/assets/pack.py`): zone models split
into ≈ 3 m spatial clusters per material with AABBs; repeated props become instance
prototypes; packed 24 B vertices + Uint16 indices; LOD1 via Blender decimate; plus
`layout.json` (placements, colliders, walk shapes, seats, interactables, lights, mirror,
anchors, stable ids, prop collision proxies), so the floor plan exists in exactly one
place. `engine/gltf.cpp` stays for user GLBs, run in slices.

**Mirror cost:** half-resolution reflection sampled with projective UVs; cheap shader
variant (≤ 4 lights, no normal/AO/emissive maps); oblique near-plane projection instead
of `discard`; tight scissor from the glass quad clipped against the near plane; own cull
list through the glass (portal frustum), small props excluded, LOD1; skipped when the
glass is outside the frustum or hidden (occlusion query from last frame) or in zones
that can't see it; 30 Hz when the camera and the reflected avatars are still; mode
local per viewer; quality tiers full / avatars-only / off.

### 2.8 Rendering core
- Game systems only fill a per-frame **`RenderFrame`** (camera, instance transforms,
  highlights, light scales, avatar instances + bone palettes, boxes, text). The
  renderer uploads once per frame through an **UploadRing** (2-3 WriteBuffers), then
  encodes **views** (mirror, main, UI; race and monitor views later) each with its own
  cull list.
- **PipelineCache:** async creation keyed by {shader variant, vertex layout, blend,
  winding, format}; WGSL **override constants** for variants
  `{main | mirror-cheap} × {opaque | blend} × {static | instanced | skinned}`. No
  `discard` in opaque main-view shaders.
- One WGSL library (string-concatenated includes) with shared tonemap; surface
  configured with `viewFormats = {BGRA8UnormSrgb}` so manual gamma goes away.
- Lighting: CPU assigns ≤ 4 dynamic lights per cluster; static diffuse later baked
  (lightmaps or vertex colour from Blender).
- Instanced dynamic layer (boxes, glasses, candles, stools) from a storage buffer;
  avatars as instances with one bones storage buffer, frustum + mirror culling and
  LOD by distance; first person draws only the body index range.
- Retained text: per-label glyph meshes rebuilt only on change; world labels in world
  space with a per-view uniform.
- Quality tiers (DPR 2.0 / 1.6 / 1.3, mirror tier, monitor refresh) chosen from
  measured frame time with hysteresis; a settings override.
- The race becomes a second scene on the same core (pipeline cache, upload ring,
  culling), with the sky drawn last and reverse-Z depth.

### 2.9 Audio
- **AudioMixer** (C++ decides, `web/audio.js` executes one command list per frame):
  buses (music, sfx, voice, game, piano), **persistent emitters** per world source
  (piano, jukebox, dartboard, bar station, bell, rigs) with one HRTF panner each;
  short one-shots use `equalpower`; global voice pool (cap ≈ 32, steal oldest); every
  source on a bus (the rhythm Track too). Generated buffers are handed over once and
  the raw copies freed.
- **Clock** (§2.5.6): `audioNow`, `serverNow`, mapping, output latency, user
  calibration (tap-along test, stored in settings).
- **Jukebox**: `<audio>` streaming stays (right choice). Drift follower every 3 s:
  under 300 ms off → `playbackRate` 0.97-1.03; above → seek.
- **Piano**: optimized synth (≈ 10× cheaper), samples for every 3rd key, neighbours
  via `playbackRate` ±1 semitone, prepared in low-priority jobs after entering; the
  scheduler skips notes whose sample isn't ready; per-player voice tables and pedal.
- **Rhythm**: built-in charts precomputed at build time; phone songs analysed from an
  11,025 Hz mono `OfflineAudioContext` render; no 54 MB copy into wasm; buffers
  released on close.
- **Visibility**: suspend on hide, resume on show; listen to `statechange`; show
  "Tap to resume sound" if iOS interrupted the context.
- **Voice** (WebRTC stays): signaling as typed SIGNAL messages over the room socket
  with trickle ICE; default 32-48 kbps mono with "studio" opt-in (Q6); connect at most
  the 6 nearest peers; one audio node chain per peer, built once; `restartIce()` on
  `disconnected/failed` from either side; TURN credentials minted by the Worker (Q6);
  voice starts after "Everything loaded", one peer every 400 ms.

### 2.10 Web/HTML bridge
- **One mailbox per frame** (`engine/bridge.*` + `web/bridge.js`): C++ allocates an
  inbound and an outbound area in the wasm heap (e.g. 256 KB / 64 KB). JS writes:
  header `{frame, flags (visible, inputFocused, audioRunning, modalOpen), cssW, cssH, dpr,
  safeArea L/T/R/B}`, voice levels per slot, then a ring of records (net frames, UI
  commands `{id, a, b, text}`, input events with `event.timeStamp`). C++ reads it once
  per frame and writes one outbound buffer that `bar3dFlush()` executes: net frames,
  DOM ops (status text, joystick rings, log lines), UI state JSON (only when changed,
  written with a proper escaping JSON writer), audio commands.
- Canvas size from a `ResizeObserver` (`devicePixelContentBoxSize` when Safari has it,
  else `contentBoxSize × devicePixelRatio`), applied after 120 ms of stable size. No
  `getBoundingClientRect` in frames or touch handlers.
- HTML panels stay for menus, forms, file pickers (right for the iOS keyboard). One
  **modal manager** for menu + account panels; it tells C++ (`modalOpen`) to pause look
  input and drop to a cheap render while an opaque panel covers the game. No
  `backdrop-filter` over the live canvas.
- **config.json** written by the deploy (no-cache): `{build, proto, ws, accountsUrl,
  reportUrl}`. Replaces `DEFAULT_SERVER`, `cloud.json` and derived URLs (the accounts URL
  stops being derived from the relay URL in 3.4a).
- **Test isolation:** on localhost the network is off unless `?cloud=` is given; `?net=off`
  turns it off anywhere (smoke and shots use it); `check_live` connects only with `NET=1`,
  so deploy checks don't walk a ghost guest through the production room (3.4a).
- Log panel: JS ring of 300 lines, DOM updated at most once per frame and only while
  open, repeats collapsed as "×N"; errors show a red badge on the HUD; only fatal
  errors open the panel.
- Bridge JS lives in `--js-library` files with quoted property names (closure-safe);
  a CI lint flags unquoted DOM/WebGPU property access in `EM_ASM`/`EM_JS`.

### 2.11 Testing and telemetry
| Layer | Tool | Runs | Covers |
|---|---|---|---|
| Native C++ (g++, fast) and the same sources with em++ + node (bit-exact reference) | `tools/native/*.cpp` | CI + locally | vehicle targets, race determinism at 30/57.3/60/120 fps, snapshot round trips, physics (glasses sleep ≤ 2 s, no NaN), synth cost, codec vs golden vectors, JSON round trip + garbage input, activity transitions, interaction resolver, interpolation buffer, clock filter, entity ids stable. Tests that need `bar_scene` include only CPU headers (`engine/mesh_data.h`, split from `mesh.h` in 2.3b; `mesh.h` pulls in `<webgpu/webgpu_cpp.h>`, which g++ doesn't have) or are built with em++ and run in node |
| Protocol | `tools/room_test.mjs` (Node vs `wrangler dev`; replaces `tools/cloud_test.mjs` in 3.2a) | CI | slots, resume, ghosts, 4001/4002/4005, concurrent writes converge, exactly-one claim, take races, leases, jukebox advance once, rows written, alarm scheduler, rate limits. Against the live worker from the container: `NODE_USE_ENV_PROXY=1` (Node 22 ignores `HTTPS_PROXY` otherwise) |
| Accounts | `tools/account_test.mjs` (php -S + PDO SQLite) | CI | signup/login/refresh/ticket/logoutAll, lockout, bad tickets |
| Browser smoke | `tools/smoke.mjs` (headless Chromium + SwiftShader) | CI + locally | loads, "Everything loaded", non-blank screenshot, no errors, prints `bar3dPerf` |
| Multiplayer | `tools/mp_test.mjs` (CLOUD vs `wrangler dev` from 3.4c; relay mode deleted in 3.5), `voice_test.mjs` (ported to `wrangler dev` in 3.4c) | locally | two-three players, glass, jukebox, seats, darts, piano |
| Visual | `tools/imgdiff.mjs` + `?still` (0.4) | locally | "looks the same" checks at the standard spots (VIS) |
| Live | `tools/check_live.mjs` with `EXPECT_SHA` (no room socket unless `NET=1`) | after deploy | right build is live |
| Phone | `?perf=1` overlay + "Send report" code | user | real frame times, hitches, stages, heap |
| Fault injection | `?netsim=lat:100,jitter:50` in `web/net.js` (localhost only) | locally | interpolation, playout, reconnect |
Headless runs at 1-2 fps, so never judge timing there: judge CPU phase costs and counts
headless, frame rate on the phone.

### 2.12 Deploy
**Target (after 8.2)**, jobs in order (one workflow, files passed as artifacts/outputs):
**build** (read-only token, pinned emsdk, site artifact with content-hashed assets +
manifest + config.json template) → **test** (native, protocol, accounts, smoke, `php -l`)
→ **deploy-cloud** → **deploy-site** (gh-pages now; Cloudflare Pages + `_headers` after
8.1) → **deploy-ionos** (PHP API, report endpoint, homepage only; own concurrency group
that never cancels a running upload; pinned SFTP host key) → **verify** (`check_live.mjs`
with `EXPECT_SHA`).
**Until 8.2** everything stays in the one `build-and-publish` job (a separate job can't
write `config.json` into another job's workspace). The Cloudflare part is **steps** before
"Publish to gh-pages":
- Workflow `env` holds only a flag, `HAVE_CF: ${{ secrets.CLOUDFLARE_API_TOKEN != '' }}`;
  steps use `if: env.HAVE_CF == 'true'` (a job-level `if:` can't read `secrets`). The token
  and account id go only into the `env:` of the steps that run wrangler.
- `npm ci --prefix cloud && npx --prefix cloud wrangler deploy`: uses the committed
  `cloud/package-lock.json` (wrangler 4.42.0). Never `npx wrangler@x`, which resolves the
  dependency tree fresh from npm on every deploy.
- `ws` for `config.json` comes from wrangler's printed URL (or the account's workers.dev
  subdomain via the API). No subdomain registered → the step fails with a clear message.
- D1 migrations (`wrangler d1 migrations apply --remote`) run in the same steps once D1
  exists (7.2); `ACCOUNT_PUBKEY` is fetched from `account.php?a=pubkey` and passed as a var
  (5.1c).
- Dark launch (3.3): the Cloudflare steps are `continue-on-error: true`, so a Cloudflare
  problem can't stop game deploys. From 3.4d they block (the server must deploy before the
  client that needs it).
- One-off admin actions go through `.github/workflows/admin.yml` (`workflow_dispatch`,
  §How to use), never through the deploy.
Order rule: the server is always deployed before the client that needs it. **Protocol
bumps:** the worker accepts protos N-1..N for one deploy; an old client that gets 4002
polls `version.txt`/`config.json` until the build changes and then reloads once (gh-pages
caches `index.html` for up to 10 min, so reloading at once would loop).

---

## 3. System scorecard

### 3.1 Table
Rating is 1-10 (10 = keep as is). Delay = time added before the effect is seen.
| System (files) | Rating | iOS/compat | Delay | Frame cost | Sync risk | Verdict → steps |
|---|---|---|---|---|---|---|
| Frame loop (`main.cpp` frame) | 4 | rAF timestamp thrown away; no visibility handling | input→screen 33-50 ms; motion to others 300-650 ms | est. 2-5 ms CPU w/ 8 players, unmeasured | high | rewrite → 0.2, 2.2, 4.1 |
| App state `g_app` | 3 | — | — | enables duplicated scans | high | rewrite → 4.1 |
| Activity/seat state machine | 3 | — | double-seat kick after 150-400 ms | negligible | high | rewrite → 4.3, 5.2 |
| Interaction (focus, taps, use) | 5 | tap on lift ok | same frame | < 0.1 ms | medium | refactor → 4.4 |
| HUD, labels, machine screens | 5 | picture decode on main thread | — | 0.3-1 ms, grows | low | refactor → 4.1, 6.6 |
| Startup & loading orchestration (+ loading screen) | 4 | stale-version reload wastes the load | several 100 ms freeze + 4-8 s load | full scene rendered behind overlay | medium | rewrite → 1.1-1.8 |
| Input (`engine/input`) | 6 | keys stick on blur | handler-time stamps | layout read per touch event | none | refactor → 2.1 |
| GPU device/surface/pipelines (`engine/gpu`) | 6 | no device-loss recovery; 11 sync pipelines at startup (+ mip mid-load, + 2 race on first sit) | pipeline compile on first use | layout read every frame | none | refactor → 0.3, 1.3, 2.1 |
| Job queue + image upload (`gpu_upload`) | 6 | ok; no size cap | FIFO, no priorities | jobs unbounded | none | refactor → 1.4 |
| Logging + status | 4 | logs stuck on phone | — | forced layout per line | none | refactor → 0.3 |
| Embedded files (`engine/file`) | 7 | fine | — | — | none | keep |
| Menu/settings bridge (`ui.*`, `menu.js`) | 5 | ok | ≤ 1 frame | JSON poll every frame | low | refactor → 2.1 |
| Scene renderer core | 4 | per-draw pattern costly on Safari | — | ≈ 1,060 draws / 4.2k calls w/ mirror, est. 4-8 ms encode | low | rewrite → 6.1, 6.2 |
| Mirror | 4 | discard defeats Apple HSR | — | +2-4 ms CPU, +4-7 ms GPU (full) | medium (shared mode) | rewrite → 1.7, 6.3 |
| Shading & lights (`scene.wgsl`) | 6 | ok | — | est. 6-10 ms GPU main pass | none | refactor → 1.7, 6.4 |
| Model streaming + glTF | 5 | ok | 2-5 s download + 1-3 s parse | 20-80 ms parse per big GLB in callback | low | refactor → 1.4, 6.2 |
| Textures + GPU memory | 4 | ASTC not requested | — | ≈ 320 MB GPU resident | none | rewrite → 1.6, 6.8 |
| Procedural textures | 4 | fine | 0.4-1.3 s before downloads start | 5 of 8 layers unused | none | refactor → 1.1 |
| Procedural meshes + boxes | 6 | fine | — | 1 draw per box per pass | low | keep (instanced in 6.1) |
| Text renderer | 7 | fine | — | 1 draw, 0.1-0.3 ms | low | keep (retained in 6.6) |
| Picture billboards | 7 | fine | network | negligible | low | keep (by id in 6.6) |
| Bar layout data (`bar_scene`) | 5 | — | 1.2 ms build | defines 617k tris, 23 lights | medium (index ids) | refactor → 4.2, 6.2a |
| Avatars (build, pose, skinned draw) | 5 | fine | remote pose 250 ms ahead of body | 3.5 µs pose; 31k tris curly looks × 16 draws | medium | refactor → 3.4, 6.5 |
| Skeleton | 7 | — | — | negligible | none | keep |
| Skinned mesh | 6 | — | — | 13 MB/frame vertex reads worst | none | refactor → 6.5 |
| Camera | 7 | 70° vertical FOV narrow in portrait | — | — | low | keep |
| Math | 6 | — | — | — | low | refactor → 2.3 |
| Client transport (`web/net.js`) | 4 | iOS kills sockets silently | cloud 20-130 ms, relay 150-400 ms | GC churn, O(N²) lookups | high (split brain) | rewrite → 3.4 |
| C++ net API (`engine/net`) | 5 | fine | 1 frame | 2 JSON round trips/frame | medium | rewrite → 3.4 |
| Remote players (`players.*`) | 5 | fine | fixed 250 ms + transport | low | medium | refactor → 3.4 |
| Shared world + glass ownership (`shared_world.*`) | 4 | fine | 90 ms + transport + 70 ms follow | low | high | rewrite → 2.3, 5.4 |
| Locks + game world keys | 3 | — | double booking 50-400 ms | low | high | rewrite → 5.2-5.9 |
| Cloudflare room (`cloud/worker.js`) | 5 | not deployed | hello waits on IONOS | O(N) per message; storage write per change | high | rewrite → 3.1-3.3 |
| PHP relay (`relay.php`) | 3 | live path today | 150-400 ms | full file rewrite per request | medium | retire → 3.5 |
| Voice (`voice.js`) | 5 | no TURN; 510 kbps full mesh | ≥ 2.5 s setup | per-player EM_ASM per frame | medium | refactor → 2.1, 5.10 |
| Clock sync | 5 | fine | ±RTT/2 | — | medium | refactor → 3.4, 5.6 |
| Multiplayer tests | 5 | — | — | — | high (untested) | refactor → 3.2, 3.4 |
| Piano | 4 | no Web MIDI on iOS; HRTF per note | remote notes 40-170 ms ±35 ms | **45-90 ms frames on song start, up to ~1 s** | high | refactor → 1.2, 5.6 |
| MIDI parser | 8 | fine | < 1 ms | — | none | keep (tick-order fix in 5.6) |
| Synth | 5 | fine | 15-30 ms per piano note | 21 MB × 2 in JS | low | refactor → 1.2 |
| Audio bridge (`audio.*`, `audio.js`) | 5 | latency unreported; HRTF per voice | jukebox never resyncs | 4-8 EM_ASM/frame; rhythm load 130-200 MB | medium | refactor → 2.5, 5.5, 5.9 |
| Darts | 5 | fine | spectators see board 250 ms before arm | negligible | medium | refactor → 5.8 |
| 4K Mania | 5 | latency unreported (AirPods) | 0.7 s analysis + 2.5 s lead-in | full bar renders under overlay | medium | refactor → 1.1, 5.9 |
| Rhythm chart generator | 4 | output depends on sample rate | 0.7 s freeze | 1 frozen frame per start | high (non-deterministic) | refactor → 5.9 |
| Bartender | 4 | client-only badge | by design | negligible | high | rewrite → 5.7 |
| Jukebox | 5 | streaming correct | 0.3-2 s buffer, no resync | 2-3 EM_ASM/frame | medium-high | refactor → 5.5 |
| Car physics (`vehicle`) | 7 | wasm bit-exact per binary | 4.17 ms step | ≈ 0.08 ms/frame | medium | keep → 7.1 |
| Track (queries + geometry) | 6 | main-thread build | est. 150-400 ms first sit | queries only | low | refactor → 2.4, 7.6 |
| Race sim loop & rules (`World`) | 4 | — | slow motion after hitches | ≈ 0.08 ms | high (wall clock) | refactor → 2.4, 7.3 |
| Bot AI | 5 | — | — | included | high (bots per phone) | refactor → 7.4 |
| Driving controls | 6 | safe areas ignored | 80-120 ms to 63% steer | < 0.02 ms | low | refactor → 2.4, 7.6 |
| Race renderer (`view`, `race.wgsl`) | 5 | fine | first-sit hitch | ≈ 80 draws, est. 5-9 ms GPU | low | refactor → 7.6 |
| Race geometry builders | 6 | — | part of first-sit stall | 15 MB heap peak | none | keep → 7.6 |
| Race game glue (`game`) | 4 | — | first-sit long frame | < 0.2 ms HUD | high | rewrite → 2.4, 7.2 |
| Multiplayer race netcode | 1 | none exists | would be 150-250 ms | — | high | build → 7.1-7.4 |
| Spectator monitors | 2 | — | — | naive: 6 full passes | high | build → 7.5 |
| Native test tools | 5 | native ≠ wasm | — | — | none | refactor → 0.4, 2.4 |
| Racing plan doc | 6 | — | — | — | medium (false claims) | refactor → 0.1, 7.3 |
| Rigid-body physics | 6 | fine | ≤ 16.7 ms stale, no interpolation | 52-300 µs; 2.6 ms catch-up | high | refactor → 2.3 |
| Props | 5 | — | — | — | medium (index ids) | refactor → 4.2, 6.2a |
| Materials | 8 | — | — | — | none | keep |
| Walking collision | 5 | — | — | < 1 µs | low | refactor → 4.4, 6.2a |
| Page shell (`index.html`) | 6 | no CSP; home-screen storage separate | no preload: +0.2-0.8 s | log forces layout | low-medium | refactor → 0.3, 1.8 |
| Accounts client (`account.js`) | 5 | blur costly | 85-300 ms, no timeout | 1-4 ms GPU blur while open | medium | refactor → 0.1, 5.1 |
| Accounts server (`account.php`) | 4 | JSON files in docroot | 85-300 ms | — | low | rewrite → 5.1 |
| Deploy pipeline | 5 | — | 4-8 min + 10 min cache | — | medium | refactor → 0.4, 3.3, 8.2 |
| Build config (CMake) | 7 | heap grows from 16 MB | — | indirect | none | refactor → 0.4, 1.5 |
| Test & ops tools | 6 | Chromium only | — | — | none | refactor → 0.4 |
| Landing page | 8 | fine | — | — | none | keep → 8.2 |
| Hosting/caching/versioning | 5 | Pages: no headers | up to 10 min stale | — | medium | rewrite → 3.5, 8.1 |

### 3.2 Problems checklist (merged, deduplicated)
Format: **severity** (C critical, H high, M medium, L low) · problem (evidence) → fix · *[step]*.
Tags name sub-steps where a step was split (e.g. 3.2a); a tag without a letter on a split
step means all its parts. Items marked *(review)* were added on 2026-10-17.

**A. Frame loop and app core**
- [x] **H** No per-phase timing; the start-up lag can't be attributed (`main.cpp:312-324` updateFps only) → FrameProfiler + phone report. *[0.2, 0.3]* (0.2 part done: profiler + overlay)
- [ ] **H** God object `g_app`; any function changes anything (`main.cpp:66-142`; `use()` `:650-773` teleports, opens panels, toggles lights, plays sfx, sends net) → Game + systems + event bus. *[4.1a, 4.1b]*
- [ ] **H** Frame order mixes network input and local decisions: taps read others from last frame (others.update `:1249` after updateInteraction `:1237`), seat conflict checked after sending your seat (`main.cpp:1253-1265` vs `:1248`), `shared.update` inside physics with its own clock (`:844`) → phase table §2.1. *[4.1b]*
- [ ] **M** Several clocks in one frame (`emscripten_get_now` at `main.cpp:148,194,206,250,844`) → one FrameTime. *[2.2]*
- [ ] **M** ≈ 14 + 4N JS crossings and 3 JSON round trips per frame; `setState` every frame while net.js sends on a timer (`ui.cpp:11-19`, `net.cpp` receiveWorld/receiveEvents, `main.cpp:1248`) → mailbox; send on the net tick. *[2.1a, 2.1b, 3.4a]*
- [x] **M** Jobs run at frame start and at least one always runs (`main.cpp:1233`, `gpu_upload.cpp:152-161`) → deadline after submit. *[1.4]*
- [x] **M** Invisible work: full scene + mirror behind the loading overlay and the 80-88% rhythm dimmer (`main.cpp:1267-1327`, `rhythm.cpp:268`) → view modes. *[1.1]*
- [ ] **M** Every remote avatar posed/skinned even off-screen (`main.cpp:1303-1317`) → pose only visible ones. *[6.5]*
- [x] **M** Abort or device loss freezes the game forever (`index.html` onAbort; `gpu.cpp:69-74`) → fatal overlay, rejoin, auto report. *[0.3]*
- [x] **M** 16 MB default heap grows repeatedly while loading (`CMakeLists.txt` only `ALLOW_MEMORY_GROWTH`) → `INITIAL_MEMORY`. *[1.5]*
- [ ] **M** Function-local statics survive room changes (`main.cpp:749,778`; `players.cpp:38`; `shared_world.cpp:84`) → members + `reset()`. *[4.1b]*
- [ ] **M** No visibility/pagehide handling: others see a frozen you, keys stick, catch-up burst on return → lifecycle §2.5.8. *[2.2]*
- [ ] **L** Per-frame heap churn (`main.cpp:876,1239-1240,494,518`; `drawModels` sort vectors) → persistent buffers. *[4.1b, 6.1]*
- [ ] **L** Shared facts duplicated and polled (`updateMirrorButtons` `main.cpp:777-789`; `setLights` from two places) → RoomState + change events. *[5.3]*
- [ ] **L** Sound handles are raw pointers into `g_app.sfx` (`main.cpp:1385-1391`) → AudioMixer sound ids. *[2.5]*
- [ ] **L** Test hooks are regexes inside startup (`main.cpp:1186-1218`) → TestHooks parsed once. *[1.8]*
- [ ] **L** *(review)* E-7 has no global check; `app/race/game.cpp` keeps 2 `EM_ASM` (the `racing` body class) that no step removed → moved behind an engine call; `grep -rn "EM_ASM\|EM_JS" app` → 0. *[2.4b, 4.1c]*

**B. Activities, seats, interaction, HUD**
- [ ] **H** Seats: "lower id wins" after both sat; a newcomer who hasn't placed others yet can steal a seat (`main.cpp:1253-1265`, `:182`, `:235`) → server claims + pending sit animation. *[5.2]*
- [ ] **H** Rhythm cabinet and darts only checked at tap, never resolved (`main.cpp:656,680`) → claims. *[5.2]*
- [ ] **H** "Busy" defined 4 ways, `setCapture` at 7 call sites (`main.cpp:266,467,510,1018-1030,1133-1137`) → ActivityManager derives it. *[4.3a, 4.3b]*
- [ ] **M** Broadcast activity is the animation, not occupancy: making a drink while seated frees your seat for others and pops your avatar up (`main.cpp:1101-1103,1127,1144-1158,897`) → occupancy vs pose layer. *[4.3b]*
- [ ] **M** Lights/fire/mirror toggles are last-writer-wins (`main.cpp:726-738,669-676`) → intents with seq. *[5.3]*
- [ ] **L** Activities don't check each other (`startEmote` `main.cpp:199-205`; bartender shift sets no Activity) → transition table. *[4.3a]*
- [ ] **L** UI commands untyped and bypass rules (emote 0, `make`/`standUp` in any state: `main.cpp:879,897,901`; `pianoAdd` split on `\n`) → validated requests. *[4.3a]*
- [ ] **L** Teleports skip collision and eye height (`main.cpp:684,702`) → `LocalPlayer::teleport`. *[4.4]*
- [ ] **L** Tap priority hard-coded; player card beats the jukebox; dead fallback lines (`main.cpp:1035-1077,1070-1071`) → one target query. *[4.4]*
- [ ] **L** Prompt logic scattered; `hasBadge` EM_ASM + `new RegExp` every focused frame (`main.cpp:327-361,168-176`) → handler table, cached badges. *[4.4]*
- [ ] **L** Interactable fields overloaded (`value` = mirror mode or drink slot) + big switch (`bar_scene.h:116-121`) → typed components + handlers. *[4.4]*
- [ ] **M** All text rebuilt and uploaded every frame (`main.cpp:452,575`) → retained text. *[6.6]*
- [ ] **L** Others' darts are added to the 3D boxes from inside `queueText`; they vanish during rhythm/race (`main.cpp:435,454-459`) → RemoteActivityVisuals in Present. *[4.1c]*
- [ ] **L** Spectator screens use the newest values while avatars play 250 ms late → same timeline. *[3.4b]*
- [ ] **L** Failed profile picture never retried; decoded with `getImageData` (`main.cpp:378-379`, `fetch.cpp:117-146`) → retry with backoff, `createImageBitmap` path, delete `decodeImage`. *[6.6]*
- [ ] **L** Billboard slots by player index (`billboard.h` kSlots 8, `main.cpp:383`) → keyed by slot/id + LRU. *[6.6]*

**C. Start-up and loading**
- [ ] **H** `onGpuReady` does everything synchronously: scene, physics, 8 avatar looks (`avatar.cpp:74`), CPU textures, sounds and 11 sync pipelines (renderer ×9 `renderer.cpp:312-391`, `text.cpp:107`, `billboard.cpp:74`); the mip pipeline(s) are created lazily per format mid-load (`gpu_upload.cpp:74-96`) and the 2 race pipelines on the first sit (`view.cpp:142,159`): 14 call sites (`main.cpp:1356-1400`) → staged loader + async pipelines + jobs. *[1.1, 1.3, 1.8]* (1.1 + 1.3 parts done: textures cut, pipelines async)
- [x] **H** Procedural textures ≈ 410 ms (8 layers), 5 unused (≈ 245 ms), run before any download starts (`bar_scene.cpp:551-563`, `renderer.cpp` createMaterials) → keep 3 layers, fetch first. *[1.1]*
- [x] **H** "Ready" means files arrived, not smooth; pipelines compile on first use; per-texture mip submits (`main.cpp:1183`) → warm-up + stable-frames gate. *[1.3]*
- [x] **H** `parseGlb` runs untimed in the fetch callback; `finish()` builds a whole model in one job (`model.cpp:184`, `finish` `:87`) → sliced jobs. *[1.4]*
- [x] **H** A finished piano song's `pso` stays; every joiner downloads and synthesizes 44-56 keys from the moment the world arrives (net starts in `onGpuReady`, `main.cpp:1392-1393`, so during loading at 3 keys/frame) onward (`piano.cpp:282-285`; break at `:274` before the skip at `:276`) → joiners ignore ended songs (song end computed from the parsed MIDI, no wire change); owner clears it. *[1.2a]*
- [ ] **M** Race track + race pipelines built synchronously on the first sit (`race/game.cpp:35-42` `world_.init()` + `view_.init()`) → pipelines async during loading, track in background jobs after enter. *[1.3, 2.4b]* (1.3 part done)
- [x] **M** You join the room while still loading, visible at spawn (`main.cpp:1392-1393`) → join on enter. *[1.1]*
- [ ] **M** Voice connects to every peer from page load with 2.5 s ICE waits (`voice.js:178-185,314`) → start after enter, staggered; trickle ICE later. *[1.1, 5.10]* (1.1 part done)
- [x] **M** Failed downloads count as done, no retry, no fail state (`model.cpp` done(nullptr), `renderer.cpp:523`, `index.html:116-128`) → retry ×3, fail UI with Retry. *[1.4]*
- [ ] **M** `version.txt` reload races the app start, errors swallowed (`index.html:93-103`) → check first (1.5 s timeout), later no-cache index. *[1.8, 8.1]*
- [ ] **M** No preload of wasm or models (`index.html:141-152`) → `<link rel=preload>`. *[1.8]*
- [x] **M** All images decoded at once; bitmaps pile up in `Module.bitmaps` → ≤ 4 in flight. *[1.4]*
- [ ] **M** Each file copied 4-5 times through JS and wasm (`fetch.cpp:79-92,53`; gltf image copies) → one copy; GPU data from JS buffers in the pack. *[1.5, 6.2c]*
- [ ] **L** Overlay fade/removal can hitch (`index.html:42-43,112`) → `will-change`, remove on `transitionend`. *[1.8]*
- [ ] **L** HUD buttons work before the world exists (`#hud` z-index 6 over `#loading`) → hidden until enter. *[1.8]*

**D. Engine plumbing (input, GPU device, jobs, logs, bridge, build)**
- [x] **H** Device loss only logged (`gpu.cpp:69-74`) → recovery overlay + report. *[0.3]*
- [x] **H** Repeating GPU errors flood the DOM log and cause lag (`gpu.cpp:65-68`, `index.html:80-86`) → dedupe, rate-limit. *[0.3]*
- [ ] **M** Canvas size read through layout every frame and in touch handlers, interleaved with DOM writes (`gpu.cpp:101-114`; `input.cpp:63,84,144`; `showStick` `input.cpp:14-23,121`) → ResizeObserver + batched DOM. *[2.1a]*
- [ ] **M** Raw presses use handler time, not `event.timeStamp` (`input.cpp:86,93,159`) → event timestamps. *[2.1a]*
- [ ] **M** Input internals public; race reads them (`input.h:55-91`) → immutable InputFrame. *[2.1a]*
- [ ] **L** Keys and stick never cleared on blur/visibility/pagehide (`input.cpp:214-218`) → clear all. *[2.1a]*
- [ ] **L** `document.activeElement` EM_ASM on every key (`input.cpp:185-190`) → focus flag in mailbox. *[2.1a]*
- [ ] **L** `beginFrame` skips Outdated/Lost forever (`gpu.cpp:134-142`) → reconfigure + count. *[2.1a]*
- [ ] **L** Canvas follows Safari's toolbar, reallocating targets (`index.html:16`) → debounced size / `100lvh`. *[2.1a]*
- [x] **M** Mip pipelines created synchronously mid-load (`gpu_upload.cpp:74-96`) → async at startup. *[1.3]*
- [x] **M** No texture size cap (`gpu_upload.cpp` full-size textures) → resize cap + export budget. *[1.6]* (resize cap + dedupe done; export budget later)
- [x] **L** Bitmap leak if the pending entry is missing; global queue can't cancel → always drop; cancel tokens. *[1.4]*
- [x] **M** Log panel unbounded, forced layout per line (`index.html:80-86`) → ring + rAF batch. *[0.3]*
- [x] **M** Any error auto-opens the log over ≈ 40% of the screen and swallows taps (`index.html:82`; `audio.js:22,305,327`) → badge; warn level for audio. *[0.3]*
- [x] **M** Logs never leave the phone (`log.h:6-7`) → report endpoint + code. *[0.3]*
- [x] **L** No levels, timestamps or categories in logs → add. *[0.3]*
- [ ] **M** `setInfo` JSON hand-built without escaping; a `\` in a MIDI file name silently freezes menu info (`main.cpp:923-937`, `menu.js:378`) → JSON writer + log parse failures. *[2.1b]*
- [ ] **M** *(review)* `engine/json.{h,cpp}` was never audited, yet from 3.4 it parses untrusted network JSON (DELTA, EVENTS, PROFILE with user names); it allocates `shared_ptr` maps per value and has no writer → writer + depth/size limits in 2.1b; garbage-input cases in proto_test. *[2.1b, 3.1]*
- [ ] **L** UI command poll stringifies `[]` every frame (`ui.cpp:11-19`) → push model. *[2.1b]*
- [ ] **L** Static catalogs re-sent every 500 ms (`main.cpp:920-939`) → once + on change. *[2.1b]*
- [ ] **L** Avatar look set through two paths (`menu.js:153-155`) → command only. *[2.1b]*
- [ ] **L** Mic button DOM rewritten every 150 ms (`menu.js:354-360`) → on change. *[2.1b]*
- [ ] **L** Dead bridge code (`net.cpp:80-81` panelOpen; `bar3dUi.isOpen`) → modal manager. *[2.1b]*
- [ ] **L** Backdrop blur over the live canvas (`account.js:62`); two modal systems both z-index 8 → opaque dim + modal manager + cheap render. *[2.1b]*
- [x] **M** Closure renames unquoted EM_ASM names in Release only → JS libraries + lint + Release smoke in CI. *[0.4]*
- [x] **L** `web/` copied twice concurrently (POST_BUILD + `site_assets`) → one staging step. *[0.4]*
- [x] **L** Models re-gzipped on every build → per-file custom commands. *[0.4]*
- [x] **L** No `STACK_SIZE`, no `-Werror` on engine → set. *[1.5]*
- [x] **L** *(review)* `-Werror` would break the build today: `engine/net.cpp:68:61` "empty character constant" inside an EM_ASM body (the only warning across `engine/*.cpp`) → fix it first, verify with a clean rebuild. *[1.5]*
- [ ] **L** Shaders via `--embed-file` pull in the FS runtime (`CMakeLists.txt:23`) → generated header (optional). *[6.4]*

**E. Rendering**
- [ ] **C** ≈ 1,060 draws / 4,200 encoder calls per frame with the full mirror; no batching or instancing (`drawModels` `renderer.cpp:551-569`, 524 primitives) → clusters + instancing (≤ 150 / ≤ 60). *[6.2d, 6.2e]*
- [ ] **C** No frustum culling; model bounds never read (`drawModels`; `model.h` bounds; race already has `visible()`) → engine culling + clusters. *[6.1, 6.2d]*
- [x] **H** `discard` in every opaque fragment entry point kills Apple hidden-surface removal (`scene.wgsl:159,179,279,290`) → variants without clip + oblique mirror projection. *[1.7]*
- [ ] **H** Full mirror redraws the whole scene at full resolution with the full shader (`renderMirror`, `resizeMirrorTargets`, `mirror.wgsl` 1:1 `textureLoad`) → half-res, cheap variant, own cull list. *[1.7, 6.3]*
- [ ] **H** Mirror visibility test weak: any corner behind the camera → full-screen reflection; walls never occlude it (`renderMirror` scissor fallback) → near-plane clip, frustum skip, occlusion query, zone gate. *[1.7, 6.3]*
- [ ] **H** All textures uncompressed RGBA8: 231.6 MB models, ≈ 320 MB GPU total (`glb_report.py`) → export budget, then ASTC. *[1.6, 6.8]*
- [ ] **M** Mirror mode shared: one player raises everyone's GPU cost (DECISIONS Shared world `m<i>`) → local per viewer (Q3). *[1.7]*
- [ ] **M** Gameplay writes GPU buffers at random points (`setGlow`, `setLightScale`, `setModelBaseGlow`, `setModelTransform`, `setOthers`) → RenderFrame + one upload phase. *[6.1]*
- [ ] **M** Mirror glass drawn after the blend pass with depth write: paints over a glass held in front of it (`render`, `createGlassPipeline`) → draw with opaques. *[6.1]*
- [ ] **M** Blend sort by placement origin; room-sized zone models at the origin (`drawModels`; `bar_scene.cpp:194,273,315,341`) → sort by part/cluster bounds centre. *[6.2d]*
- [ ] **M** Light grid 8 per cell with seams; 2 crowded cells (`buildLighting`) → ≤ 4 dynamic per cluster; bake later. *[6.4]*
- [ ] **M** 5 texture fetches per pixel even with 1×1 fallbacks, anisotropy 8 (`scene.wgsl:247-251`, `renderer.cpp:363`) → ORM packing + feature variants; aniso 4. *[6.4]*
- [ ] **M** Fixed DPR 2, no adaptive quality (`gpu.cpp:16`) → quality tiers. *[6.7]*
- [ ] **M** Asset URLs unversioned while wasm is cache-busted (`renderer.cpp:520`, `jukebox.cpp:45`, `piano.cpp:107`) → hashed manifest. *[8.1]*
- [ ] **M** Floor plan duplicated in `tools/assets/layout.py` and `bar_scene.cpp`; three collision descriptions (physics boxes, walk rects, GLBs) → one `layout.json`. *[6.2a, 6.2b]*
- [ ] **M** Curly-hair looks 20.9k verts / 31k tris, × up to 16 draws, no LOD or culling (`avatar.cpp:205-219`) → ≈ 2k-vert hair, LOD, cull. *[6.5]*
- [ ] **M** First-person body skins the collapsed head (≈ 15k wasted verts) (`avatar.cpp:560-573`, `drawSkinned`) → index ranges. *[6.5]*
- [ ] **M** CPU texture arrays: `pow()` per channel per texel, 4 MB copies (`texture.cpp:93-116`) → LUT, only tiny images, in jobs. *[1.1]* (LUT + no copies done; jobs in 1.4)
- [ ] **L** Manual gamma; tonemap duplicated (`scene.wgsl:394`, `race.wgsl`) → sRGB view + shared include. *[6.4]*
- [ ] **L** Skinned path samples the texture array 3× (`scene.wgsl:183-194`) → fewer samples. *[6.5]*
- [ ] **L** Boxes: one draw + 256 B slot each, both passes (`setBoxes`/`drawBoxes` `renderer.cpp:607-638`) → one instanced draw. *[6.1]*
- [ ] **L** Uint32 indices and 48 B vertices everywhere → packed 24 B + Uint16. *[6.2c]*
- [ ] **L** Legacy SceneObject path costs setup with 0 objects → delete. *[6.1]*
- [ ] **L** One submit per texture for mips → batch (gone with ASTC). *[1.4, 6.8]* (1.4 part done: one submit per image job)
- [ ] **L** An image used as colour and data gets two textures (`model.cpp`) → one texture + sRGB view format. *[6.2c]*
- [ ] **L** World text never in the mirror (`text.cpp:233-234`) → per-view uniform, or document as intended. *[6.6]*
- [ ] **L** One full mesh per look; per-player bones buffer + WriteBuffer → palette + one storage buffer. *[6.5]*
- [ ] **L** Reflection re-rendered every frame even when nothing moves → 30 Hz when still. *[6.3]*

**F. Networking and the server**
- [ ] **C** Split brain: a client silently falls back from Cloudflare to the relay (separate room) after 3 failures or "room full" (an HTTP 503 at upgrade, `worker.js:116`), or when the `cloud.json` fetch fails (`.catch(useRelay)`, `net.js:303-305`) (`net.js:136-145`) → one backend per deployment, "offline, retrying". *[3.4a, 3.4d]*
- [ ] **C** No echo + optimistic apply: concurrent writes to one key diverge for good on Cloudflare (`worker.js:172-184` broadcast to all but writer; `net.h:47-49`; the relay converges because it keeps one entry per key) → server seq + echo to everyone. *[3.2b]*
- [ ] **M** *(review)* Echoes of your own writes (needed for I-2) break code that assumes none: stale `kMoving` echoes switch your resting thrown glass to remote mode and rewind it by about one RTT (`shared_world.cpp:106-160`); `jb`/`pso` echoes could restart music → shim delivers an own echo only when the server's value differs from our latest write. *[3.4a, 3.4c]*
- [ ] **H** Nothing is decided by a server: seats, machines, glasses, drinks, lights, jukebox advance → claims and intents. *[5.2-5.9]*
- [ ] **H** No idle timeout: suspended iPhones become ghosts holding seats and glasses (`worker.js:206-218` removes only on close/error) → ghost check + grace + AFK close. *[3.2c]*
- [ ] **H** Free-plan cost: 20 Hz sends, never hibernates, `storage.put` of the whole world per world message (≈ 22 rows/s per moving glass) (`worker.js:187`) → batching, debounced persistence of changed discrete keys. *[3.2b, 3.2c]*
- [ ] **M** *(review)* A 5 s ghost alarm would keep the DO awake while anyone is connected (an open desktop tab ≈ 28% of the daily duration per 8 h) → checks inside message handling, one ≥ 60 s idle alarm, AFK close after 10 min (§2.5.9). *[3.2c]*
- [ ] **M** *(review)* A DO has one alarm; separate `setAlarm` calls for persistence, ghosts, jukebox, timeouts overwrite each other → alarm scheduler (§2.5.11). *[3.2c]*
- [ ] **M** *(review)* Free-plan gaps: rows (not writes) are counted; Worker requests include upgrades and `/health`; a public `/health` costs a DO request per call; any `/room/<name>` creates a DO; no per-IP connect limit; nothing defined for an exhausted quota → changed-keys persistence, cached `/health`, per-IP limit, only `main` persisted, "Server busy" + 5 min retry. *[3.2a, 3.2c, 3.4a]*
- [ ] **M** *(review)* Room DO storage changes shape at 3.2, 4.2, 5.3, 5.4, 5.11 with no schema version → `schemaVersion` key, wipe to layout defaults on mismatch. *[3.2a; bumps in 4.2, 5.3, 5.4, 5.11]*
- [ ] **M** Identity = client-chosen public id; anyone can kick, impersonate or read another's messages (`worker.js:114-118`; `relay.php:55-57,125-126`) → server slots + resume secret. *[3.2a]*
- [ ] **M** No protocol/build version in hello (`net.js:94`, `worker.js:13`) → proto field, close 4002, reload prompt. *[3.2a]*
- [ ] **M** While reconnecting, status stays "connected" and others freeze for up to 8 s (`net.js:128-134,145`) → stale/dimmed players, fast first retry, snapshot resync. *[3.4a, 3.4b]*
- [ ] **M** Log in/out tears down the socket: everyone sees you leave and rejoin, voice drops (`net.js:149-151`) → in-band REAUTH. *[5.1c]*
- [ ] **M** Unbounded queues replay stale bursts after stalls (`net.js:46,53,56`) → latest-per-player, event TTL, collapse by key. *[3.4a]*
- [ ] **M** Hello waits for an uncached IONOS fetch with no timeout; messages meanwhile are dropped (`worker.js:136-147,221-232`) → guest welcome + async verify; tickets later. *[3.2a, 5.1c]*
- [ ] **M** Discrete state applied on arrival, positions 250 ms later: seated before arriving, pop on stand-up (`players.cpp:61` vs `:83`) → samples carry everything. *[3.4b]*
- [ ] **M** No extrapolation: freeze then jump (`players.cpp` clamps f to [0,1]); fixed 250 ms not tuned to a 50 ms tick → adaptive delay, 100 ms extrapolation. *[3.4b]*
- [ ] **M** 18 positional float slots with activity-dependent meanings, 2 spare of `MAX_VALUES` 20 (slot enum `players.h:18-31`, `kParam` at `:22`) → binary avatar block + activity TLV. *[3.4a, 4.3b]*
- [ ] **M** Piano notes over two paths (data channel or server) → keyUp overtakes keyDown, stuck notes (`net.js:364-370`) → one ordered path. *[5.6]*
- [ ] **M** Events have no timestamp, seq or playout (`net.h:62-68`); client fans out per recipient (`net.js:366-369`) → envelope, '*' fan-out on server. *[3.2b, 5.6]*
- [ ] **M** Three timelines (sender clock for avatars, server seconds for music, arrival time for glasses/events) → server time everywhere. *[3.4b]*
- [ ] **M** Glass knocked: several clients claim at once; the loser snaps (`shared_world.cpp:181-191,247-254`) → server leases; only own bodies trigger claims. *[5.4]*
- [ ] **M** Owner leaves mid-throw: every client drops its own copy somewhere else; server keeps stale "held" (`shared_world.cpp:201-206`; `worker.js:206-213`) → server ends lease, hands a settle lease. *[5.4]*
- [ ] **M** Glass extrapolation up to 300 ms pushes remote glasses through the counter (`shared_world.cpp:209`; kinematic skips static collision) → snapshot interpolation. *[5.4]*
- [ ] **M** Glass keys are body indices; DO caps world keys at 96 (`shared_world.cpp:33`; `worker.js:30,179`) → stable ids, props in TICK, only rest poses stored. *[4.2, 5.4]*
- [ ] **L** `serverTime()` is 0 before the first pong; callers write 0 (`net.js:390-393`; `shared_world.cpp:234`) → time in WELCOME; server stamps start times. *[3.4a, 5.5]*
- [ ] **L** Clock pings every 2 s; first estimate off by up to an RTT (`net.js:170-173`) → 5-ping burst at connect. *[3.4a]*
- [ ] **L** Room full = unreadable 503, client retries forever (`worker.js:116`) → accept, send reason, close 4001; client doesn't auto-retry on 4001/4002/4005. *[3.2a, 3.4a]*
- [ ] **L** Player cap counts only hello'd players; no-hello sockets unlimited → count sockets; 5 s hello timeout. *[3.2a]*
- [ ] **L** No rate or size limits on fan-out; no Origin check (`worker.js:126,191-203`) → token bucket + allowlist. *[3.2a, 3.2c]*
- [ ] **L** `webSocketClose` doesn't reciprocate (`worker.js:206`, compat date 2025-09-01) → `ws.close()`. *[3.2a]*
- [ ] **L** O(N) attachment deserialization per message (`worker.js:89-101`) → in-memory map. *[3.2a]*
- [ ] **L** After hibernation, states empty until the next heartbeat (`worker.js:74-77`) → keep last state in attachment. *[3.2c]*
- [ ] **L** *(review)* `tools/cloud_test.mjs` tests the v1 protocol and would overlap room_test → renamed/extended into `tools/room_test.mjs`; `tools/setup.sh` runs `npm ci --prefix cloud` so `wrangler dev` works in fresh sessions. *[3.2a]*
- [ ] **M** *(review)* After 3.4, `check_live` (run after every deploy) and local smoke/shots would join the production room as guests (a ghost walks in; DO requests spent); `?server=off` (`shots.mjs:39`) means nothing in v2 → net off on localhost unless `?cloud=`, `?net=off`, `NET=1` for check_live. *[3.4a]*
- [ ] **L** Worker not deployed; `wrangler.toml:2` comment false; `cloud/.wrangler` state committed to the public repo → deploy job; gitignore. *[0.1, 3.3]* (0.1 part done: comment fixed, `.wrangler` untracked + ignored)
- [ ] **L** Remote piano players get YOUR piano's hand pose (`main.cpp:1153`) → from their notes/pose. *[5.6]*
- [ ] **L** A reconnecting player gets a new index: history lost, teleport (`net.js:82-87` drops the player, `net.js:71` `nextIndex++`, `players.cpp:73-76` erases it) → stable slot. *[3.4a]*
- [ ] **M** Relay: trusted ids, readable inboxes, over-limit players silently invisible, no fetch timeout, rev reset (`relay.php:55-57,125-130,102`; `net.js:183-189`) → retire. *[3.5]*
- [ ] **M** Multiplayer tests cover only happy paths; `mp_test` CLOUD mode still polls PHP and always hits its 60 s timeout (`mp_test.mjs:38,88-93`) → room_test + fixed mp_test. *[3.2a, 3.4c]*
- [ ] **H** Voice 510 kbps per peer in a full mesh (≈ 7.6 Mbps uplink at 16 players) (`voice.js:172`, `menu.js:15`) → 32-48 kbps default (Q6), ≤ 6 nearest peers. *[5.10]*
- [ ] **H** No TURN: cellular/strict-NAT users can't connect (`voice.js:16`) → Cloudflare TURN via the Worker (Q6). *[5.10]*
- [ ] **M** Web Audio nodes leak and audio doubles: `playRemote` rebuilds the chain each call, `drop()` never disconnects (`voice.js:216-251,262-268,336-340`) → one chain per peer. *[5.10]*
- [ ] **M** Game events over the data channel bypass the server (`voice.js:211-214`) → room path only. *[5.6]*
- [ ] **L** Only `failed` handled; non-callers never retry (`voice.js:194-198,309`) → `restartIce` either side. *[5.10]*
- [ ] **M** *(review)* A public TURN-credential endpoint would let anyone relay traffic on the user's TURN allowance → credentials only as a message on an open HELLO'd socket, TTL ≤ 10 min, ≤ 1/min per slot. *[5.10]*
- [ ] **L** Voice level and panner placement via EM_ASM per player per frame, each an O(N) id lookup (`main.cpp:486-489,1130,1163-1170`; `voice.js:355-368`) → one batch at 20 Hz through the mailbox. *[2.1c]*

**G. Games and audio**
- [x] **C** Piano synthesizes notes inside the frame: 16.7 ms average, 30.6 ms middle C (wasm; native g++ -O2 15.7-20 ms middle C, 9.1 ms average); 3 per frame, budget up to 40 after a slow frame (`piano.cpp:21,259-267`; also in `keyDown` `:181`) → optimized synth, every 3rd key + playbackRate, prepare in jobs, skip not-ready notes. *[1.2a, 1.2b]*
- [x] **H** Synth ≈ 10× too expensive: 96 full-length `addPartial` passes for middle C (16 partials × 3 strings × 2 decay stages; `synth.cpp:25-44,83-92`) → one pass per partial, -80 dB cutoff, strings in one loop. *[1.2a]*
- [x] **H** One live-voice table and pedal for you and every remote player; stuck glowing keys (`piano.h:100-104`; `piano.cpp:182,197,233,325`) → per-player tables, release on leave/timeout. *[1.2b]*
- [ ] **H** Remote notes unstamped, played on arrival in 50/100 ms clumps, stale bursts after background (`piano.cpp:234`; `relay.php:122`) → stamped events + playout. *[5.6]*
- [ ] **H** HRTF panner per voice for every note and sfx (`audio.js:182-192`) → persistent emitters, equalpower one-shots, voice cap. *[2.5]*
- [x] **M** MIDI-keyboard pedal not shared (`main.cpp:896`; the on-screen pedal does send, `piano.cpp:410-412`) → `setPedal(down, share)`. *[1.2b]*
- [ ] **M** Concurrent piano song picks diverge (`piano.cpp:143`) → server session record. *[5.6]*
- [ ] **M** Piano key lights: 2 boxes per lit key, one draw each, redrawn in the mirror (`piano.cpp:312-360`) → instanced boxes. *[6.1]*
- [x] **L** Dead `busy()`; own .mid leaves `pso` at the old song for others (`piano.h:50`; `piano.cpp:141-144`) → remove; explicit local mode. *[1.2a]*
- [x] **L** Long bass notes cut at 3.6 s (`synth.cpp:78`); engine hum uses `sin` per sample (`synth.cpp:184`) → loop tail; phasor. *[1.2a]*
- [ ] **L** MIDI events on the same tick not ordered off-before-on (`midi.cpp:107-110`) → tempo < off < pedal < on. *[5.6]*
- [ ] **H** Rhythm Track ≈ 200 MB peak, never released; 54 MB copied into wasm (`audio.js:236-260`; `audio.cpp:77-81`) → low-rate mono via OfflineAudioContext, no copy, free on close. *[5.9b]*
- [ ] **M** Generated sounds kept twice in JS (`audio.js:153-166`) → drop raw data after the AudioBuffer exists. *[2.5]*
- [ ] **M** Rhythm Track bypasses the volume buses (`audio.js:281`) → bus. *[2.5]*
- [ ] **M** No audio-latency calibration; AirPods shift judgements 150-250 ms (`audio.js:290`) → calibration + Clock. *[5.9b]*
- [ ] **M** Jukebox stream seeks once and never resyncs (`audio.js:312-328`) → drift follower. *[5.5]*
- [ ] **L** No visibility/interruption handling in audio (`audio.js:92-101`) → statechange + "tap to resume sound". *[2.2]*
- [ ] **L** 4-8 EM_ASM audio calls per frame (`audio.cpp`) → command buffer. *[2.5]*
- [ ] **L** Unlock listeners never removed; object URLs never revoked (`audio.js:100-104,119`; `menu.js:253`) → clean up. *[2.5]*
- [ ] **M** Darts: spectators see no flight and hear nothing (`darts.cpp:124,143,156`) → throw events. *[5.8]*
- [ ] **L** Darts position encoding overflows off-board (`darts.cpp:30-31,112`) → clamp ±0.499. *[5.8]*
- [ ] **L** Darts: leaving mid-turn leaves ghost darts locally (`darts.h:23`) → `leave()` clears. *[5.8]*
- [ ] **L** Darts: no turns, score not saved → session turn queue; stats saved locally, uploaded to D1 in 7.2. *[5.8, 7.2]*
- [x] **H** 4K Mania renders the whole bar under its overlay (`main.cpp:454-458` vs `:1326`) → Overlay view mode. *[1.1]*
- [ ] **H** Chart analysis 0.7 s on the main thread, redone with re-download on every Retry (`rhythm.cpp:89,131,180-182`) → prebaked built-in charts, cache, keep buffer while open. *[5.9a, 5.9b]*
- [ ] **H** Charts differ by device sample rate (44.1 vs 48 kHz: Easy 242 vs 288 notes, 15 match) (`rhythm_chart.cpp:58` `lround(rate/11025)` gives 11,025 Hz at 44.1 kHz but 12,000 Hz at 48 kHz) → fixed 11,025 Hz, hop in seconds. *[5.9a]*
- [ ] **M** Tempo quantized to frames, notes on grid not onsets (`rhythm_chart.cpp:135,177,208-210`) → parabolic lag refine, snap to onset (verify by listening). *[5.9a]*
- [ ] **M** Spectators see a fake falling-note pattern (`rhythm.cpp:441`) → session + real chart + judgement events. *[5.9b]*
- [ ] **L** Rhythm bests lost on reload; song list cut to 7 (`rhythm.cpp:49,67,113`) → save locally (D1 in 7.2); scrolling list. *[5.9b]*
- [ ] **H** Drinks can be duplicated (two takers) or lost (two servers into one slot) (`bartender.cpp:91-98,124-143`) → server serve/take. *[5.7]*
- [ ] **M** Bartender badge checked only on the client (`main.cpp:168-177`; `menu.js:293`) → server checks the ticket. *[5.7]*
- [ ] **M** Serving into a full counter does nothing (`bartender.cpp:126`) → "Counter full" message. *[5.7]*
- [ ] **M** Spectators can't hear drinks being made (`bartender.cpp:118`) → step events from the station emitter. *[5.7]*
- [ ] **L** `make()` overwrites a drink in progress (`bartender.cpp:79-89`; `main.cpp:765`) → confirm. *[5.7]*
- [ ] **H** Every client auto-advances the jukebox and writes `jb` with its own start time (`jukebox.cpp:80`; `shared_world.cpp:165-172`) → server-owned queue + alarm. *[5.5]*
- [ ] **M** Phone songs take over the jukebox only for you (`jukebox.cpp:71-78`; `shared_world.cpp:168`) → "My music" local mode, ducks the room (Q8). *[5.5]*
- [ ] **L** Songs identified by list index; changes found by polling a counter (`shared_world.cpp:103-105`; `jukebox.h:38-39`) → stable song ids, commands. *[5.5]*

**H. Racing**
- [ ] **C** No race protocol or channel: 5 floats in the generic slots (`game.cpp:157-166` `stateFor`; packed at `main.cpp:1134-1138`; `worker.js:29` MAX_VALUES 20) → race sub-protocol. *[7.2, 7.3]*
- [ ] **H** Results depend on frame rate (`world.cpp:150` `stepPhysics(kStep, std::min(simMs_, nowMs))`; race_test with all bots at 30/60/120 fps gives different winners and times, e.g. 60 fps: Mika 226.4 s vs Lena 227.9 s) → tick-based sim. *[2.4a]*
- [ ] **H** Sim clock drifts from wall clock after every hitch (kMaxSteps = 30 at `world.cpp:12` is unreachable: the dt cap of 0.1 s at `main.cpp:1227` allows at most 24 steps); HUD lap time uses wall `nowMs` (`game.cpp:190`), start lights `world_.lightsOn(nowMs)` (`game.cpp:148`); race clock set at `world.cpp:114-116`, steps at `:145-154` → ticks; server epoch later. *[2.4a, 7.2]*
- [ ] **H** No render interpolation: 3/4/5 steps per frame = 0.75-1.25 m judder at 60 m/s; render draws the current car state (`game.cpp:93` update, `:146-148` carDraw) → prev/cur lerp. *[2.4a]*
- [ ] **H** Remote cars: infinite mass on both phones, double impulse, never exercised (`world.cpp:365-376`; `Racer::remote` is never set true) → proxies, finite mass, impulse on own car only, ContactEvents. *[7.3]*
- [ ] **H** Each racing phone runs its own 7 bots; races are chaotic (`kBots` `game.cpp:19`, used at `:48`; bots driven at `world.cpp:171`) → one bot authority. *[7.4]*
- [ ] **H** Rigs claimed client-side after the fact (`main.cpp:1252-1263`) → claims. *[5.2]*
- [ ] **H** Race time comes from `emscripten_get_now` in main (`main.cpp:250` `race.start(…, emscripten_get_now())`, `:1224`; race clock `world.cpp:114-116`), no server clock in C++ → `serverNowMs`, goAt from the server. *[3.4a, 7.2]*
- [ ] **H** Sends are timer-driven and not tick-stamped (`net.js` setInterval) → sent from the frame loop, tick-stamped. *[3.4a, 7.3]*
- [ ] **H** Spectators have no data; naive monitor rendering is unaffordable → snapshot stream + MonitorRenderer. *[7.5]*
- [ ] **H** Race renderer: no LOD, no distance or backface culling; terrain chunk never culled (`view.h:20` zFar 2500; `view.cpp:218,394-396`; `track.cpp:441-510`) → LOD chunks, tiles, far cull, culling. *[7.6]*
- [ ] **M** No VehicleState save/restore; private fields; CarSpec with vectors copied per car (`vehicle.h:87-93,187-191`) → POD state + `const CarSpec*`. *[7.1]*
- [ ] **M** Determinism only for identical wasm; native ≠ wasm; `sin` in the surface hash (`track.cpp:60-63`) → document, em++ tests, integer hash, build hash in race join. *[0.1, 2.4a, 2.4b, 7.3]* (0.1 part done: documented in DECISIONS → Restructure)
- [ ] **M** Resets ignore other cars, no cooldown (`world.cpp:137-143,180,250`; `game.cpp:140`) → free slot, 2 s ghost, cooldown. *[2.4b]*
- [ ] **M** Start time local and pseudo-random (`world.cpp:115`) → server goAt. *[7.2]*
- [ ] **M** Positions and finish computed locally (`world.cpp:155-163,408-411`) → server results. *[7.2]*
- [ ] **M** Contact solver: no slop or iterations, jitter in sustained contact (`world.cpp:286,366-367`) → Baumgarte 0.2-0.4, 1-2 cm slop, 2 iterations. *[7.3]*
- [ ] **M** No track LOD; ≈ 18 quads per metre, unshared vertices (`track.cpp:321-380`) → LOD chunks, shared strips. *[7.6]*
- [ ] **M** Surface bumps stepwise every 0.59 m → damper spikes (`track.cpp:261-264`) → smooth value noise. *[2.4b]*
- [ ] **M** Curbs/lines z-fight beyond ≈ 50 m (Depth24Plus, near 0.1) → reverse-Z Depth32Float. *[7.6]*
- [ ] **M** Procedural shading sparkles, no anti-aliasing (`race.wgsl:135,149`; `view.cpp:208-219`) → fade octaves by distance, MSAA 4× option. *[7.6]*
- [ ] **M** Wasted fragment work: car-shadow loop with sin/cos per pixel; sky drawn first (`race.wgsl:96-113`; `view.cpp:225-228,389-391`) → CPU sin/cos; sky last. *[7.6]*
- [ ] **M** 40 B vertices + Uint32 indices, 11.4 MB kept all session (`mesh.h:29-34`; `view.cpp:306`) → 20 B + Uint16, chunked builds. *[7.6]*
- [ ] **M** Car drawing needs a full Vehicle; single-instance uniforms (`view.cpp:283-300,344,371`) → CarVisual + per-view uniform ring. *[7.5]*
- [ ] **M** Steering/pedal latency longer than needed (`controls.cpp:255,283-284,293`) → steer τ ≈ 25 ms, brake jump to 0.4. *[2.4b]*
- [ ] **M** Wheel pedals read 50% until touched; source switching on absolute values (`controls.cpp:217-241`) → per-axis "moved" flag, switch on deltas. *[7.6]*
- [ ] **M** Touch zones ignore safe areas and edge gestures (`controls.h:47`; `controls.cpp:103-112`) → safe-area insets via the mailbox. *[2.1a, 2.4b]*
- [ ] **M** Mesh builders peak at 15 MB of vector capacity (`mesh.h:40-49`) → reserve, strips, chunk by chunk. *[7.6]*
- [ ] **M** Bot traffic ignores cars alongside; passes in braking zones (`world.cpp:222-234`) → lateral clearance, commit side, no passes when braking. *[7.4]*
- [ ] **M** RACING.md:47-48 and DECISIONS "max 30 steps" (`DECISIONS.md:419`; really 24) are false; RACING.md:116-121 describes the broken infinite-mass model → correct now; rewrite netcode section. *[0.1, 7.3]* (0.1 part done: both corrected, caveat added)
- [ ] **M** Monitors: RACING says 3 per rig, the scene has 1 quad (`main.cpp:395-403`) → decide (Q10); new monitor quads in `tools/assets/sim_rig.py` + `bar_scene.cpp`. *[7.5]*
- [ ] **L** `place()` leaves stale state and a different sag (`vehicle.cpp:46-60`) → one `resetState()`. *[2.4b]*
- [ ] **L** Gear presses during a shift are dropped (`vehicle.cpp:176-189`) → shift queue. *[7.1]*
- [ ] **L** `tan()` recomputed in the hot tire path (`vehicle.cpp:95,248,400`) → precompute. *[2.4b]*
- [ ] **L** Safety clamps hide bugs (`vehicle.cpp:438-442`) → count hits in telemetry. *[7.1]*
- [ ] **L** `cast()` samples at the origin; `locate()` repeated ≈ 9× per car-step (`track.cpp:211-222,273-288`) → cache per car per step. *[7.6]*
- [ ] **L** Frame-rate dependent smoothing (`controls.cpp:293`) → `1 - exp(-dt/τ)`. *[2.4b]*
- [ ] **L** Only the first gamepad is read (`gamepad.cpp:13-23`) → all pads by role. *[7.6]*
- [ ] **L** Inputs not tick-stamped (`controls.h:39`) → InputFrame per tick. *[7.1]*
- [ ] **L** Lap-0 dead code; car B uses car A's extents (`world.cpp:315,326,398-414`) → fix. *[2.4a]*
- [ ] **L** Racing line rebuilt every init (`world.cpp:48-62`) → part of TrackData built once. *[2.4b]*
- [ ] **L** AI mixed into `world.cpp` → `app/race/ai.*`. *[7.4]*
- [ ] **L** Degenerate quads and dead code in the car body (`car_model.cpp:102,110,139-140`); no car LOD → fix; LOD1. *[7.5, 7.6]*
- [ ] **L** Race resources never released; other cars silent (`game.cpp:97,125-130,157-158`) → pooled engine voices for the nearest 2-3 cars. *[7.6]*
- [ ] **L** Magic numbers, fragile restart tap (`game.cpp:147,215`) → `spec.maxSteer`, Input taps. *[2.4b]*
- [ ] **L** Car spec duplicated (`vehicle_test.cpp:23-32`; `world.cpp:21-30`) → one source. *[7.1]*

**I. World simulation**
- [ ] **H** Overlapping coplanar counter colliders: the pint at x = -3.7 jiggles ≈ 60 s after load, just under the 0.05 m/s claim threshold (`physics.cpp:167-205`; `bar_scene.cpp:206`) → contact dedupe, non-overlapping segments, drift-based sleep. *[2.3b]*
- [ ] **H** Kinematic remote bodies move by step count, velocity from frame dt; no render interpolation (`shared_world.cpp:212`; `physics.cpp:104-109,149-151`; `main.cpp:847-859`) → kinematic targets per step + interpolation. *[2.3b]*
- [ ] **M** No broadphase; all statics per substep + O(n²) pairs (`physics.cpp:136,167-170`) → static grid. *[2.3b]*
- [ ] **M** Up to 64 substeps in a hitch frame; dropped time not tracked (`physics.h:74-75`) → 4 substeps, ≤ 4 catch-up steps, tick counter. *[2.3b]*
- [ ] **M** No NaN guard; a bad pose would be broadcast (`physics.cpp:23-26`; `shared_world.cpp:62-66`) → finite + bounds checks both ways. *[2.3b]*
- [ ] **L** Held glass grinds into walls (`physics.cpp:42-45,116-133`) → sweep the hold target. *[2.3b]*
- [ ] **L** Impacts collected then discarded every frame (`physics.cpp:348-352`; `main.cpp:846`) → ring buffer, off until sounds use it. *[2.3b]*
- [ ] **M** *(review)* `bar_scene.h` includes `engine/mesh.h`, which includes `<webgpu/webgpu_cpp.h>`, so no native g++ test can include the layout (the planned physics/entity/activity/interaction tests) → CPU part split into `engine/mesh_data.h`. *[2.3b]*
- [ ] **L** Body identity is a vector index, fields written from 3 places → BodyId + modes. *[5.4]*
- [ ] **M** Runtime state stored in the "static" layout (`bar_scene.h:133-134` mirror on/mode) → Layout const + RoomState/local. *[5.3]*
- [ ] **M** Indices as network identities (seat index in kParam `main.cpp:1128`, `m<i>`, `firstModel`) → stable ids. *[4.2]*
- [ ] **L** Glass collision sizes hand-copied from Blender (`props.cpp:64-71`) → exported proxies. *[6.2a]*
- [ ] **L** Dead SceneObject → static box path (`props.cpp:38-46`) → remove. *[6.1]*
- [ ] **L** Material table size unchecked (`materials.cpp:29-45`) → `static_assert`. *[4.1b]*
- [ ] **M** Walk blockers on the curved counter are AABBs of rotated segments: bumpy sliding (`bar_scene.cpp:169,209-218`) → oriented shapes from layout. *[6.2b]*
- [ ] **M** Walking separate from physics: players don't push glasses; thrown glasses fly through people → kinematic player capsules. *[5.4 (optional part)]*
- [ ] **M** All 8 looks built and uploaded at startup; CPU copies kept (≈ 4 MB) (`main.cpp:1363-1368`) → jobs; own look first; free CPU mesh. *[6.5]*
- [ ] **L** Activity poses snap; only emotes fade (`avatar.cpp:382-450`) → 0.25 s crossfade. *[6.5]*
- [ ] **L** `pose()` string lookups + allocation (`avatar.cpp:334-336`) → bone indices, `std::array`. *[6.5]*
- [ ] **L** Hip/eye constants duplicated; Activity/Emote enums are the wire format (`avatar.cpp:108,596`; `avatar.h:13-29`) → rest pose; protocol version. *[3.2a, 6.5]*
- [ ] **M** Skeleton has no inverse-bind matrices (blocks GLB/VRM avatars) (`skeleton.cpp:28,36`) → bind + inverse-bind. *[later, with own-avatar import]*
- [ ] **M** Skinned mesh: no index ranges; Uint32; 2 influences (`skinned_mesh.h:61-67`; `scene.wgsl:102`) → ranges, Uint16, 4-weight path. *[6.5]*
- [ ] **L** Fixed 70° vertical FOV = 36° horizontal in portrait (`camera.h:12`) → horizontal minimum or setting. *[6.7]*
- [ ] **L** Yaw unbounded; no frustum planes (`camera.h:21-24`) → wrap; `frustum()`. *[6.1]*
- [ ] **M** Math helpers duplicated (`physics.cpp:21-26`, `vehicle.cpp:13-14`, `shared_world.cpp:26-31`, `players.cpp:18-23`, `avatar.cpp:457-466`) → one `math.h`. *[2.3a]*
- [ ] **L** `axisAngle` with a zero axis returns a non-unit quaternion (`math.h:33-47`) → identity. *[2.3a]*

**J. Web, accounts, hosting, deploy, tools**
- [x] **C** Token and password theft through `?server=` / `?cloud=`: the saved token is POSTed to any host on page load (`net.js:30-36,96-97,196,298-301`; `account.js:330-331,347-348`) → exact-host allowlist (never a `*.workers.dev` suffix match); never send tokens elsewhere. *[0.1]*
- [ ] **H** Long-lived bearer token in localStorage, never expires, sent 10×/s to the relay (`account.js:303`; `net.js:196`; `account.php:95-109`) → refresh token + 15 min tickets. *[5.1b, 5.1c]*
- [ ] **H** `names.json` rewritten in place; a bad write wipes all usernames and can make a new owner (`account.php:77-83,135-150`) → MySQL with UNIQUE. *[5.1b]*
- [ ] **H** Account data inside the deployed web folder, protected by `.htaccess` only, no backups (`account.php:43`; `server/.htaccess`) → MySQL + backups (Q11). *[5.1b]*
- [ ] **M** Password guessing barely slowed; 6-char minimum (`account.php:134,154-157`) → per-IP/per-user lockout, 8+ chars. *[5.1a, 5.1b]*
- [ ] **M** Lost updates from unlocked read-modify-write (`account.php:164-166,181-232`) → transactions. *[5.1b]*
- [ ] **M** Failed writes destroy user files (`account.php:68-72`) → moot with MySQL. *[5.1b]*
- [ ] **M** CORS `*`, no signup limits (`account.php:29-31,139-150`) → origin allowlist + limits. *[5.1b]*
- [ ] **M** Accounts URL derived from the relay URL (`account.js:330-331`); once 3.4 removes the relay path, log-in, person cards and pictures would break until 5.1 → `config.json accountsUrl` with a fixed fallback. *[3.4a]*
- [ ] **M** Account fetches have no timeout (`account.js:47-55`) → 8 s AbortController. *[5.1b]*
- [ ] **L** Every profile read loads `admins.json` + all of `names.json` (`account.php:77-93`) → role column. *[5.1b]*
- [ ] **L** Avatar responses lack `nosniff` and CORP (`account.php:116-121`) → headers. *[5.1b]*
- [ ] **L** Person cards not cached (`account.js:364-369`) → cache by id + rev. *[5.1b]*
- [ ] **M** *(review)* Migration gaps: pictures live in `accounts/avatars/<id>.jpg` (`account.php:26,211`) and no step moved them; migrated tokens would never expire; cached clients hold `bar3dToken` and use `{a:'me', token}`; no way to apply `schema.sql` (MySQL is reachable only from IONOS PHP); signups between dry run and switch-over would be lost; nobody deletes the JSON later → see 5.1a/5.1b. *[5.1a, 5.1b]*
- [ ] **M** *(review)* Ed25519 keypair "created on first use" races (two first requests each write one); public key pinned by hand; REAUTH could switch identity mid-session → create-if-absent under `flock`, deploy passes `ACCOUNT_PUBKEY`, REAUTH keeps `sub`. *[5.1c]*
- [ ] **L** *(review)* `login_attempts(key, …)`: `KEY` is reserved in MySQL, and PDO-SQLite tests wouldn't catch it → `attempt_key`; reserved-word check in account_test. *[5.1a]*
- [ ] **L** No Content-Security-Policy (`index.html:3-11`) → meta CSP that still allows the local test servers and `https://*.workers.dev`. *[1.8]*
- [x] **M** *(review)* The planned `report.php` stores into the docroot (`.htaccess` only, the weakness 5.1 removes), has no global cap or retention, a public GET, and no CORS for the github.io page → private folder, cap 500, 30-day expiry, no secrets in payloads, allowlisted CORS, `?env=1` IONOS facts for 5.1. *[0.3]*
- [ ] **M** IONOS upload not atomic; a newer push can cancel it halfway (`deploy.yml:16-18,104-106`) → own concurrency group; upload index last / versioned folder. *[8.2]*
- [x] **M** IONOS `mput` list misses `menu.js`, `voice.js`; `mirror --ignore-time` skips same-size edits (`deploy.yml:104,106`) → generated list, no `--ignore-time`. *[0.1]*
- [x] **M** No tests before publishing (`deploy.yml`) → test job. *[0.4]*
- [x] **M** *(review)* The CI smoke depends on SwiftShader WebGPU on GitHub's ubuntu runners, never tried (works here with global Playwright 1.56.1) → pinned Playwright + `--with-deps`, non-blocking until one green run. *[0.4]*
- [ ] **L** *(review)* Sessions can't run Cloudflare/IONOS admin commands (secrets only in Actions; `gh secret list` returns 403 through the proxy) → `cloudflare-check` step and `admin.yml` workflow_dispatch. *[0.4, 3.3]*
- [ ] **M** SFTP host key accepted blindly (`deploy.yml:98`) → pinned key. *[8.2]*
- [ ] **M** Cloudflare not wired into the deploy (no wrangler step, no config) → Cloudflare steps in the existing job + config.json (a separate job can't write into the site build; a job-level `if:` can't read secrets). *[3.3]*
- [ ] **M** *(review)* `npx wrangler@4.42.0` would resolve wrangler's dependencies fresh on every deploy, ignoring `cloud/package-lock.json` → `npm ci --prefix cloud && npx --prefix cloud wrangler deploy`, token only in step env. *[3.3]*
- [ ] **L** `apt-get install` without update; job-wide write permissions; unpinned emsdk clone (`deploy.yml:11-14,46,53,93`) → fix. *[0.4, 8.2]*
- [ ] **L** This repo deploys the whole domain's landing page (`deploy.yml:102-103`) → own repo (optional). *[8.2]*
- [ ] **L** Landing page sends no security headers (`homepage/.htaccess`) → HSTS, nosniff, Referrer-Policy. *[8.2]*
- [ ] **M** GitHub Pages gives no header control (max-age 600, no immutable, no COOP/COEP) → Cloudflare Pages + `_headers`. *[8.1]*
- [ ] **M** Two live game copies (github.io + joshuavelasco.com/bar): different logins, maybe different builds → one canonical URL (Q5). *[3.5]*
- [ ] **M** *(review)* Stopping the upload deletes nothing: `lftp mirror -R` has no `--delete`, so `home/bar/relay.php` (returns 200 today) and `rooms/` would stay live as a second room for cached tabs; dropping `project.json` would hide the bar from the homepage (`homepage/index.php` lists folders with one); `account_test.mjs:31` and `voice_test.mjs:33` copy `relay.php` → one-off remote delete with exclusions, keep `project.json` + redirect, port both tests. *[3.4c, 3.5]*
- [ ] **L** `version.txt` reload becomes dead weight once `index.html` is no-cache → remove. *[8.1]*
- [x] **M** Smoke test passes even if models never finish loading (`smoke.mjs:41-42,89`) → require "Everything loaded" + non-blank image. *[0.4]*
- [x] **M** No performance numbers in any test → `window.bar3dPerf` printed by smoke. *[0.2]*
- [ ] **L** Static server + browser launch copied into 6 scripts (`smoke`, `shots`, `mp_test`, `account_test`, `rhythm_test`, `voice_test`) → `tools/lib/`. *[0.4]* (smoke, shots, check_live ported; the other four when next touched)
- [ ] **L** No security/abuse tests → negative cases in room_test and account_test. *[3.2c, 5.1a]*
- [x] **M** *(review)* "Looks the same" checks have no baseline images or diff tool, and headless frames aren't deterministic (candle flicker, idle animation) → `?still`, `tools/imgdiff.mjs`, standard spots (VIS). *[0.4]*
- [x] **L** `check_live` doesn't compare the sha → `EXPECT_SHA`. *[0.4]*

---

## 4. Execution plan
Strangler pattern: the live game works after every push. New paths are built next to
old ones, switched over, then the old path is deleted in the same or the next step.
Biggest lag wins first (Phases 1-2 need no outside setup), then sync (G1 is met, so the
server side can be built early in the background).

**Execution order** (one entry = one session; this list wins over the numbering):
0.1 → 0.2 → 0.3 → 0.4 → 1.1 → 1.2a → 1.3 → 1.4 → 1.2b → 1.5 → 1.6 → 1.7 → 1.8 →
2.1a → 2.1b → 2.1c → 2.2 → 2.3a → 2.3b → 2.4a → 2.4b → 2.5 →
3.1 → 3.2a → 3.2b → 3.2c → 3.3 → 3.4a → 3.4b → 3.4c → 3.4d → 3.5 →
4.1a → 4.1b → 4.1c → 4.2 → 4.3a → 4.3b → 4.4 →
5.1a → 5.1b → 5.1c → 5.2 → 5.3 → 5.4 → 5.5 → 5.6 → 5.7 → 5.8 → 5.9a → 5.9b → 5.10 → 5.11 →
6.1 → 6.2a → 6.2b → 6.2c → 6.2d → 6.2e → 6.3 → 6.4 → 6.5 → 6.6 → 6.7 → 6.8 →
7.1 → 7.2 → 7.3 → 7.4 → 7.5 → 7.6 → 8.1 → 8.2 → 8.3.
- **3.1, 3.2a-c and 3.3** touch only `cloud/`, `proto/`, `docs/PROTOCOL.md`, `tools/` and CI
  and change nothing the phone runs: they may be pulled forward to any time after 0.4 (for
  example between Phase 1 steps), in their own order. That checks the unknowns early
  (token permissions, workers.dev subdomain, wrangler in CI, free-plan counters).
- **3.4a needs 2.1a-c (mailbox) and 2.2 (lifecycle)**, plus 3.1-3.3.
- **4.1a-c, 4.3a and 4.4** are local refactors with no wire change and may run before
  Phase 3 (4.4 needs 4.3a; if 4.3a runs before 4.2, its claim keys are filled in by 4.2).
  **4.2 and 4.3b change the wire and need 3.4d** (the relay has no version check, so mixed
  builds would misread seats).
- **Phase 5 needs 3.4d live and 4.2 + 4.3a + 4.3b done.** 5.1a may run any time once G2 is
  met (it runs dark).
- Inside Phase 6, phone reports may reorder 6.3-6.7; 6.2a-e stay in order.

**What NOT to change**
- The from-scratch rule, C++20, WebGPU via emdawnwebgpu, callbacks (no Asyncify), no
  threads/COOP/COEP for now (the hitches are not caused by a lack of threads).
- Keep as they are (only the noted fixes): car physics model and its tuning, MIDI
  parser, materials table, pixel font + text renderer, billboards, mesh helpers,
  skeleton FK core, camera, the PBR math, the `createImageBitmap →
  copyExternalImageToTexture → GPU mips` image path, `<audio>` music streaming, HTML
  panels for menus/forms, the invisible song-picker label trick, walk animation driven
  by distance, the first-person-body approach, the relative steering pad, the Blender
  asset pipeline, the game rules (301, rhythm judge windows, drink recipes), the look of
  everything.
- No WebRTC data channels or WebTransport for game state; no accounts in D1 (decided:
  IONOS MySQL); no copyrighted music in the repo or on R2; no new runtime libraries;
  `astcenc` only after the user says yes.
- Don't move or rename files just for tidiness; extract only when a step says so.
- Don't polish the PHP relay; it's retired in 3.5.
- Cloudflare names that hold storage: the DO class `Room`, the binding `ROOMS` and the
  `[[migrations]]` tag `v1` in `cloud/wrangler.toml` (renaming needs a DO migration and can
  lose storage; add new migration tags, never edit old ones).
- The `account.php` URL path (`https://joshuavelasco.com/bar/account.php`: the Worker's
  `ACCOUNT_URL`, `config.json` and cached old clients use it); IONOS `home/bar/accounts/`,
  the reports folder and the `.htaccess` RedirectMatch rules; `web/project.json` (the
  homepage lists the bar only because of it).
- The `#build` label and `version.txt` until 8.1 (the user and check_live rely on them);
  test URL params (`skipintro`, `cam`, `sit`, `rhythm`, `autoplay`, `darts`, `shift`,
  `pianosong`, `emote`) and tool env vars (`CAM`, `VIEW`, `HOLD`); localStorage keys
  (`bar3dToken`, `bar3dSettings`, `bar3dName`, `bar3dProfile`; change only with a migration).
- `CLAUDE.md` is kept current, not left behind: its Layout ("`server/` is the multiplayer
  relay") and Build-and-test sections must be updated by the step that changes them
  (`cloud/`, `proto/`, `app/activities/`, `tools/lib/`, room_test, imgdiff).

**Data migration summary:** accounts JSON → MySQL: backup first, dry run in 5.1a,
idempotent re-run at the 5.1b switch-over (catches signups since the dry run); token
hashes kept as sessions expiring 30 days after import, and old clients' `bar3dToken`
accepted for 30 days (nobody is logged out); profile pictures move to the private folder
with their rev numbers (5.1b); the JSON files are deleted after 30 days (dated
`PROGRESS.md` item). Relay room files are not migrated (glasses go back to their layout
spots once, at 3.4d). Room DO storage carries a `schemaVersion` and is wiped to layout
defaults when it changes (3.2a; bumps in 4.2, 5.3, 5.4, 5.11). localStorage settings keep
their keys, validated against a schema with defaults (2.1c). A new origin starts with
empty localStorage (3.5 for joshuavelasco.com/bar users, 8.1 for everyone): log in and set
settings again. Your own songs and MIDI files stay local.

### Phase 0: Safety net and eyes on the phone (no outside setup)

#### 0.1 Repo hygiene, honest docs, close the token leak
Done in `de05ef9` (2026-10-17).
- **Files:** `.gitignore`, `cloud/.wrangler/` (remove from git), `web/net.js`, `web/account.js`,
  `docs/RACING.md`, `docs/DECISIONS.md`, `.github/workflows/deploy.yml`.
- **Build:** `git rm -r --cached cloud/.wrangler` + `.wrangler/` in `.gitignore`. In `net.js`
  and `account.js`: the defaults (`DEFAULT_SERVER`, the same-origin relative `relay.php` on
  the IONOS copy (`net.js:33`), `cloud.json`) are always used. An override (`?server=`,
  `?cloud=`) is honoured only if **its target host** is `localhost`/`127.0.0.1` or exactly on
  the allowlist, whatever host the page itself is on. The allowlist holds exact hosts, never
  a suffix: `joshuavelasco.com` now; the deployed `bar3d.<account-subdomain>.workers.dev`
  is added in 3.3 (hard-coded, or read from same-origin `config.json`). Anyone can create a
  `*.workers.dev` subdomain, so a suffix match would still leak tokens (1.8's CSP entry for
  `*.workers.dev` is defence in depth, not the gate). Until 3.3, a `?cloud=` override works
  only for localhost targets. A token or password is attached only to same-origin or
  allowlisted hosts. Anything else: log "ignored server option" and ignore the param.
  Resolve hosts with `new URL(value, location.href).host`, so the relative default keeps
  working on joshuavelasco.com/bar. Fix RACING.md:47-48 ("same at 30 or 120 fps" is false
  until 2.4a) and the DECISIONS racing line (`DECISIONS.md:419` "max 30 steps" is really
  24 and frame-rate dependent). Update DECISIONS "Hosting split": accounts stay on IONOS
  (MySQL); D1 holds game data; add a short "Restructure (2026-10-16)" section pointing here
  with the invariants of §1.3. Generate the IONOS `mput` list from `build/site/*.js` instead
  of the hand list, and drop `--ignore-time` from `mirror` (it skips same-size edits).
  Update `PROGRESS.md` line 14 and any doc that still says to ask for Q1: Q1 is answered
  (2026-10-17).
- **Accept:** STD; `git ls-files cloud/.wrangler | wc -l` → 0 and
  `git check-ignore -q cloud/.wrangler/x` succeeds (`cloud/wrangler.toml` stays tracked);
  `node tools/shots.mjs` with `?server=https://evil.example/relay.php` (extend `shots.mjs`
  to log requests) → no request to `evil.example`, log line "ignored server option"; with
  `?server=http://127.0.0.1:8090/relay.php` the override is honoured.
- **Phone:** build code changed; everything behaves as before; multiplayer still connects
  (check on both github.io and joshuavelasco.com/bar).

#### 0.2 Frame profiler and `?perf` overlay
Done 2026-10-17 (see PROGRESS log). `window.bar3dPerf` is a getter (computed when read),
not refreshed once a second: building it cost ~5 ms headless, a hitch every second.
First numbers (headless, at the door): 527 draws / 649k tris main and 525 draws / 649k tris
mirror, so the mirror pass runs at spawn; budget is ≤ 150 / ≤ 60 (Phase 6).
- **Files:** new `engine/profiler.{h,cpp}`; `app/main.cpp` (scopes around every phase of
  `frame()` as it is today, plus startup stages); `engine/gpu_upload.cpp` (jobs run, pending,
  max job ms); `app/renderer.cpp` (draw + triangle counters per pass); `web/menu.js` (Settings
  toggle "Performance overlay"); `tools/smoke.mjs` (print `window.bar3dPerf`).
- **Build:** `Profiler` with a fixed phase enum, RAII `Scope`, ring of 300 frames {per-phase ms,
  total, draws, tris, jobs pending, heap MB}; stats avg/p95/worst; hitch log (frames > 20 ms
  with the phase that overran); startup stage times (gpu ready, scene built, textures,
  pipelines, each model file, everything loaded, entered, worst frame in first 10 s after enter).
  Overlay drawn with the existing text renderer: 120-frame bar graph + top 3 phases + counts.
  `window.bar3dPerf` updated once per second (not per frame).
- **Accept:** STD; smoke prints `perf: avg … p95 … worst … hitches … stages {…}`;
  `node tools/shots.mjs` with `?perf=1` shows the overlay; profiler self-cost ≤ 0.05 ms (its own row).
- **Phone:** Menu → Settings → Performance overlay ON (Low Power Mode off). At the door the
  graph is mostly green; at the big mirror note the ms number; screenshot both. Fail = no overlay.

#### 0.3 Logs and reports that leave the phone
Done 2026-10-17. Differences from the text below: the log ring and the report payload live
in `web/index.html` (`bar3dLog`, `bar3dReport`), because every log line (C++ and the JS
bridges) ends there; C++ contributes `window.bar3dPerf`. Categories are the leading
"Word:" of a line. Rate limiting is in the page (3 per second per message, digits ignored,
then "(xN more in 1 s)"). Hitches also record `touches` (taps in that frame).
- **Files:** `engine/log.{h,cpp}` (levels, timestamps, categories, ring of 300, rate limit:
  first 3 repeats then "×N" per second); `web/index.html` (ring + rAF-batched DOM, red error
  badge, panel opens only on fatal; fatal overlay "Something broke - tap to rejoin" keeping
  `?room`); `engine/gpu.cpp` (dedupe uncaptured errors; device lost → fatal); `web/audio.js`
  (non-fatal audio problems at warn level); new `server/report.php` (POST ≤ 256 KB JSON →
  `<6-char code>.json` in a **private folder outside the docroot** (the same kind of folder
  5.1 uses for keys; the docroot + `.htaccess` is the weakness 5.1 removes), returns the
  code; GET `?code=` returns it; per-IP limit 10/h; global cap 500 files (oldest deleted);
  files older than 30 days deleted on each POST (IONOS cron may not exist on this plan,
  Q11); rejects payloads containing a 48-hex string (a token); `Access-Control-Allow-Origin`
  for `https://barcode162.github.io` and `https://joshuavelasco.com` so the github.io page can
  read the code; `?env=1` returns the PHP version, whether sodium and pdo_mysql exist and
  whether the private folder is writable, so 5.1's assumptions are checked now);
  `web/menu.js` (Settings → "Send report" shows the code); C++ export of the payload (log
  ring, profiler stats + hitch log, build, UA, DPR, settings, room, player count; the GET is
  public, so the client strips tokens, names and account fields); automatic send on fatal;
  `deploy.yml` uploads `report.php` and creates the private folder.
- **Accept:** STD; `php -S localhost:8090 -t server &` then shots with
  `?server=http://localhost:8090/relay.php` and a page-evaluate of the report command → a code;
  `curl -s 'localhost:8090/report.php?code=XXXXXX'` returns JSON with perf stats and no
  token or name; a POST containing a 48-hex string → rejected; `?env=1` answers; a forced
  error (temporary test hook `?testerror`) shows the badge and doesn't open the panel. After
  deploy: `curl -s 'https://joshuavelasco.com/bar/report.php?env=1'`, and record the PHP
  version, sodium and pdo_mysql in DECISIONS (input for 5.1 and R8).
- **Phone:** Menu → Settings → Send report → a code like `K7Q2XD`; paste it in chat. Claude
  runs `curl -s 'https://joshuavelasco.com/bar/report.php?code=K7Q2XD'`. Ask for 3 reports
  (G4 baseline): right after "Tap to enter", at the big mirror, after 2 minutes of walking.

#### 0.4 Tests gate every deploy
Done 2026-10-17. Tests run as steps of the one deploy job (a failed step stops it before
publishing) instead of a separate job. `mp_test`, `account_test`, `rhythm_test` and
`voice_test` still have their own server/browser code (port them when next touched).
The native tests got basic pass/fail checks (they only printed before).
- **Files:** `.github/workflows/deploy.yml` (job `test` between build and publish: native tests with
  g++ and em++ + node, `php -l server/*.php`, Playwright chromium smoke on the Release build
  (pin `playwright@1.56.1`, the version that works here, and run
  `npx playwright install --with-deps chromium`); `apt-get update` first; emsdk pinned to the
  6.0.12 tag. SwiftShader WebGPU on GitHub's ubuntu runners is untried: the smoke part starts
  as `continue-on-error: true` and becomes blocking after one green run; if WebGPU won't
  start there, keep native tests + `php -l` blocking, smoke non-blocking, and record the
  outcome in DECISIONS. Plus a **`cloudflare-check`** step that prints only "Cloudflare
  secrets: set/missing", runs `npm ci --prefix cloud && npx --prefix cloud wrangler whoami`
  with the token only in that step's env, and calls
  `GET /client/v4/accounts/$CLOUDFLARE_ACCOUNT_ID/workers/subdomain` to print whether a
  workers.dev subdomain exists); `tools/smoke.mjs` (pass = no errors + "Everything loaded"
  within 120 s + screenshot pixel variance above a floor); `tools/check_live.mjs`
  (`EXPECT_SHA` → exit 1 on mismatch); new `tools/lib/site.mjs`, `tools/lib/browser.mjs` (port
  `smoke` and `shots` to them); new `tools/imgdiff.mjs` (PNG decode with `node:zlib`,
  per-pixel threshold, prints the % of differing pixels, exit 1 above a limit); a `?still`
  test hook (fixed time, no idle animation, no candle flicker; a temporary regex in
  `app/main.cpp`, moved into TestHooks in 1.8); new `tools/lib/spots.mjs` (the standard CAM
  spots; write the chosen strings into §How to use → VIS); `CMakeLists.txt` (single staging
  step; per-file gzip commands); a lint script `tools/lint_emasm.mjs` (flags unquoted
  property access in EM_ASM/EM_JS bodies).
- **Accept:** all commands pass locally; two `?still` shots of the same build at the same spot
  differ by ≤ 0.1% in imgdiff; after push the Actions run shows `test` before publish (green,
  or smoke marked non-blocking with the reason in DECISIONS);
  `gh run view <id> --log | grep -i cloudflare` shows "set", a wrangler account and whether
  the subdomain exists (if not, ask the user to open Cloudflare → Workers & Pages once, which
  registers one); `EXPECT_SHA=$(git rev-parse --short HEAD) NODE_PATH=$(npm root -g) node tools/check_live.mjs` passes.
- **Phone:** nothing changes.

### Phase 1: Kill the start-up lag (local only)

#### 1.1 Stop invisible and useless work
Done 2026-10-17: 3 texture layers (avatar fabric, denim, hair; the room ones map to no
texture), sRGB table, models requested before textures, loading screen and 4K Mania draw
only the text layer (one warm-up frame of the full bar once loaded), the room is joined
and voice starts on enter (one new peer per 400 ms). Headless: textures 1250 → 300-530 ms,
everything loaded 9.5 → 5.2 s, heap 84 → 48 MB, 4K Mania 1 draw. The 5 standard spots are
pixel-identical.
- **Files:** `app/bar_scene.cpp` (`buildBarTextures`: only fabric, denim, hair), `engine/texture.cpp`
  (sRGB LUT instead of `pow`, no layer copies), `app/renderer.cpp` (start `loadModels` before
  generating textures), `app/main.cpp` (until entered: clear only, except the one warm-up frame;
  rhythm open → overlay only, no scene/mirror/avatars; `others.start()`/`shared.start()` moved
  to `app_enter`), `web/voice.js` (start after enter, one peer every 400 ms).
- **Accept:** STD; smoke log "Procedural textures: 3 layers in X ms" (was 8 layers, ≈ 410 ms);
  profiler stage "everything loaded" earlier than the 0.3 baseline; with `?rhythm=0,1&autoplay`
  the profiler shows ≤ 10 draws per frame; `tools/mp_test.mjs` still passes (players appear
  after they enter).
- **Phone:** loading bar starts moving sooner; phone stays cooler on the loading screen; in
  4K Mania the overlay's ms number is lower; your brother doesn't appear until he taps enter.

#### 1.2a Cheaper synth and the stale piano song
Done 2026-10-17. Middle C 24 → 1.1-1.7 ms native, 30.6 → 2.9 ms wasm; 30 keys 394 → 36-49 ms
(same sound energy). How: the 2-3 strings are one carrier times a slow "beat" factor updated
every 64 samples, both decay stages in one pass, -80 dB cutoff, 4 partials side by side. Bass
notes now last up to 6 s instead of a looped tail. Joiners check a song's end on the server
clock ("Piano: song already ended", 0 keys made); the owner clears `pso` when it ends; playing
your own .mid clears `pso` for the others. `synth_bench` runs in `tools/native/run.sh`.
- **Files:** `engine/synth.cpp` (one pass per partial with both decay stages, stop at -80 dB,
  detuned strings in the same loop, phasor engine hum, looping bass tail), `app/piano.{h,cpp}`
  (a joiner computes the song's end from the parsed MIDI and ignores songs that already
  ended; the owner clears `pso` at the end, after checking that the current build handles
  an empty `pso` safely (if not, leave it set); `nextNote_` found by binary search; delete
  `busy()`), new `tools/native/synth_bench.cpp`.
- **Build:** keep the `pso` wire format unchanged: old and new builds share the relay until
  3.4d and the relay has no version check. Sample preparation, per-player voices and the
  pedal are 1.2b (they need 1.4's job lanes).
- **Accept:** `g++ -std=c++20 -O2 -I. tools/native/synth_bench.cpp engine/synth.cpp -o build/synth_bench && build/synth_bench`
  → middle C ≤ 2.5 ms (was 15.7-20 ms native, 30.6 ms wasm), all 30 sample keys ≤ 60 ms total;
  STD; smoke with `?pianosong=3` → profiler "piano" phase lower than before (log both
  numbers); mp_test variant: page A starts a short song and waits until it ends, then page B
  joins → B logs "piano song already ended" and synthesizes 0 keys; `tools/mp_test.mjs` passes.
- **Phone:** sit at the piano, pick a song: the freeze when it starts is much shorter. Ask
  your brother to start a short song and wait until it ends; then you reload and enter: no
  stutter after entering.

#### 1.3 Async pipelines, warm-up, an honest "ready"
Done 2026-10-17. `engine/pipeline_cache.*`: every pipeline compiles async (13 gated + the 2 race
ones, prepared at start-up); text, pictures and mirror glass got explicit bind group layouts
(they used `GetBindGroupLayout`, which needs the finished pipeline); the bar draws only the
text layer until its 9 pipelines exist; texture jobs wait for the 2 mip pipelines. Warm-up is
the one full frame behind the loading screen (from 1.1, now after jobs and pipelines are done)
instead of a separate 64×64 offscreen render, then `OnSubmittedWorkDone`, then 30 steady
frames or 2 s. Headless: pipelines ready in ~550 ms, "ready" ~1.5 s after the last file.
- **Files:** new `engine/pipeline_cache.{h,cpp}` (`CreateRenderPipelineAsync` with
  `CallbackMode::AllowSpontaneous`, pending count, failure log); `app/renderer.cpp`,
  `engine/text.cpp`, `engine/billboard.cpp`, `engine/gpu_upload.cpp` (both mip pipelines at
  startup instead of lazily per format mid-load), `app/race/view.cpp` (its 2 pipelines compiled
  async during loading, but not part of the warm-up gate); `app/main.cpp` (ready = files done +
  jobs 0 + bar pipelines 0 pending + warm-up frame done via `queue.OnSubmittedWorkDone` + 30
  frames < 20 ms or 2 s); `web/index.html` ("Warming up…" stage text).
- **Build:** today 11 pipelines are created synchronously at startup (renderer ×9, text,
  billboard), the mip pipeline(s) lazily mid-load and the 2 race pipelines on the first sit
  (14 call sites). Draws whose pipeline isn't ready are skipped. Warm-up renders main, full
  mirror and avatars-only mirror into a 64×64 offscreen target, touching every material. The
  race pipelines compile in the background (cheap, no draw); the track is built after enter
  in 2.4b.
- **Accept:** `grep -rn "CreateRenderPipeline(" app engine` → no matches; smoke log
  "Pipelines ready in X ms (N)", "Warm-up done" and "Race pipelines ready"; STD.
- **Phone:** after "Tap to enter", turn straight to the big mirror: no hitch the first time.
  Open 4K Mania for the first time: no hitch.

#### 1.4 Job scheduler and sliced model loading
Done 2026-10-17. `engine/jobs.*` (lanes, kinds with learned cost, cancel tokens, a gate for the
mip pipelines), run after submit. Models: parse begin / one job per mesh part / materials /
one per image copy / materials+bind groups / buffer slices of 512 KB / done; shared 1×1
fallbacks; ≤ 4 image decodes in flight; bitmaps always dropped; one submit per image job.
Downloads retry 3× (0.5, 1.5, 4 s). "N parts failed - Retry" (reload) on the loading screen,
still enters unless `hall` failed. Smoke: `FAIL_MODEL=candle` check (in CI). Headless: max job
7-12 ms (single GPU calls, SwiftShader shares the CPU), slowest loading frame 12-25 ms.
- **Files:** `engine/gpu_upload.{h,cpp}` → `engine/jobs.{h,cpp}` (priority lanes, cancel tokens,
  per-kind cost estimate, run while `now + estimate < deadline`; always drop bitmaps; batch
  mipgen per job); `engine/model.cpp` (parse in a job, sliced per mesh; `finish()` = one job per
  ≤ 1 MB buffer slice / per material; shared 1×1 fallback textures; ≤ 4 image decodes in flight);
  `engine/fetch.cpp` (3 retries with backoff, failure reported); `app/renderer.cpp` (failed count;
  critical files: hall + colliders' models); `web/index.html` (`barLoading.fail(msg)` + Retry);
  `app/main.cpp` (jobs after submit with a deadline).
- **Accept:** STD; smoke prints `max job ms` (≤ 8 ms headless, i.e. well under 3 ms on the
  phone) and no loading frame over 100 ms of CPU; a smoke run that aborts one model
  (`page.route`) shows "1 part failed - Retry" and still enters.
- **Phone:** loading bar moves smoothly with no long pauses; walking right after entering is
  smooth while the last textures arrive.

#### 1.2b Piano samples in background jobs, per-player voices, shared pedal (runs after 1.4)
Done 2026-10-17. 30 samples (key 3s+1), neighbours by playbackRate; background-lane jobs from
the middle out after enter; song notes without a ready sample are skipped; a live key makes
its sample at once if needed. Per-player `Hands` (voices, pedal) released after 10 s quiet.
`setPedal(down, share)` for the on-screen and MIDI pedal. Not done: the headless pedal
mp_test variant (checked by reading the code; no Web MIDI on iOS anyway).
- **Files:** `app/piano.{h,cpp}` (samples for every 3rd key, neighbours by `playbackRate`;
  prepared by background-lane jobs (1.4) after enter, one key per job; the scheduler skips
  notes whose sample isn't ready; per-player voice tables + pedal, released on leave or 10 s
  silence; MIDI pedal shared through one `setPedal(down, share)`), `engine/audio.*` +
  `web/audio.js` (only `playbackRate` on `soundPlay`; the emitter id and the voice cap belong
  to 2.5, which would otherwise redo them).
- **Accept:** STD; smoke with `?pianosong=3` → profiler "piano" phase never above 1 ms and
  every piano-sample job ≤ 2 ms headless-scaled (`max job ms`); `tools/mp_test.mjs` passes;
  headless pedal check: an mp_test variant where page A sends pedal-down (through the same
  call the MIDI pedal uses) and then a note, and page B logs the note as sustained until A's
  pedal-up. (There's no Web MIDI on iOS, so the pedal can't be checked on the phone.)
- **Phone:** sit at the piano, pick a song: no freeze at all when it starts. Your brother
  starts a song, you reload and enter: no stutter after entering.

#### 1.5 Memory: initial heap, fewer copies
Done 2026-10-17. `INITIAL_MEMORY=80MB` (headless peak 76 MB while loading), `STACK_SIZE=1MB`,
`-Werror` on `engine/`; downloads land directly in the vector C++ hands on (no copy); the
profiler counts heap growth after start ("grew 0 times").
- **Files:** `CMakeLists.txt` (`-sINITIAL_MEMORY=<peak from the phone report, rounded up to 16 MB>`,
  `-sSTACK_SIZE=1MB`, `-Werror` for `engine/`), `engine/net.cpp` (first fix the one warning
  `-Werror` would turn into an error: line 68 "empty character constant
  [-Winvalid-pp-token]" inside an EM_ASM body; use `""` or move it into a JS library
  function), `engine/fetch.cpp` (decompress straight into a buffer C++ owns, no extra
  `std::vector` copy), profiler reports heap growth count.
- **Accept:** a clean rebuild with `-Werror` passes (`rm -rf build && emcmake cmake -B build -G Ninja && cmake --build build`);
  STD; smoke log "heap grew 0 times after start"; heap MB in the report.
- **Phone:** nothing visible; send one report after entering (heap number).

#### 1.6 Texture budget at export (no new tools)
Done 2026-10-17 differently (the user asked not to change the models): no re-export. At load,
identical images in different models are decoded and uploaded once (237 → 159), roughness and
occlusion maps are scaled to ≤ 256 and everything else to ≤ 1024 (createImageBitmap resize).
Log "Textures: 159 images on the GPU, 132 MB" (the plan's estimate for before: 231.6). Download
size unchanged; the export caps can still be done when models are rebuilt. Spots: ≤ 0.04% of
pixels differ (threshold 4).
- **Files:** `tools/assets/common.py` (max size per role: 1024 floor/brick/counter top, 512
  default, 256 props), rebuilt `assets/models/*.glb` via `bash tools/assets/build_all.sh <scratch>`,
  `engine/gpu_upload.cpp` (`createImageBitmap` resize cap 1024 as a safety net).
- **Accept:** `python3 tools/assets/glb_report.py` → total texture MB ≤ 70 (was 231.6), download
  size lower; VIS before/after at the standard spots (bar top, back bar and a floor close-up
  added for this step) compared side by side: as sharp at normal distance (imgdiff number
  logged; small differences are expected here).
- **Phone:** loads faster; look closely at the bar top, the floor and the bottles: still sharp
  enough? Report anything blurry.

#### 1.7 Cheap GPU wins: no discard, half-resolution mirror, tight scissor
Done 2026-10-17. No `discard` left in `scene.wgsl`: mirror views use an oblique near plane
(`obliqueNearPlane` in renderer.cpp, Lengyel, depth 0..1, computed on the full world→clip
matrix). Reflection + its own depth buffer at half resolution, sampled with a linear sampler
(`params.yz` = 1/screen). Scissor from the glass quad clipped against the near plane (a mirror
behind you is skipped). Mirror spot 4.87% differing pixels (softer), others 0%; grazing views
checked, no leaks. Not done: per-viewer mirror mode (Q3, not asked).
- **Files:** `shaders/scene.wgsl` (main-view entry points without the clip test, via an override
  constant), `app/renderer.cpp` (mirror: oblique near-plane projection so the mirror view needs no
  `discard`; half-resolution target; tight scissor from the glass quad clipped against the near
  plane; skip when the glass is outside the frustum; anisotropy 4), `shaders/mirror.wgsl`
  (projective UVs + linear sampler), settings: mirror mode per viewer if the user agreed (Q3).
- **Accept:** STD; VIS: mirror-front spot in both modes ≤ 5% differing pixels (the mirror is
  allowed to get softer), every other standard spot ≤ 1%; no wall or frame leaking into the
  reflection; no gaps at the edges.
- **Phone:** stand in front of the big mirror on full mode: the overlay ms number should be lower
  than the baseline; the reflection may look a little softer. Fail = things behind the wall
  appear in the mirror.

#### 1.8 Loader state machine, page shell, test hooks
Done 2026-10-17. `app/loader.*` (stages models → finishing → shaders → warm-up → steady →
ready/failed, named on the loading screen, profiler stages `load:*`), `app/test_hooks.*` (all
C++ `?` options parsed once, applied at Ready). Page: version check before the scripts load
(1.5 s timeout), wasm preload, progress bar via `scaleX`, overlay removed on `transitionend`,
HUD buttons hidden until enter, CSP meta (with localhost for tests), CSP violations logged as
errors (so smoke/mp_test fail on them).
- **Files:** new `app/loader.{h,cpp}` (stages of §2.7, progress + names to `barLoading.stage()`),
  new `app/test_hooks.{h,cpp}` (all `?` params parsed once into a struct; applied as normal
  commands after Ready), `app/main.cpp`, `web/index.html` (preloads; version check before injecting
  `app.js`; progress via `transform: scaleX`; `will-change: opacity`; remove overlay on
  `transitionend`; HUD buttons hidden until enter; meta CSP: `connect-src 'self'
  https://joshuavelasco.com https://*.workers.dev wss://*.workers.dev`, `img-src 'self'
  https://joshuavelasco.com data: blob:`. The local tests need more: mp_test/voice_test use
  `http://127.0.0.1:<port>`, 0.3 uses `http://localhost:8090`, 3.x uses `ws://localhost:8787`.
  Either inject the CSP only in the CI build (`sed` in deploy.yml) or add
  `http://localhost:* http://127.0.0.1:* ws://localhost:* ws://127.0.0.1:*`. The CSP is
  defence in depth; the token gate is 0.1's exact-host allowlist); TestHooks absorbs `?still`
  (0.4) and later `?net=off` (3.4a).
- **Accept:** STD; smoke log lists every stage with times; `?sit=30`, `?rhythm=0,1&autoplay`,
  `?darts`, `?shift`, `?pianosong=2`, `?cam=…`, `?still` still work (shots); smoke and mp_test
  log no CSP violations (listen for `securitypolicyviolation` and print each one).
- **Phone:** loading screen shows stage names; note the time to "Tap to enter" (the report has
  it); everything else as before.

### Phase 2: Engine plumbing (local)

#### 2.1a One mailbox: bridge, canvas size, input
- **Files:** new `engine/bridge.{h,cpp}`, new `web/bridge.js` (+ script tag); `engine/gpu.cpp`
  (size from the mailbox, applied after 120 ms stable; reconfigure on Outdated/Lost);
  `engine/input.cpp`/`.h` (no layout reads; event timestamps (check `timestamp` in
  `EmscriptenTouchEvent` in 6.0.12's `html5.h`, else a small JS listener); stick visuals through
  the flush; clear on blur/visibility/pagehide; `Input::poll()` → `InputFrame`; safe-area
  insets in the mailbox header; focus flag instead of the per-key `activeElement` EM_ASM).
- **Accept:** STD; smoke prints JS crossings per frame and "bridge" phase ≤ 0.3 ms; shots at
  `VIEW=932x430` then `430x932` → one resize each; `HOLD=ArrowUp:3000` still walks.
- **Phone:** walk with the left thumb for 10 s: smooth. Rotate the phone: picture re-fits after a
  moment, no black flash.

#### 2.1b UI commands, a JSON writer, one modal manager
- **Files:** `engine/ui.cpp` + `web/menu.js` (commands pushed into the mailbox ring; settings
  change flag; UI state sent only on change; catalog once; look via command only; mic button
  updates on change; dead bridge code removed); `engine/json.{h,cpp}` (not audited before:
  the escaping JSON writer lives here; parse without a `shared_ptr` map per value; reject
  depth > 32 and strings > 4 KB, because from 3.4 it parses untrusted network JSON such as
  names in PROFILE); one modal manager for menu + account panels with the `modalOpen` flag,
  no backdrop blur (`web/menu.js`, `web/account.js`).
- **Accept:** STD; menus, MIDI file add (file name with a backslash) and emotes still work
  (shots); new `tools/native/json_test.cpp`: writer output parses back equal (backslash,
  quotes, control characters, emoji) and 1,000 random garbage inputs don't crash.
- **Phone:** open the menu and the account panel: they work, no blur lag; pick an emote:
  it plays.

#### 2.1c Voice batch, log flush, settings schema
- **Files:** `engine/log.cpp` (lines out through the flush); `app/main.cpp` (voice levels read
  from the mailbox; voice positions sent as one batch at 20 Hz; per-player `present()`
  replaced by a roster snapshot in the mailbox), `web/voice.js`; settings schema with
  defaults (localStorage keys unchanged).
- **Accept:** STD; `grep -c "EM_ASM" app/main.cpp` ≤ 2; smoke prints JS crossings per frame ≤ 4;
  `tools/voice_test.mjs` passes; a corrupted `bar3dSettings` value → defaults + one warning.
- **Phone:** voice with your brother still works and comes from his direction; your
  settings are still there after a reload.

#### 2.2 FrameTime from rAF and the page lifecycle
- **Files:** new `engine/frame_time.h`; `app/main.cpp` (`emscripten_request_animation_frame_loop`;
  FrameTime passed everywhere; remove the stray `emscripten_get_now()` calls); `web/bridge.js`
  (visibility); `web/net.js` (on hidden: send an "away" flag, stop sending state; on visible:
  resync); `web/audio.js` (suspend/resume; "Tap to resume sound" when interrupted);
  `engine/jobs.cpp` (paused while hidden).
- **Accept:** `grep -n "emscripten_get_now" app/*.cpp app/race/*.cpp` → none (only in engine
  frame_time/profiler; the race build timing at `race/game.cpp:37,40` uses the profiler);
  mp_test variant: page B fires `visibilitychange` (hidden) → A shows
  "away" over B within 1 s; STD.
- **Phone:** switch to another app for 10 s and back: no fast-forward jump, sound comes back.
  Your brother sees "away" over you while you're gone.

#### 2.3a One `math.h`
- **Files:** `engine/math.h` (quat conjugate/normalize/nlerp/slerp, angle wrap/lerp, Vec3 ops,
  AABB, `isFinite`; zero-axis fix for `axisAngle`); delete the copies in `engine/physics.cpp`,
  `engine/vehicle.cpp`, `app/shared_world.cpp`, `app/players.cpp`, `app/avatar.cpp`; new
  `tools/native/math_test.cpp`.
- **Accept:** math_test (g++) passes; vehicle_test targets unchanged (0-100 ≈ 4.2 s, braking
  ≈ 31 m); race_test output identical to before the change; STD.
- **Phone:** nothing changes.

#### 2.3b Physics: no jiggle, no judder, no NaN
- **Files:** `engine/physics.{h,cpp}` (per-sphere contact dedupe by normal; drift-based sleep;
  prev/cur pose + `alpha`; kinematic target per step; `isfinite` + room-bounds guard → reset to
  last good pose; 4 substeps; ≤ 4 catch-up steps; tick counter; static uniform grid broadphase;
  hold target swept against statics; impacts ring buffer off by default), `app/bar_scene.cpp`
  (non-overlapping counter-top segments), `app/shared_world.cpp` (targets instead of velocity
  hack), `app/main.cpp` (render interpolated transforms), new `engine/mesh_data.h` (the CPU
  part of `engine/mesh.h`: `MeshData` and the `make*` builders, no webgpu include; `mesh.h`
  includes it and `bar_scene.h` includes only it), new `tools/native/physics_test.cpp`.
- **Why mesh_data.h:** `bar_scene.h:8` includes `engine/mesh.h`, whose line 3 includes
  `<webgpu/webgpu_cpp.h>`, so g++ stops with "No such file". Adding files doesn't fix a
  missing header. The native tests of 4.2, 4.3a/4.3b and 4.4 need the same split. If the split
  turns out impossible, build these tests with em++ and run them in node (like 2.4a's variant).
- **Accept:** `g++ -std=c++20 -O2 -I. tools/native/physics_test.cpp engine/physics.cpp app/props.cpp app/bar_scene.cpp app/materials.cpp engine/procedural.cpp -o build/physics_test && build/physics_test`
  (add .cpp files until it links) → "all 10 glasses asleep by t ≤ 2 s", "no NaN after 10k random
  impulses", worst step time; STD; `tools/mp_test.mjs` glass scenario passes.
- **Phone:** the pint near the left end of the bar no longer wobbles after loading; throw a
  glass: smooth flight, no stutter.

#### 2.4a Race: tick clock, interpolation, frame-rate independent results
- **Files:** `app/race/world.{h,cpp}` (uint32 tick, goTick, lap/finish in ticks; never reads
  `nowMs`; per-car extents; lap-0 cleanup), `app/race/game.{h,cpp}` (prev/cur pose per car,
  render lerp; HUD/lights from ticks; pause on hidden), `tools/native/race_test.cpp` (same
  race at 30/57.3/60/120 fps must give identical results, exit 1 otherwise).
- **Accept:** native race_test (command at its top) prints "frame-rate independent: OK"
  (today 30/60/120 fps give different winners and times); same test built with `em++ -O3`
  and run in node gives OK too; vehicle_test targets unchanged (0-100 ≈ 4.2 s, braking
  ≈ 31 m); STD with `?sit=30`.
- **Phone:** sit in a rig: at full speed the car and scenery move smoothly; the lap timer
  matches a stopwatch.

#### 2.4b Race: background build, controls feel, resets
- **Files:** `app/race/world.{h,cpp}` (reset service: free slot, 2 s ghost, 4 s cooldown;
  racing line built once into TrackData), `app/race/game.{h,cpp}` (`world_.init()` +
  `view_.init()` track build (`game.cpp:35-42`) as background jobs after enter; the race
  pipelines already compile in 1.3; "warming up" if you sit early; `spec.maxSteer`; restart
  via taps; the 2 `EM_ASM` calls that toggle the `racing` body class move behind an engine
  call), `app/race/controls.cpp` (exp smoothing, steer τ ≈ 25 ms, brake jumps to 0.4 then
  ramps; safe-area insets from the mailbox), `engine/vehicle.{h,cpp}` (`resetState()` used by
  setup and place; precomputed tire constants), `app/race/track.cpp` (integer-hash smooth
  value noise instead of `sin`).
- **Accept:** STD; smoke `sit=30` logs "Race track ready" before the sit; race_test still OK;
  vehicle_test unchanged; `grep -n "EM_ASM" app/race/*.cpp` → none.
- **Phone:** wait 10 s after entering, sit in a rig: no freeze. Steering feels a bit quicker;
  after a crash, the reset puts you on a free spot.

#### 2.5 AudioMixer: emitters, voice pool, buses, one command list
- **Files:** `engine/audio.{h,cpp}` → AudioMixer (named sound ids, buses, persistent emitters,
  voice cap/steal, one command buffer flushed through the mailbox, small status block back),
  `web/audio.js` (executor; one HRTF panner per emitter, equalpower one-shots; Track on a bus;
  raw sound data dropped once the AudioBuffer exists; unlock listener trimmed; object URLs
  revoked; the piano's emitter id and the global voice cap 32, left out of 1.2b), games use
  sound ids instead of `&g_app.sfx[…]`.
- **Accept:** STD; smoke prints audio commands per frame and active voices; `tools/rhythm_test.mjs`
  passes; piano song headless shows ≤ 32 voices.
- **Phone:** play a fast piano song next to the jukebox: no crackling; volume sliders now also
  change the 4K Mania song; bell, darts and drinks still sound right and come from the right place.

### Phase 3: The authoritative Cloudflare room (G1 met; 3.1-3.3 may run early, see Execution order)

#### 3.1 Protocol spec, codec, golden vectors
- **Files:** new `docs/PROTOCOL.md` (from §2.5: every message, binary layouts, JSON shapes, close
  codes and what the client does on each (no auto-retry on 4001/4002/4005), rates, limits,
  versioning rules (worker accepts protos N-1..N for one deploy), the storage
  `schemaVersion` history), new `proto/vectors.json`, new `cloud/proto.js` (binary codec), new
  `engine/netproto.{h,cpp}` (C++ codec), new `tools/proto_test.mjs`, new
  `tools/native/proto_test.cpp`.
- **Accept:** `node tools/proto_test.mjs` and the native proto_test both pass on the same
  vectors, including garbage cases (truncated frames, wrong type bytes, NaN floats, huge
  lengths, malformed JSON for `engine/json`) that must be rejected without a crash.
- **Phone:** nothing.

#### 3.2a Room DO v2 skeleton: slots, hello, close codes
- **Files:** `cloud/worker.js` (router, Origin allowlist of exact origins, per-IP connect
  limit), new `cloud/room.js` (the DO), `cloud/proto.js`, `cloud/wrangler.toml` (compat date
  bump; `ALLOWED_ORIGINS`, `PROTO`; keep class `Room`, binding `ROOMS`, migration tag `v1`),
  `tools/cloud_test.mjs` → renamed and extended into `tools/room_test.mjs` (update the
  references in `PROGRESS.md` and `CLAUDE.md`), `tools/setup.sh` (`npm ci --prefix cloud` when
  `cloud/` exists, so `wrangler dev` works in a fresh session).
- **Build:** server slots + resume tokens; guest welcome then async token check (2 s
  timeout, 10 min cache) → PROFILE; close codes 4001/4002/4003; protos N-1..N accepted; 5 s
  hello timeout; PING/PONG; `schemaVersion` key (§2.5.11); in-memory slot map; close
  reciprocated; only room `main` persists, other rooms `deleteAll()` when empty.
- **Accept:** `(cd cloud && npx wrangler dev --port 8787) &` then `node tools/room_test.mjs` →
  distinct slots; resume token → same slot; wrong proto → 4002 (N-1 still accepted); 17th
  client → 4001 with reason; a socket without HELLO closed after 5 s; a disallowed Origin
  refused; a non-`main` room leaves no storage after its last socket closes;
  `tools/cloud_test.mjs` no longer exists.
- **Phone:** nothing.

#### 3.2b Ticks, events, signals, legacy KV with echo
- **Files:** `cloud/room.js`, `cloud/proto.js`, `tools/room_test.mjs`.
- **Build:** binary STATE in, batched 20 Hz TICK out while dirty (stops after 2 s idle);
  EVENTS stamped (tick + seq) and fanned out; SIGNAL forwarding; **legacy world KV kept for
  the transition, with a server seq and echo to everyone including the writer**.
- **Accept:** room_test → A and B write the same key at once 50× → identical final state on
  both; A's STATE in B's TICK ≤ 100 ms; events arrive in seq order with server ticks; a
  SIGNAL reaches only its target.
- **Phone:** nothing.

#### 3.2c Alarm scheduler, ghosts, limits, persistence, `/health`
- **Files:** `cloud/room.js`, `cloud/worker.js` (`/health` answered from the Cache API for
  60 s), `tools/room_test.mjs`, `docs/PROTOCOL.md`.
- **Build:** the alarm scheduler (§2.5.11); ghost checks inside message handling + one long
  idle deadline + auto-response keepalive (§2.5.9); AFK close after 10 min; 30 s grace;
  token buckets → 4005; debounced persistence of changed discrete keys only; hibernation
  rebuild (last STATE in the socket attachment); usage counters (messages, alarms, rows
  written, unknown ticket kids) kept in memory and written with the debounced flush; what
  the Worker sees when a daily quota runs out, written into PROTOCOL.md; at 70% of any daily
  quota (from the counters) the room tells clients to send STATE at 10 Hz. Re-check the
  free-plan numbers in §2.5.12 now (including whether the Workers rate-limit binding exists
  on the free plan).
- **Accept:** room_test → kill a socket without close → slot freed ≤ 15 s + grace (timers may
  be shortened by a dev-only var); flood of 500 messages → 4005; ≤ 30 rows written per minute
  under a 10 Hz world-write load (rows-written counter, not writes); a debounced flush and a
  fake "song end" deadline both fire when their times overlap; an idle connected socket causes
  ≤ 1 alarm per 60 s; two `/health` calls within 60 s → one DO request.
- **Phone:** nothing.

#### 3.3 Deploy the room (dark launch) + `config.json`
- **Before:** if 0.4's `cloudflare-check` reported no workers.dev subdomain, ask the user to
  open Cloudflare → Workers & Pages once (that registers one); a non-interactive
  `wrangler deploy` fails until then.
- **Files:** `.github/workflows/deploy.yml` (Cloudflare **steps in the existing
  `build-and-publish` job**, before "Publish to gh-pages", exactly as §2.12: `HAVE_CF` flag,
  token only in the step env, `npm ci --prefix cloud && npx --prefix cloud wrangler deploy`,
  `continue-on-error: true` for the dark launch; write `build/site/config.json {build, proto,
  ws, accountsUrl, reportUrl}` with `ws` from wrangler's printed URL), new
  `.github/workflows/admin.yml` (`workflow_dispatch` with an `action` input; starts with
  `cf-check`; later steps add `d1-create`, `d1-migrate`, `secret-sync`, `pages-create`,
  `r2-create`, `migrate-dry`, `migrate-run`; prints results, never secrets),
  `cloud/wrangler.toml` comment fixed ("Deployed by deploy.yml" becomes true), `web/net.js` +
  `web/account.js` (the exact `bar3d.<subdomain>.workers.dev` host joins the allowlist).
  The old client ignores `config.json` (it fetches only `cloud.json`, `net.js:303`), so
  nothing switches yet.
- **Accept:** Actions run green and its log shows the Cloudflare steps ran (not skipped,
  not failed-but-continued); `curl -s https://bar3d.<subdomain>.workers.dev/health` → ok;
  `NODE_USE_ENV_PROXY=1 ROOM_URL=wss://bar3d.<subdomain>.workers.dev NODE_PATH=$(npm root -g) node tools/room_test.mjs`
  passes against the live worker (Node 22 ignores `HTTPS_PROXY` without that flag);
  `curl -s https://barcode162.github.io/3D/config.json` shows the ws URL;
  `gh workflow run admin.yml -f action=cf-check` runs (if the proxy blocks it, set up the
  marker-file fallback from §How to use and note it in DECISIONS).
- **Phone:** nothing changes (still on the relay).

The cutover is split so the old path stays the default until the last part: v2 runs only
with `?net=v2` until 3.4d flips it.

#### 3.4a Client net v2 behind `?net=v2`
- **Files:** `engine/net.{h,cpp}` v2 (`poll()` → typed messages from the ring; `sendState(block)`
  on the net tick; `sendIntent(kind, args) → reqId`; `sendEvent`; `serverNowMs()`; `slot()`;
  status with reason; compatibility shims `setWorld/receiveWorld` over the legacy KV),
  `web/net.js` v2 (single backend from `config.json`, no relay fallback, binary frames copied
  into the wasm ring, reconnect with resume token (250 ms, 1, 2, 4, max 8 s), HUD status:
  connecting / online / offline / full / new version / server busy; `?netsim=` on localhost;
  the old path stays the default, v2 only with `?net=v2`), `web/account.js` (read `accountsUrl`
  from `config.json`, falling back to `https://joshuavelasco.com/bar/account.php`; stop calling
  `bar3dNet.serverUrl()`, which v2 doesn't have; without this, log-in, person cards and
  pictures over heads disappear until 5.1), test isolation (§2.10: net off on localhost
  unless `?cloud=`; `?net=off`; `tools/check_live.mjs` connects only with `NET=1`; smoke and
  shots pass `?net=off`).
- **Build:** **Echo shim.** The server now echoes your own writes (I-2), but `shared_world`,
  `piano`, `bartender` and `jukebox` assume no echo (`cloud_test` even checked "no echo of your
  own world change"). Track the last-sent seq per key; deliver an own echo only when the
  server's accepted value differs from our latest local write (someone else won); drop echoes
  with seq ≤ ours that match. Without this a thrown glass rewinds by about one round trip
  after it rests (stale `kMoving` echoes set `b.kinematic` and remote mode,
  `shared_world.cpp:106-160`, with up to 300 ms extrapolation), and `jb`/`pso` echoes could
  restart music or piano songs. Each system drops the shim when it's migrated (5.3-5.9).
  **Retries:** none automatic on 4001/4002/4005; on 4002 poll `version.txt`/`config.json` until
  the build changes, then reload once; quota errors → "Server busy, try later", retry every
  5 min.
- **Accept:** STD (default path unchanged); shots with
  `?net=v2&cloud=ws://localhost:8787` against local `wrangler dev` → status "online", a slot
  assigned, the account panel still reaches `account.php` (request logged); mp_test (default
  relay mode) passes; check_live without `NET=1` opens no socket.
- **Phone:** nothing changes (v2 is off by default). Log in still works and your picture
  still shows over your head.

#### 3.4b Remote players on server time
- **Files:** `app/players.{h,cpp}` (samples `{serverMs, block}`, adaptive delay, discrete
  fields at playback, 100 ms extrapolation, stable slots, dimmed while reconnecting; profiles
  cached from PROFILE; spectator data from the same samples), `app/main.cpp` (`isLower` →
  slot order; state sent on the net tick only; race start time from `serverNowMs()` in v2),
  new `tools/native/interp_test.cpp`.
- **Accept:** interp_test (jitter, gaps, extrapolation cap, discrete fields switch at render
  time); two pages with `?net=v2&cloud=ws://localhost:8787&netsim=lat:100,jitter:50` → B's
  view of A is smooth (log the largest position jump per frame); STD.
- **Phone:** nothing changes yet (default is still the relay).

#### 3.4c Voice signaling over the room; tests on `wrangler dev`
- **Files:** `web/voice.js` (signaling through SIGNAL keyed by slot in v2), `tools/mp_test.mjs`
  (CLOUD mode probes the room, not PHP; new assertions below), `tools/voice_test.mjs` (ported
  to `wrangler dev`; today it copies `server/relay.php`, `voice_test.mjs:33`, which 3.5 deletes).
- **Accept:** with local `wrangler dev`, `CLOUD=ws://localhost:8787 NODE_PATH=$(npm root -g) node tools/mp_test.mjs`
  passes all scenarios (presence, glass rest + late join, mirror, jukebox) plus: the
  thrower's glass doesn't move after it rests (no echo rewind); Alice changes the jukebox song
  and her music doesn't restart; Bob reconnects with his resume token → same slot;
  `tools/voice_test.mjs` passes on `wrangler dev`.
- **Phone:** nothing changes.

#### 3.4d Cutover: everyone moves to Cloudflare
- **Files:** `web/net.js` (v2 becomes the default; the relay path stays in the file, unused,
  until 3.5), `.github/workflows/deploy.yml` (Cloudflare steps blocking: drop
  `continue-on-error`).
- **Accept:** room_test (live, `NODE_USE_ENV_PROXY=1`) + mp_test (CLOUD) + voice_test + STD pass;
  check_live (without `NET=1`) passes. Parity list checked headless where possible: names,
  log in, pictures over heads, tapping a person card, movement, emotes, voice, seats,
  glasses, jukebox, mirrors, lights, piano song + notes, darts/rhythm/race screens, drinks,
  badges.
- **Phone (with your brother):** both of you reload first (old and new builds can't meet).
  Top bar says "online"; he moves smoothly and with less delay than before; emotes, voice,
  seats, glasses, jukebox all still work; log in, see your picture over your head, tap your
  brother to open his card; turn on airplane mode for 5 s and off: you come back in the same
  place and he saw you dimmed meanwhile. Glasses start from their normal spots once (old
  relay positions aren't carried over). Fail = "offline" stays, or someone is invisible.

#### 3.5 Retire the relay and the second game copy
- **Files:** delete `server/relay.php` and the relay paths in `web/net.js`/`tools/mp_test.mjs`;
  port `tools/account_test.mjs` to `account.php` alone (it copies `relay.php` today,
  `account_test.mjs:31`); `deploy.yml` stops uploading the relay and the game copy to
  `home/bar/`, but keeps uploading `account.php`, `report.php`, `.htaccess` (with its
  RedirectMatch rules), `project.json` (`homepage/index.php` lists only subfolders that have
  one) and a redirect `index.html` to the canonical URL (Q5); a one-off lftp cleanup step
  (`rm -f home/bar/relay.php; rm -r home/bar/rooms`, plus the old game files listed by name),
  because `mirror -R` without `--delete` never removes remote files and the old relay would
  stay live as a second room for cached tabs; it must **never** touch `home/bar/accounts/` or
  the reports folder (write the exclusions out); docs updated (DECISIONS Multiplayer: relay
  retired; CLAUDE.md Layout: `server/` is accounts + reports).
- **Accept:** STD; `grep -rn "relay" web app engine` → only comments explaining history;
  `curl -sI https://joshuavelasco.com/bar/` → redirect; `curl -sI https://joshuavelasco.com/bar/relay.php`
  → 404; `curl -sI https://joshuavelasco.com/bar/accounts/` → still 403; the homepage
  (`https://joshuavelasco.com/`) still lists the bar; account_test passes.
- **Phone:** the old joshuavelasco.com/bar address sends you to the game; log in once if asked
  (the new address doesn't know your old log-in or settings).

### Phase 4: Clean core (local refactors, behaviour preserved; 4.1a-c, 4.3a and 4.4 may run before Phase 3, 4.4 after 4.3a; 4.2 and 4.3b need 3.4d)

#### 4.1a LocalPlayer and HUD out of `main.cpp`
- **Files:** new `app/local_player.{h,cpp}` (camera, controller, eye height, held item), new
  `app/hud.{h,cpp}` (on-screen text, prompts and labels now built in `main.cpp`), `app/main.cpp`.
- **Accept:** STD; VIS at the standard spots plus shots with `?sit=30`, `?rhythm=0,1&autoplay`,
  `?darts`, `?shift`, `?emote=3` → ≤ 1% differing pixels vs before; mp_test passes.
- **Phone:** quick tour: walk, sit, emote, read a prompt: all as before.

#### 4.1b The Game object and the phase table (`g_app` gone)
- **Files:** new `app/game.{h,cpp}`, `app/world.{h,cpp}`; `app/main.cpp` (≈ 100 lines);
  function-local statics → members with `reset()`; `app/materials.cpp` (`static_assert` on the
  table size).
- **Build:** the order of §2.1 (apply inbound → sim → send → present → render → flush → jobs) as
  a table of named phase functions; the seat-conflict check moves before NetOut (it goes away
  entirely in 5.2).
- **Accept:** STD + VIS and the test-param shots as in 4.1a; mp_test passes;
  `wc -l app/main.cpp` ≤ 150 (1,418 today); no mutable function-local `static` left in `app/`
  (`grep -n "^\s*static " app/*.cpp` reviewed).
- **Phone:** quick tour: sit, emote, piano, darts, 4K Mania, a drink: all as before.

#### 4.1c Presentation: remote visuals in one place
- **Files:** new `app/presentation.{h,cpp}` (others' darts and remote activity visuals move into
  Present, so they no longer vanish during rhythm/race; posing remote avatars), `app/game.cpp`;
  any `EM_ASM` still in `app/` (for example `hasBadge`) moves behind an engine call (E-7; 4.4
  then caches badges).
- **Accept:** STD + VIS and test-param shots; mp_test passes; `grep -rn "EM_ASM\|EM_JS" app` → 0.
- **Phone:** quick tour with your brother: his darts, piano and 4K Mania screens look as before.

#### 4.2 Stable entity ids
- **Files:** `app/bar_scene.{h,cpp}` (string id on every seat, interactable, prop, mirror, rig,
  machine), new `app/entities.{h,cpp}` (registry + 16-bit net ids, collision check), new
  `app/entity_ids.txt` (golden list), `app/shared_world.cpp` (glass keys by net id), seat param on
  the wire = seat net id, protocol version bump (the worker accepts protos N-1..N for one
  deploy, §2.12; the 4002 handling from 3.4a), Room DO `schemaVersion` bump (old glass keys
  by body index are wiped, not misread).
- **Needs:** 3.4d. The relay has no version check, so on the relay old and new builds would
  misread each other's seats.
- **Accept:** new `tools/native/entity_test.cpp` (includes only CPU headers, see 2.3b): ids
  unique, list equals `app/entity_ids.txt`; STD; mp_test glass + seat scenarios pass on
  `wrangler dev`; room_test: a proto N-1 client still joins, N-2 gets 4002.
- **Phone:** when the game says "New version", both of you reload; glasses and seats still
  sync with your brother.

#### 4.3a ActivityManager: framework, seats, piano, rhythm (local)
- **Files:** new `app/activity.{h,cpp}` (states Idle → Requesting → Active → Leaving, with an
  instant local grant for now so 5.2 only plugs in claims; transition table), new
  `app/activities/{seat,piano,rhythm}.cpp` (adapters over the existing game classes);
  `app/game.cpp` (their menu/busy/label flags and if/else branches deleted); UI commands become
  validated requests (emote range, `make` only on shift, `standUp` only when seated); local
  exclusivity still via `playerUsing`, but only inside `ActivityManager::canEnter` (claims
  replace it in 5.2). No wire change.
- **Accept:** new `tools/native/activity_test.cpp` (CPU headers only, see 2.3b; transition
  table cases, emote blocked during rhythm, standUp ignored when not seated); STD + VIS
  test-param shots (`?sit=30` is a rig, so add a bar-stool sit shot).
- **Phone:** sit, stand, piano, 4K Mania: each starts and ends cleanly.

#### 4.3b ActivityManager: darts, bartending, race + the pose layer (needs 3.4d)
- **Files:** new `app/activities/{darts,bartend,race}.cpp`; `app/game.cpp` (the remaining
  exclusive chains deleted; one place calls `setCapture`); occupancy vs pose layer in the
  avatar block (protocol bump: protos N-1..N accepted for one deploy).
- **Accept:** activity_test gains: walk-away leaves darts, drink while seated keeps the seat;
  STD + all test-param shots; `grep -rn "setCapture" app | wc -l` → 1; mp_test: Alice makes a
  drink while seated → Bob still sees her seated.
- **Phone:** both reload when asked. Darts, shift, a race: each starts and ends cleanly;
  making a drink while seated keeps you on the stool for your brother too.

#### 4.4 Interaction system
- **Files:** new `app/interaction.{h,cpp}` (one target query over interactables, seats, props,
  players with a priority bias; the active activity can intercept taps; handler table kind →
  `prompt()`, `available()`, `use(ctx)`; shared actions emit requests), `app/local_player.cpp`
  (`teleport(pos, yaw)` resolves collision + eye height; darts oche and bartender spot use it),
  badges cached on account change (no per-frame EM_ASM), `use()` switch deleted from `game.cpp`.
- **Accept:** new `tools/native/interaction_test.cpp` (CPU headers only, see 2.3b; resolver picks
  the nearer target with the documented priorities; jukebox beats a player standing behind its
  hit box); STD + VIS shots.
- **Phone:** tap the jukebox while someone stands next to it: the jukebox opens; walk to the
  dartboard and the bartender spot: you land in the right place.

### Phase 5: The server decides every shared thing (needs 3.4d live and 4.2 + 4.3a + 4.3b done)

#### 5.1a MySQL accounts, running dark (needs G2)
- **Files:** new `server/schema.sql` (§2.6 tables, `CREATE TABLE IF NOT EXISTS`, no reserved
  words: `login_attempts(attempt_key, fails, locked_until)`), new `server/account_v2.php` (PDO,
  prepared statements, utf8mb4; endpoints signup/login/refresh/me/profile/update/avatar/badge/
  logout/logoutAll; per-IP and per-user lockout, 8+ character passwords, origin allowlist,
  avatar headers `nosniff` + CORP, role column; deployed but not used yet), new
  `server/migrate_accounts.php` (applies `schema.sql` itself, because IONOS MySQL is reachable
  only from IONOS PHP; idempotent upserts of `users/*.json`, `tokens/*.json` → sessions with
  `expires_at` = import time + 30 days, `names.json`, `admins.json`; protected by a one-time
  secret; dry-run prints counts), `.github/workflows/deploy.yml` (writes the DB config as a
  `.php` file that returns an array, mode 0600, outside the docroot, with `printf` from the
  step env, never echoed; a G2 step that prints only set/missing), `admin.yml` gains
  `migrate-dry` and `migrate-run`, `tools/account_test.mjs` (PDO SQLite, plus a check of
  `schema.sql` against a MySQL reserved-word list, since SQLite accepts reserved words).
- **Before:** read 0.3's `report.php?env=1` result in DECISIONS (PHP version, sodium,
  pdo_mysql, private folder writable). Missing pdo_mysql → stop and tell the user.
- **Accept:** `php -S localhost:8090 -t server` + `node tools/account_test.mjs` → signup, login,
  refresh, logoutAll, 6th wrong password locked out, reserved-word check passes;
  `gh workflow run admin.yml -f action=migrate-dry` → its log (`gh run view <id> --log`) shows
  the expected user, session and badge counts.
- **Phone:** nothing changes.

#### 5.1b Switch logins to MySQL
- **Files:** `server/account_v2.php` → `server/account.php` (same URL path; the legacy
  `{a:'me', token}` API and old 48-hex tokens accepted for 30 days so cached old clients keep
  working); profile pictures moved from `accounts/avatars/<id>.jpg` to the private folder,
  rev numbers kept; `web/account.js` (on first load a saved `bar3dToken` is sent as the
  refresh token and replaced by the new pair; 8 s timeouts; card cache by id + rev); deploy
  order: backup of `home/bar/accounts/` → `migrate-run` again (idempotent, catches signups
  since the dry run) → switch; a dated `PROGRESS.md` item "delete `home/bar/accounts/` after
  <switch date + 30 days>".
- **Accept:** account_test passes against the new `account.php`, including the legacy API and
  a token saved by the old client; the migrate-run log shows counts equal to the JSON files;
  `curl -sI https://joshuavelasco.com/bar/accounts/` still 403.
- **Phone:** you stay logged in; your name, picture, bio and badges are all still there. Fail =
  logged out, or a picture missing.

#### 5.1c Signed tickets and in-band re-auth
- **Files:** `server/account.php` (Ed25519 tickets; key created only if absent with
  `fopen($path, 'x')` under `flock`; `?a=pubkey`; HMAC fallback only if 0.3 showed no sodium),
  `web/account.js` (ticket refresh every 10 min), `web/net.js` (ticket in HELLO; REAUTH on
  refresh), `cloud/room.js` (verify with `ACCOUNT_PUBKEY`; delete the IONOS fetch; REAUTH must
  keep the same `sub`; `/health` shows the kid and unknown-kid count), deploy steps fetch
  `?a=pubkey` and pass `ACCOUNT_PUBKEY` to wrangler (no hand pinning), `tools/room_test.mjs`.
- **Accept:** account_test: the ticket verifies in node `crypto.subtle`; two simultaneous
  first requests create exactly one key; room_test: forged ticket → treated as guest; REAUTH
  with a different `sub` → closed; STD.
- **Phone:** log out and in again: you don't vanish and reappear for your brother any more.

#### 5.2 Claims: seats, rigs, machines, bar station, mic
- **Files:** `cloud/room.js` (claims table, claim/release intents, RESULT, DELTA, release on
  grace end), `docs/PROTOCOL.md`, `app/activity.cpp` (Requesting state with the approach/sit
  animation; grant/deny/revoke), `app/game.cpp` (delete the seat tie-break loop and occupancy
  inference; remote occupancy from claims), `tools/room_test.mjs`, `tools/mp_test.mjs`.
- **Accept:** room_test: two clients claim `seat.bar.3` in the same millisecond → exactly one ok,
  the other `taken by`; socket killed → claim freed after grace; mp_test: Alice sits, Bob taps the
  same seat → Bob gets "Taken by Alice" and never sits.
- **Phone:** you and your brother tap the same stool at the same moment: only one sits, the other
  sees "Taken by …" with no teleport. Same for the 4K Mania cabinet and the dartboard.

#### 5.3 RoomState: lights, fire, mirrors
- **Files:** `cloud/room.js` (RoomState fields with seq; toggle intents; `schemaVersion` bump),
  new `app/room_state.{h,cpp}` (replicated struct + change events; renderer light scales set only
  from it), mirror mode local per viewer (Q3) saved in settings, legacy `lt` and `m<i>` keys
  deleted (and their echo shim).
- **Accept:** room_test: A and B toggle the lights at the same time 20× → both equal the server;
  STD; mp_test mirror scenario updated.
- **Phone:** flip the light switch: both phones change; the mirror mode only changes on your phone.

#### 5.4 Glasses: ownership leases
- **Files:** `cloud/room.js` (prop leases, rest poses in RoomState, PROP relayed in TICK, settle
  lease on owner loss; leases never persisted; `schemaVersion` bump), new `app/prop_sync.{h,cpp}` (grab/release intents; owner streams 15 Hz;
  others interpolate on server time and drive kinematic targets; only contacts from your own
  bodies/capsule trigger a claim; deny → follow the stream), `engine/physics.*` (`BodyId`, modes
  Dynamic/Held/KinematicTarget; optional kinematic player capsules so glasses bounce off people),
  `app/shared_world.cpp` (glass part removed), `tools/room_test.mjs`.
- **Accept:** room_test: two grabs → one owner; owner disconnects mid-throw → settle lease handed
  over, rest pose stored once; mp_test glass scenario on cloud passes; native interpolation-buffer
  test.
- **Phone:** your brother throws a glass: you see a smooth arc landing where he sees it land.
  Grab the same glass at the same time: only one of you gets it.

#### 5.5 Jukebox owned by the room
- **Files:** new `assets/music/songs.json` (stable id, file, title, credit, durationMs; a local test
  checks durations with ffprobe), `cloud/room.js` (queue, current `{songId, startMs}`, song-end
  advance as an alarm-scheduler deadline, play/skip/stop/enqueue with rate limits), `app/jukebox.cpp` (no writes on song end; stable ids;
  phone songs = local "My music" mode that ducks the room (Q8)), `web/audio.js` (drift follower).
- **Accept:** room_test: song end → exactly one advance even with 3 clients; STD; mp_test jukebox
  scenario passes.
- **Phone:** both phones hear the same song at the same spot; when it ends the next one starts on
  both together; your own phone song plays only for you, labelled "My music".

#### 5.6 Piano session, timestamped events, one Clock
- **Files:** `cloud/room.js` (piano session record; event stamping/TTL; `machine.piano` claim),
  new `engine/clock.{h,cpp}` (server↔audio mapping, latency + calibration), `app/piano.cpp` (song
  from the session; live notes as stamped events; spectators schedule at ts + playout; drop > 1 s;
  remote hands from their notes), `engine/midi.cpp` (same-tick order), `web/menu.js` (MIDI events
  call an exported function immediately with `event.timeStamp`), `web/net.js`/`voice.js` (no game
  events on the data channel).
- **Accept:** native clock test; mp_test variant: Alice plays 20 notes → Bob gets 20 in order,
  scheduled with even spacing (log the scheduled times); STD.
- **Phone:** your brother plays the on-screen keyboard: an even rhythm, no clumps, no stuck notes;
  a song picked by one of you plays in sync for both.

#### 5.7 Bartender on the server
- **Files:** `cloud/room.js` (drinks[8]; serve(type) checks the ticket badge → slot or "counter full";
  take(slot) compare-and-set; step events), `app/bartender.cpp` (requests; "Counter full" HUD;
  confirm before replacing a drink; spectator step sounds from the station emitter), legacy `dk*` deleted.
- **Accept:** room_test: two takes of one slot → one ok; serve without badge → denied; STD.
- **Phone:** your brother serves a drink and you both tap it at once: only one gets it. Without
  the badge the bar station says you can't serve. You hear his shaker from across the bar.

#### 5.8 Darts session
- **Files:** `cloud/room.js` (darts session: players, turn queue, scores), `app/darts.cpp` (throw
  events `{u, v}`; the same deterministic `score()` everywhere; spectators animate the flight and
  play sounds; clamp encoding; `leave()` clears darts; stats saved locally under a settings-schema
  key; D1 comes in 7.2).
- **Accept:** native score determinism test; mp_test: Bob sees Alice's dart fly and land on the
  same segment and score; STD.
- **Phone:** your brother throws: you see the dart fly and hear the thunk; the chalk score matches.

#### 5.9a Deterministic charts, baked at build time
- **Files:** `app/rhythm_chart.cpp` (fixed 11,025 Hz input, hop in seconds, parabolic tempo refine,
  onset snapping), new `tools/native/chart_bake.cpp` → `assets/charts/<song>-<difficulty>.json`,
  `app/rhythm.cpp` (built-in charts loaded from files).
- **Accept:** chart_bake gives ≥ 95% matching notes from 44.1 kHz and 48 kHz decodes of the same
  song (was 15/242); `tools/rhythm_test.mjs` passes and logs "chart from file"; STD.
- **Phone:** start a built-in song: no freeze before it starts.

#### 5.9b 4K Mania session, spectators, calibration, memory
- **Files:** `app/rhythm.cpp` (phone songs analysed in jobs with an "Analysing…" screen and cached
  per song + difficulty; Retry reuses the buffer; session `{songId, difficulty, startMs}`;
  judgement events; spectators draw the real chart; bests saved locally under a settings-schema
  key (D1 in 7.2); scrolling list; audio calibration screen), `web/audio.js` (11,025 Hz mono via
  OfflineAudioContext, no 54 MB copy, free on close), `cloud/room.js` (rhythm session).
- **Accept:** `tools/rhythm_test.mjs` passes; after closing a phone song the heap and JS memory
  return to the level before (logged); mp_test: Bob sees Alice's real chart; STD.
- **Phone:** Retry starts at once; with AirPods, run the calibration and the timing feels right;
  your brother watching sees the real notes on the cabinet; your best score survives a reload.

#### 5.10 Voice: signaling, bitrate, mesh cap, leaks, TURN
- **Files:** `web/voice.js` (trickle ICE over SIGNAL; default 48 kbps mono, "studio" opt-in (Q6);
  ≤ 6 nearest peers; one node chain per peer, `drop()` disconnects it; `restartIce` on
  disconnected/failed from either side; voice status per player), `cloud/room.js` (TURN
  credentials only if Q6 says yes and the user added `CF_TURN_KEY_ID` / `CF_TURN_KEY_TOKEN`
  (synced with `admin.yml secret-sync`): sent as a message over the open, HELLO'd socket, never
  from a public HTTP endpoint; TTL ≤ 10 min; ≤ 1 per minute per slot), `web/menu.js` (defaults).
- **Accept:** `tools/voice_test.mjs` passes; a page that toggles settings 10× keeps one node chain
  per peer (count logged); room_test: TURN credentials without HELLO → none, a second request
  within a minute → refused, TTL ≤ 10 min; STD.
- **Phone:** voice with your brother on cellular (not Wi-Fi) connects; changing voice settings
  doesn't make him sound doubled.

#### 5.11 Remove the legacy world KV
- **Files:** `cloud/room.js` (legacy KV and its storage removed; `schemaVersion` bump),
  `engine/net.*` (`setWorld`, `receiveWorld`, `takeOther`, `isLower` and the echo shim gone),
  `app/shared_world.*` (deleted if empty), `docs/PROTOCOL.md` (proto bump).
- **Accept:** `grep -rn "setWorld\|isLower\|takeOther" app engine cloud web` → none; room_test +
  mp_test + STD pass.
- **Phone:** quick multiplayer tour with your brother: everything still in sync.

### Phase 6: Faster drawing (use phone reports to pick the order inside the phase)

#### 6.1 Render core: RenderFrame, UploadRing, culling, instanced boxes
- **Files:** new `engine/render_frame.h`, `engine/upload_ring.{h,cpp}`, `engine/culling.{h,cpp}` (from
  `race/view.cpp` `visible()`), `engine/camera.h` (`frustum()`, yaw wrap), `app/renderer.cpp` (no
  setters writing GPU buffers mid-frame; one upload phase; delete the SceneObject path; one
  instanced draw for all boxes; glass drawn with opaques), `app/props.cpp` (dead path removed).
- **Accept:** STD + VIS ≤ 1%; profiler: "upload" ≤ 0.5 ms, draws ≤ before; a glass held
  in front of the mirror is visible.
- **Phone:** nothing looks different; perf overlay number a bit lower at the mirror.

#### 6.2a One floor plan: export `layout.json` from Blender
- **Files:** `tools/assets/layout.py` (exports `assets/layout.json`: placements, colliders, walk
  shapes incl. oriented rectangles, mirror, anchors, stable ids, prop collision proxies, and
  seats, interactables and lights; **those last three exist only in `bar_scene.cpp` today, so
  moving them into `layout.py` is its own sub-task**, done first), a one-time native
  comparison test (CPU headers only, see 2.3b). The Blender pipeline (`tools/assets/*.py`,
  20 scripts) wasn't audited: read `layout.py` and `common.py` before changing them.
- **Accept:** the comparison test: colliders, seats, anchors, interactables and lights from the
  JSON equal the old C++ values within 1 cm; rebuilt GLBs unchanged (`glb_report.py` counts
  equal); STD.
- **Phone:** nothing changes.

#### 6.2b Switch the game to `layout.json`
- **Files:** new `app/layout.{h,cpp}` (loads it; `bar_scene.cpp` numbers deleted), walk collision
  against oriented shapes.
- **Accept:** the comparison test still passes, then delete it with the old values; STD + VIS;
  walking along the curved bar in shots (`HOLD=`) shows no bumps.
- **Phone:** slide along the curved bar: smooth, no invisible bumps; seats still put you in the
  same places.

#### 6.2c Geometry pack format and loader
- **Files:** new `tools/assets/pack.py` (clusters ≈ 3 m per material with AABBs; 24 B vertices;
  Uint16 indices; LOD1 by Blender decimate; `assets/pack/manifest.json` + `geometry.bin`; one texture
  with sRGB view instead of duplicates), new `engine/pack.{h,cpp}` (header read + byte-budgeted
  upload jobs straight from the JS buffer); drawn without culling at first.
- **Accept:** STD + VIS ≤ 1%; download size reported; `max job ms` not above before.
- **Phone:** looks the same; loads a little faster.

#### 6.2d Renderer: cull clusters, sort blended parts
- **Files:** `app/renderer.cpp` (cull clusters; sort blended parts by bounds centre).
- **Accept:** profiler draws in the main pass ≤ 250 (with culling ≤ 150 at the spawn view);
  STD + VIS ≤ 1%.
- **Phone:** turn around in the middle of the bar: steady 60 on the overlay; send a report.

#### 6.2e Instancing repeated props
- **Files:** `tools/assets/pack.py` (instance prototypes for stools, tables, candles, glasses,
  pendants), `app/renderer.cpp` (one draw per prototype from an instance buffer), `app/entities.cpp`.
- **Accept:** draws main ≤ 150, mirror ≤ 60 (profiler); STD.
- **Phone:** same look; report at the mirror.

#### 6.3 The mirror as a proper view
- **User wish (2026-10-17):** in the mirror, avatars always full quality and everything within a
  radius of the mirror/viewer full quality; only things farther away in the reflection drop to low
  quality (LOD / cheaper shader). The half-resolution reflection from 1.7 should become sharp for
  that near zone too (e.g. full-res near layer or a higher-res target when close).
- **Files:** `app/renderer.cpp` (own cull list through the glass, small props excluded, LOD1,
  cheap shader variant, occlusion query on the glass quad (last frame's result), zone gate,
  30 Hz when still, quality tiers full / avatars-only / off).
- **Accept:** profiler: mirror draws ≤ 60; looking away from the mirror → mirror phase ≈ 0; STD +
  mirror shots.
- **Phone:** at the full mirror the overlay stays at 60; walk behind the bar where the mirror is
  hidden: the number drops further.

#### 6.4 Shader library, variants, light budget, sRGB
- **Files:** `shaders/*.wgsl` (shared include for tonemap/PBR; override-constant variants; ORM
  packing support), `engine/gpu.cpp` (`viewFormats` sRGB; render through the sRGB view), `app/renderer.cpp`
  (≤ 4 lights per cluster assigned on the CPU), `tools/assets/pack.py` (ORM packing), optional
  generated shader header instead of `--embed-file`.
- **Accept:** STD + VIS ≤ 1% (colours equal, no seams at former light-cell borders);
  `grep -n "pow(" shaders/scene.wgsl` → no gamma pow.
- **Phone:** colours look the same; no light seams on the floor.

#### 6.5 Avatars: lighter, culled, blended
- **Files:** `app/avatar.cpp` (curly hair ≈ 40 clumps ≤ 2k verts; named index ranges body/head/LOD1;
  build looks as jobs, own look first; free CPU meshes; bone indices; 0.25 s crossfade on activity
  change), `engine/skinned_mesh.*` (ranges, Uint16, 4 weights), `shaders/scene.wgsl` (4-weight blend,
  fewer samples), `app/renderer.cpp` (one bones storage buffer; per-look palette; frustum + mirror
  culling; LOD by distance; first person draws only the body range), `app/presentation.cpp` (pose
  only visible avatars).
- **Accept:** smoke log triangle count per look (Josh ≤ 12k); profiler "pose" and avatar draws drop
  with players outside the view (mp_test with 3 players); STD + `smoke-down.png` still shows your body.
- **Phone:** curly-haired avatars still look curly; sitting down blends instead of snapping; look
  down: your body is there.

#### 6.6 Retained HUD, billboards by id, picture path
- **Files:** `app/hud.cpp` (retained widgets: Label3D, ScreenPrompt, Banner, Toast; rebuilt on
  change), `engine/text.cpp` (cached glyph ranges; world labels in world space + per-view uniform),
  `engine/billboard.cpp` (slots by player slot, LRU), profile pictures through `loadImageTexture`
  variant with retry/backoff, `engine/fetch.cpp` `decodeImage` deleted.
- **Accept:** profiler "hud" ≤ 0.2 ms with 3 players (mp_test); STD + shots of name tags.
- **Phone:** name tags, prompts and pictures over heads look as before; a friend's picture shows
  up even if it failed once.

#### 6.7 Dynamic resolution and quality settings
- **Files:** `engine/gpu.cpp` (resolution scale 2.0/1.6/1.3), new quality controller (GPU-bound
  detection: frame > 15 ms while CPU < 8 ms, hysteresis), `web/menu.js` (Graphics: Auto/High/Medium/
  Low, mirror quality, field of view), `engine/camera.h` (horizontal-FOV minimum in portrait).
- **Accept:** headless forced tiers via `?quality=low` render correctly; STD.
- **Phone:** Settings → Graphics shows Auto; play for 5 minutes: no stutter when it changes level;
  portrait view feels less like a tunnel.

#### 6.8 ASTC textures (needs G3)
- **Files:** `tools/assets/pack.py` (astcenc 4×4 colour/normal, 6×6 big surfaces, pre-mipped, own tiny
  container), `engine/gpu.cpp` (request `texture-compression-astc` when available),
  `engine/texture_store.*` (WriteTexture of pre-mipped blocks, sliced by bytes; RGBA8 fallback path).
- **Accept:** report shows resident texture MB ≤ 64; Chromium (no ASTC) uses the fallback and smoke
  passes; VIS ≤ 1% on the fallback path.
- **Phone:** looks the same, loads faster, phone stays cooler; send a report.

### Phase 7: Racing together (replaces RACING.md steps 4-6; update that file in 7.1)

#### 7.1 VehicleState, snapshots, replays
- **Files:** `engine/vehicle.{h,cpp}` (POD `VehicleState` + `const CarSpec*`; quantized ≈ 32 B
  snapshot encode/decode; shift queue; clamp-hit counters), `app/race/world.*` (InputFrame per tick
  `{tick, steer i8, throttle u8, brake u8, buttons, shifts}`), one car-spec source shared with the
  tests, `docs/RACING.md` (sim contract, netcode, render budget, loading sections).
- **Accept:** native tests (g++ and em++/node): save → restore → replay gives identical results;
  `-O0` vs `-O3` wasm identical; lap-time bounds.
- **Phone:** nothing changes.

#### 7.2 RaceSession on the room + best laps in D1
- **Files:** `cloud/sessions/race.js` (roster from rig claims, seed, goAt in server ms, lap/finish
  reports validated (monotonic, plausible sector times), results, restart vote), `cloud/wrangler.toml`
  (D1 binding; the database created once with `admin.yml d1-create`), `cloud/migrations/*.sql`
  (best_laps, darts stats, rhythm bests keyed by account id; applied by the deploy steps with
  `wrangler d1 migrations apply --remote`), local darts stats and rhythm bests (5.8, 5.9b)
  uploaded once per account, `app/race/session.{h,cpp}`
  (Idle → Claiming → Grid → Countdown → Racing → Finished → Restart vote), `app/race/game.cpp`
  split into session / DriverView / RaceAudio.
- **Accept:** room_test race cases (both rigs claimed → one roster, one goAt; a fake lap of 10 s is
  rejected); STD with `?sit=30`.
- **Phone:** sit in a rig alone: countdown and race work as before; your best lap shows in the results.

#### 7.3 Two players: prediction, proxies, contacts
- **Files:** `app/race/world.cpp` (Proxy racers extrapolated to the current tick, finite-mass impulse
  applied only to your own car, ContactEvents, Baumgarte + slop + 2 iterations), race snapshots at
  30 Hz in 2-tick batches through the room, build hash checked on race join.
- **Accept:** native two-world test (two Worlds exchange snapshots with 100 ms ± 50 ms simulated
  latency; contacts push each car once, no launch); mp_test race scenario.
- **Phone (with your brother, left and right rig):** you both start on the same lights; bumping
  each other feels like a real nudge, nobody gets launched; results agree on both phones.

#### 7.4 Bots on one authority + AI module
- **Files:** new `app/race/ai.{h,cpp}` (BotDriver → InputFrame; lateral clearance, committed pass
  side, no passes in braking zones), host phone (lowest slot in the race) simulates bots and sends
  them in its snapshots; handoff of full VehicleStates if it leaves.
- **Accept:** native race_test bot sanity (stuck resets below N); two-world test: bots identical on
  both sides within interpolation error; host leaves → bots continue.
- **Phone:** in a two-player race, the bots are in the same places on both phones.

#### 7.5 Spectator monitors
- **Files:** new `app/race/monitor_renderer.{h,cpp}` (lite track ribbon at 4 m + barrier tops ≈ 6-10k
  tris, car LOD1, 960×180 atlas, one view refreshed per frame only when the rig is within ≈ 10 m and
  on screen), bar renderer samples the atlas as emissive on the monitor quads (3 or 1 per rig: Q10;
  for 3, add monitor quads in `tools/assets/sim_rig.py` and `app/bar_scene.cpp`/layout),
  spectators subscribe to the race snapshot stream (interpolated at now - 100 ms), CarVisual input.
- **Accept:** profiler: monitor phase ≤ 0.8 ms GPU-side estimate (draws ≤ 30); mp_test: Bob near the
  rig sees Alice's race on the monitors (screenshot).
- **Phone:** while your brother races, walk to his rig: the monitors show his race live.

#### 7.6 Race renderer and controls polish
- **Files:** `app/race/view.cpp` + `shaders/race.wgsl` (reverse-Z Depth32Float, sky last, backface
  culling, LOD chunks + terrain tiles + far cull, instanced trees/wheels, packed 20 B vertices +
  Uint16, distance-faded noise, CPU car-shadow sin/cos, optional MSAA 4× at DPR 1.5),
  `app/race/track.cpp`/`mesh.cpp`/`car_model.cpp` (chunked builds, reserve, shared strips, LOD1,
  degenerate quads), `app/race/controls.cpp` + `engine/gamepad.cpp` (per-axis "moved", delta source
  switching, all pads by role), pooled engine voices for the nearest cars.
- **Accept:** profiler race draws ≤ 100; native track build heap peak ≤ 8 MB; STD with `?sit=30`.
- **Phone:** no flickering curbs in the distance, less sparkle, steady 60 while racing.

### Phase 8: Hosting hardening

#### 8.1 Cloudflare Pages, hashed assets, no `version.txt`
- **Files:** `CMakeLists.txt` (content-hashed asset names + `manifest.json`), C++ resolves asset
  names through the manifest, `deploy.yml` (`wrangler pages deploy build/site`), new `web/_headers`
  (`index.html`, `config.json`: no-cache; hashed files: `max-age=31536000, immutable`), `web/index.html`
  (version check removed), canonical URL per Q5 (gh-pages kept as a mirror, then retired); the
  Pages project created once with `admin.yml pages-create`. A new origin starts with empty
  localStorage (`bar3dToken`, `bar3dSettings`, `bar3dName`, `bar3dProfile`): tell the user
  they'll log in and set settings again.
- **Accept:** `curl -sI <pages url>/index.html` shows no-cache; a hashed model shows immutable;
  check_live against the Pages URL passes.
- **Phone:** open the new address (Claude gives it) and log in again (a new address doesn't know
  your old log-in or settings); after a deploy you get the newest build at once.

#### 8.2 Deploy pipeline split and IONOS hardening
- **Files:** `deploy.yml` (the target jobs of §2.12, passing the site and `config.json` between
  jobs as artifacts/outputs; read-only build job; IONOS upload in its own concurrency
  group, `index` last; pinned SFTP host key via `sftp:connect-program` with `StrictHostKeyChecking=yes`),
  `homepage/.htaccess` (HSTS, nosniff, Referrer-Policy), optional: homepage to its own repo.
- **Accept:** a full Actions run shows build → test → deploy-cloud → deploy-site → deploy-ionos →
  verify; `curl -sI https://joshuavelasco.com/` shows the headers.
- **Phone:** nothing changes.

#### 8.3 Reports and telemetry on the Worker + D1
- **Files:** `cloud/worker.js` (`POST /report` with a per-IP limit (it counts against Worker
  requests), `GET /report/<code>`, D1 table, same no-secrets rule as 0.3), `config.json reportUrl`
  switched, `server/report.php` kept read-only for old codes then deleted, optional daily usage
  summary on `/health`.
- **Accept:** a report from smoke round-trips through the live worker; old codes still readable.
- **Phone:** "Send report" still gives a code.

---

## 5. Risks and open questions

### Questions for the user
- **Q1 (answered 2026-10-17):** the user created the Cloudflare account and added the GitHub
  secrets `CLOUDFLARE_API_TOKEN` and `CLOUDFLARE_ACCOUNT_ID`. Only possible follow-up: if 0.4's
  `cloudflare-check` finds no workers.dev subdomain, ask the user to open Cloudflare → Workers &
  Pages once; if the token lacks a permission (Workers Scripts, D1, Pages), ask them to edit it.
- **Q2 (blocking 5.1a):** create a MySQL database in the IONOS panel and save its host, name, user and
  password as GitHub secrets `IONOS_DB_HOST`, `IONOS_DB_NAME`, `IONOS_DB_USER`, `IONOS_DB_PASSWORD`.
  Only if 0.3's `report.php?env=1` shows no sodium: also a long random secret
  `ACCOUNT_HMAC_SECRET` (the ticket fallback).
- **Q3:** mirror mode per person (recommended: switching to "full" only costs your own phone) or
  shared like today?
- **Q4:** pause racing-feel tuning until Phase 2 is done (step 2.4a fixes the race's frame-rate bug
  and judder first)?
- **Q5:** one game address: keep `barcode162.github.io/3D` and redirect `joshuavelasco.com/bar` to it?
  Later `bar.joshuavelasco.com` on Cloudflare Pages?
- **Q6:** voice defaults to 48 kbps (still great for speech; "Studio" stays as an option) and
  connects to the 6 nearest people; add Cloudflare's TURN relay so voice works on cellular (may cost
  money past the free amount)? If yes: create a TURN key in the Cloudflare dashboard and add the
  GitHub secrets `CF_TURN_KEY_ID` and `CF_TURN_KEY_TOKEN` (the current API token can't create one).
- **Q7 (6.8):** OK to use `astcenc`, an offline texture compressor (like Blender, never part of the
  game), for 4× smaller textures?
- **Q8:** songs from your phone stay "only you hear them" (recommended; sharing copyrighted files
  is risky)?
- **Q9:** if you switch apps, keep your seat for 30 s before freeing it?
- **Q10:** spectator monitors: 3 per rig (left/middle/right) as planned, or 1 big one?
- **Q11:** OK to back up accounts with IONOS's database backup (or a nightly dump to a private folder)?
  IONOS cron may not exist on your plan; then it's IONOS's own backup plus a dump triggered by
  `admin.yml`.

### Risks
- **R1 Cloudflare free-plan limits** change or get hit → numbers re-checked at 3.2c; `/health`
  counters; at 70% of any daily quota clients drop STATE to 10 Hz (8 players for 3 h can reach
  ≈ 86% at the worst-case rate); "Server busy" with a 5 min retry when a quota is gone; DO CPU
  limit (10 ms per invocation) means server work stays tiny and race bots start on the host phone.
- **R2 iOS kills sockets in the background** → resume tokens + grace; "away" handling.
- **R3 Big refactors break tested features** → strangler steps, parity checklist at 3.4d, native +
  protocol + smoke tests in CI, phone checks after every step.
- **R4 Performance is only measurable on the phone** → reports at G4 after each phase; the user's
  time is the cost, so ask for reports only at phase ends and after 1.7, 6.2d, 6.3.
- **R5 Determinism needs identical builds** for races → build hash on race join; protocol bumps make
  old tabs reload; keep bumps rare.
- **R6 Accounts migration data loss** → backup + dry run + JSON files kept read-only for 30 days.
- **R7 Safari WebGPU gaps** (async pipelines, occlusion queries, `devicePixelContentBoxSize`) →
  feature-detect with fallbacks; test each on the phone.
- **R8 IONOS PHP may lack sodium** → HMAC fallback (§2.6).
- **R9 Asset rebuilds are heavy** (Blender) → only in 1.6 and 6.2.
- **R10 Cloudflare setup surprises** (G1 is met, but token permissions, a missing workers.dev
  subdomain or wrangler in CI can still fail) → 0.4's `cloudflare-check`, the 3.3 dark launch
  with `continue-on-error`, and 3.1-3.3 pulled early; never invest in the relay meanwhile.
- **R11 Session context limits** → steps sized for one session (the review split the big ones);
  read only the needed sections.
- **R12 Sessions can't see secrets or run admin commands** (`gh secret list` 403 through the
  proxy; `gh workflow run` untested) → CI steps print status; `admin.yml`; marker-file fallback.
- **R13 Paid extras by surprise** (R2 may ask for a payment method; TURN past the free amount)
  → ask the user before enabling either.
- **R14 Mixed builds during a deploy** (gh-pages caches `index.html` up to 10 min) → protos
  N-1..N for one deploy, wire changes only after 3.4d, 4002 handling that polls instead of
  reload-looping.

---

## Plan review (2026-10-17)
An independent check of this plan against the code at build `64d6ef6` (the commits since
touch only docs). Findings were applied above; this section records what was checked.

### Verified (claims that held up; no change needed)
- **Rendering:** 524 GLB primitives × placements (425 fixed + 6 tables × 16 + 3 rocks glasses);
  `drawModels` issues 2 bind groups + vertex + index buffer + DrawIndexed per part with no
  batching; the full mirror repeats it → ≈ 1,060 draws. No bounds or frustum test anywhere on
  the bar path. `discard` in all four fragment entry points (`scene.wgsl:159,179,279,290`).
- **Networking:** split brain after 3 failed connects or "room full" (HTTP 503 at upgrade),
  plus a second path: a failed `cloud.json` fetch also falls back to the relay (added).
  No-echo divergence is real on Cloudflare (A ends on B's value and B on A's); the relay
  converges. No idle timeout; `storage.put` of the whole world per world message. Voice
  510 kbps by default (`hq:true`) and STUN only. mp_test CLOUD mode polls PHP and always runs
  60 iterations; smoke passes with "Running." even if models never load. The old client
  fetches only `cloud.json`, so writing `config.json` switches nobody.
- **Frame loop and activities:** interaction runs before `others.update`, the seat is sent
  before the conflict check, `shared.update` has its own clock; lower-id-wins seats include
  the rigs; a newcomer who hasn't placed others can steal a seat.
- **Start-up:** procedural textures are built before downloads start and 5 of 8 layers are
  only used by the legacy path; a finished piano `pso` makes joiners synthesize every key
  (and it starts during loading); `parseGlb` runs untimed in the fetch callback; `finish()`
  is one job; at least one job always runs.
- **Audio and games:** piano synth inside the frame (native g++ -O2: middle C 15.7-20 ms,
  9.1 ms average per key; consistent with the 19 ms baseline); 96 `addPartial` passes for
  middle C; one live-voice table and pedal for everyone; MIDI pedal not sent; an HRTF panner
  per sound; Rhythm Track kept twice in JS and copied into wasm; 4K Mania renders the bar
  under its dimmer; charts use 12,000 Hz at 48 kHz; every client auto-advances the jukebox;
  drinks can be duplicated or lost.
- **Racing:** results depend on frame rate (race_test at 30/60/120 fps gives different
  winners); kMaxSteps 30 is unreachable (max 24); remote cars get infinite mass and the path
  is never exercised; MAX_VALUES 20.
- **World:** counter colliders overlap by 4 cm; kinematic bodies integrated per fixed step
  with velocity from frame dt; up to 64 substeps per frame.
- **Accounts, hosting, deploy:** `?server=`/`?cloud=` leak the token and passwords; tokens
  never expire; `names.json` is rewritten in place (a lost file makes the next signup the
  owner); account data sits in the web folder behind `.htaccess`; the `mput` list misses
  `menu.js`/`voice.js` and `mirror --ignore-time` skips same-size edits; `cloud/.wrangler`
  (6 files incl. 2 `.sqlite`, test data only) is tracked; `account.php` sends CORS `*`; the
  worker isn't deployed; IONOS `accounts/` and `rooms/` return 403.
- **Cloudflare config and baselines:** wrangler 4.42.0 pinned in `cloud/package.json`,
  compat date 2025-09-01, `new_sqlite_classes` (needed on free), `MAX_PLAYERS` 16; the
  `wrangler.toml:2` comment is false. vehicle_test: 0-100 in 4.22 s, 100-0 in 30.7 m. The
  free-plan numbers in §2.5.12 and the 2-player example are right.

### Corrected
- **Wrong references:** race protocol → `game.cpp:157-166` + `main.cpp:1134-1138`; race clock
  and HUD → `game.cpp:148,190`, `world.cpp:114-116,145-154`; no interpolation →
  `game.cpp:93,146-148`; 7 bots → `game.cpp:19,48`, `world.cpp:171`; race time source →
  `main.cpp:250,1224`; first-sit build → `game.cpp:35-42`; slot enum → `players.h:18-31`;
  reconnect index → `net.js:71,82-87`, `players.cpp:73-76`.
- **Wrong numbers:** "14 sync pipelines" → 11 at startup + mip mid-load + 2 race on first
  sit (14 call sites). "85.46 vs 92.15 s" wasn't reproduced → "different winners at
  30/60/120 fps". 8 players for 3 h: 45% typical but ≈ 86% worst case (trigger added).
- **Out-of-date status:** Q1/G1 were treated as open; the user already created the Cloudflare
  account and both secrets. Q1 is answered, the G1 fallback is gone, and G1 is confirmed by a
  CI step (sessions get 403 on `gh secret list`).
- **Plain-words list:** it merged Phase 3 and Phase 5 (stools and drinks are fixed in Phase 5,
  not by the move to Cloudflare) and skipped Phases 2 and 8. Rewritten, plus "What you might
  notice" (glasses reset at 3.4d, log in again on a new address, reload prompts).
- **Commands that couldn't work:** the 0.1 accept command (`grep -c wrangler` matches
  `wrangler.toml`); the 0.1 rule contradicted its own test and would have broken the relative
  `relay.php` default; a `*.workers.dev` suffix allowlist would still leak tokens (exact hosts
  now); the 2.3 native test can't compile because `bar_scene.h` pulls in the webgpu header
  (`engine/mesh_data.h` split added); a separate `deploy-cloud` job can't write
  `config.json` into the site and a job-level `if:` can't read secrets (now steps in the
  existing job); `-Werror` would fail on `engine/net.cpp:68`; the 1.2 MIDI-pedal phone check
  is impossible on an iPhone (headless check instead).
- **Order:** Phase 4 and 4.2/4.3 wire changes now wait for 3.4d (no version check on the
  relay); "worker accepts only the new version" → protos N-1..N for one deploy with a 4002
  handler that doesn't reload-loop; Phase 5 needs 4.2 + 4.3; 1.2 split around 1.4 (job lanes
  first, `pso` wire format unchanged); race pipelines compile during loading (1.3), the track
  builds after enter (2.4b); darts/rhythm bests stay local until D1 exists (7.2); 3.1-3.3 may
  run right after 0.4.
- **Steps split to one session each:** 1.2, 2.1, 2.3, 2.4, 3.2, 3.4 (old path stays default
  behind `?net=v2` until 3.4d), 4.1, 4.3, 5.1 (dark → switch → tickets), 5.9; 6.2a/6.2b became
  6.2a-6.2d and the old 6.2c is now 6.2e.
- **Missing coverage added:** `account.js` accounts URL (would have broken log-in from 3.4 to
  5.1); the echo shim that would have rewound thrown glasses and restarted music;
  `cloud_test.mjs` → `room_test.mjs`; 3.5 remote cleanup (relay files stay live without
  `--delete`; keep `project.json`, `accounts/`, `.htaccess`; port `account_test`/`voice_test`);
  storage `schemaVersion`; check_live/smoke joining the production room; an `admin.yml`
  workflow for one-off Cloudflare/IONOS actions; `engine/json`, `tools/setup.sh`,
  `cloud/package-lock.json`, `sim_rig.py`, the unaudited Blender scripts; more "What NOT to
  change" items; E-7 global check; CI smoke on GitHub runners; visual checks with
  `?still` + `tools/imgdiff.mjs` + fixed spots.
- **Limits:** no 5 s ghost alarm (it prevents hibernation); AFK close; rows written (not
  writes) budgeted; one alarm → scheduler; cached `/health`, per-IP connect limit, only
  `main` persisted, behaviour when a quota runs out, other free-plan limits listed.
- **Security:** `report.php` out of the docroot with caps, expiry, CORS and no secrets, plus
  `?env=1` to check IONOS PHP early; CSP that doesn't break local tests; keypair created only
  if absent and pushed to the Worker by the deploy; REAUTH keeps `sub`; TURN credentials only
  over an open socket; `npm ci` with the lockfile and the token only in step env; DB config
  0600 outside the docroot; `login_attempts.key` (MySQL reserved word) renamed.

### Still uncertain (check when the step comes)
- SwiftShader WebGPU on GitHub's runners (0.4 finds out; fallback written).
- Whether `gh workflow run` works through the proxy (only `gh run list`/`gh run view --log`
  were verified); marker-file fallback in §How to use.
- The Cloudflare token's permissions (Workers Scripts, D1, Pages) and whether the account
  already has a workers.dev subdomain (0.4's `cloudflare-check`).
- Free-plan details: Workers rate-limit binding on free, what a hit quota looks like to the
  client, R2 payment-method requirement, TURN pricing (3.2c, Q6, R13).
- IONOS: PHP version, sodium, pdo_mysql, writable private folder, cron (0.3 `?env=1`, Q11).
- Measurements not reproduced in the review: the ≈ 410 ms procedural textures, the ≈ 60 s
  pint jiggle, wasm synth timings (30.6 ms middle C; plausible from the native bench), the
  exact old race times.
- Whether the current build handles an empty `pso` safely (1.2a checks before clearing it).
- Safari support for async pipelines, occlusion queries and `devicePixelContentBoxSize` (R7).
- The exact CAM strings for the standard spots (0.4 picks them).
- `PROGRESS.md:14` still says to ask for Q1; this review could edit only this file, so 0.1
  fixes it.
