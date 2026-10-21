# Looped: the engine, the bar, the private site and the new hub (2026-10-20)

**Top priority** (user, 2026-10-20, ahead of the master list). Bar features
pause until the engine work (phases B and D) is built. Driving, performance and
the rest of the master list wait; their notes stay where they are.

## In plain words (for the user)
The project splits into clear parts, each with its own job:
- **Looped** is the engine (the name for now): drawing, light, sound, physics,
  multiplayer, and the **loop system** that makes the world feel alive.
- **Bar3D** is the main game built on Looped: the bar, your account, money,
  friends. Your other games (Meridian, the platformer, 4K Mania, chess, racing)
  live **inside the bar's folder** as its projects and plug into it one standard way.
- **Tools** are how you and Claude work: tests, deploys, the hub.

The game becomes **private**: only you, the admins you choose and the people
you invite can play it, at joshuavelasco.com/bar. The version from before this
push stays public as **Bar3D classic**.

The **hub** stops being cards. Home is a news feed (each version: what changed,
with screenshots) with "needs you" bubbles you answer in one tap; Explore is a
picture of the bar with glowing spots you tap, and the engine as a layer under
the floor. A **Publish** switch on a post puts it on the public news board.

Then the engine gets its two big upgrades: **lighting** (baked light and the
other tricks, the "Unity killer" look on a phone) and **loops** (every thing in
the world plays its own pattern on one shared clock, locked to the music; you
describe a loop in words, Claude writes it).

## The user's decisions (2026-10-20, chat)
- One repo, strict folders. Projects are children of the bar's folder (on the
  site they're part of the bar, not separate products).
- Private repo on GitHub **and** a private site: logins; the owner and admins
  they add; the owner invites people. The public build is the one from before
  this push ("classic").
- Free only (the user has $0): no GitHub Pro, no paid services. IONOS is paid
  for a year already; use it. Watch the GitHub Actions minutes (private repos
  on a free account have a monthly allowance).
- Hub: no cards. Home = news feed with versions and screenshots (reused as the
  public changelog) + one-tap decisions; Explore = the bar as a map, engine
  under the floor; per-thing talk with Claude. Lively: waves, motion, animated
  selection, rich colors; "it should feel like a sort of game". Simple
  decisions in front, Claude turns them into the right work in the background.
- The hub is the interface to the engine: planning first, live controls
  (a loop's speed, a light's color, the mood) once loops and lighting exist.
- Public news board (joshuavelasco.com/news): only posts the owner publishes.
- Order: A private → B folder split → C new hub + news → D lighting, then loops.

## Phase A: private (the game behind logins)
- **A1 Gate.** joshuavelasco.com/bar/ serves nothing without a login on the
  allowed list. `server/gate.php` + `.htaccess`: every request except
  `account.php`, `report.php`, `version.txt` and the sign-in page itself goes
  through the gate, which checks the login cookie and streams the file (with
  Range support: Safari's `<audio>` needs 206 replies) with the right type and
  caching headers. `account.php` sets the cookie (`bar_t`, HttpOnly, Secure,
  SameSite=Lax, Path=/) on signup, login, reset and `me`, and clears it on logout.
- **A2 Who may play.** The owner (admins.json, else the first account), the
  admins in admins.json, and the usernames in `accounts/players.json`
  (invited). Signing up stays open (the classic build uses the same accounts),
  but a new account sees "Ask the owner for an invite" until invited.
  Admin actions in account.php: `invite`, `uninvite`, `people` (list), and
  `admin` (owner only: make or remove an admin). The sign-in page has a People
  screen for admins.
- **A3 Claude's checks.** The gate also lets in the hub's Claude key (header
  or `?key=` once, then a cookie), so `check_live.mjs` can load the private
  site headlessly. Never logged, never in the repo.
- **A4 Classic.** The last public build (gh-pages "Site build 105f593") is
  copied once to joshuavelasco.com/classic/ (CI, `admin.yml` action
  `classic`). It keeps the same accounts and the open room "main". When the
  room's protocol moves on, classic may lose multiplayer; that's accepted.
- **A5 The room.** The private build uses room `looped` (config.json
  `room`); the room lets into `looped` only logins account.php says may play
  (`me` reply carries `play: 1`). Room `main` stays open for classic.
- **A6 Repo private (the user's two taps).** After A1-A5 are live: GitHub →
  the repo → Settings → General → Danger zone → Change visibility → Private.
  barcode162.github.io/3D then stops (free Pages needs a public repo); the
  deploy keeps pushing gh-pages as the record of the live build (the
  `changes` job reads it). The Claude GitHub app keeps its access.
- **Phone check:** /bar asks you to sign in, then the bar loads as before;
  a friend's new account sees "Ask the owner for an invite" until you add
  them in People; /classic is the old bar for anyone.

## Phase B: the folder split
Target layout (site URLs stay the same, so nothing changes on the phone):
```
looped/           the engine (was engine/), + engine-type code from app/:
  render/         renderer core, mirrors, materials, culling, gpu upload...
  ...             (audio, physics, net, text, ui, jobs, drive/ ...)
  shaders/        engine shaders
bar/              the main game (was app/ minus engine parts)
  web/            the page, menus, bridges (was web/)
  projects/       the bar's projects, one folder each, each with project.json
    meridian/     (was web/meridian/)
    platformer/   (was web/arcade/platformer/)
    mania/        4K Mania (was app/rhythm*)
    chess/        (was app/chess*)
    racing/       (was app/race/)
  assets/ (stays at the root for now: shared with tools/assets)
tools/  hub/  cloud/  server/  docs/  (unchanged)
```
- B1 Find the tangle: everything in app/ that isn't about the bar (renderer
  core, mirror, avatar skeleton system, loader stages, materials) → looped/.
  Rule: looped/ never includes bar/ (a CI grep enforces it).
- B2 Move engine/ → looped/, update CMake, includes, tools, CI, docs.
- B3 Projects into bar/projects/ with a `project.json` each (name, kind
  web|native, entry, credits). The "plug" (how the bar opens a project, who
  the player is, what it gives back) is written down in `docs/LOOPED.md` → The plug.
- B4 web/ → bar/web/.
- Proof: VIS before/after (≤1% pixels), every test green, same site paths.

## Phase C: the new hub and the news board
- C1 **News posts**: one per deploy (build code, title, plain-words "what
  changed", screenshots). The deploy takes the shots (`tools/shots.mjs`: the
  spots of what changed + a hero shot) and uploads them to the hub's private
  storage; Claude writes the words at "go" end (`hub.mjs post`).
- C2 **Home**: the feed, "needs you" bubbles (Claude's questions as one-tap
  choices: Yes / Not now / Change this + a box), what Claude is working on.
- C3 **Explore**: a picture of the bar with glowing spots (jukebox, rigs,
  arcade, mirror, lounge, bartender...) and the engine layer "under the
  floor" (Looped's systems) and Tools. A spot opens its page: one sentence
  of where it stands, its work items folded underneath, and a talk thread.
  Today's 66 cards become those work items, each attached to a spot; the
  owner's own words are kept untouched.
- C4 **Publish**: a switch on each post; joshuavelasco.com/news shows the
  published ones (same style, read-only, public).
- C5 Look and feel: wavy animated titles, glowing pulsing spots, animated
  selection, rich colors; reduced motion respected.
- `hub.mjs` gets the matching commands (inbox, post, spot, say, ask).

## Phase D: the engine upgrades
- **D1 Lighting**: baked light into images (Blender bakes at build time),
  light probes for moving things (avatar, glasses), a reflection capture for
  shiny things, bloom for neon, color grading (one small image sets the
  mood), fake fog and light beams; a few live lights stay for what moves or
  pulses. Before/after shots on the news feed.
- **D2 Loops**: one shared clock (the room's time), locked to the jukebox's
  beat when music plays; a loop is a small recipe file (what, path, length,
  personality, beat lock, reaction) Claude writes from the user's words;
  lengths that don't line up (3, 7, 11 s) + seeded randomness so nothing
  looks repeated; touch → react → blend back. Everyone sees the same loop
  without the server sending it. Hub controls: list, on/off, speed.

## The plug (B3, 2026-10-20): how a project fits into the bar
Every project is a folder `bar/projects/<id>/` with a `project.json`:
`name`, `about`, `kind`, `opens_from` (where in the bar), `credits`/`docs`.
- **kind "web"** (Meridian, the platformer: any tech, three.js allowed for
  these): `site` is where its files go on the site (CMake copies the folder
  there, so addresses never change), `entry` its page. The bar opens it full
  screen in an iframe from `bar/web/menu.js` (`GAMES`: `src` = site/entry),
  mutes its own music and sounds (voice stays), stops drawing, keeps the
  room connection, and shows "← Back to the bar". New web project = folder +
  project.json + one `GAMES` line + the machine or spot that opens it
  (`bar/bar_scene.cpp`, an Interactable with its action).
- **kind "native"** (4K Mania, chess, racing): C++ built into the game
  (`code` lists its files); it uses Looped directly and the bar's activity
  system (`bar/activity.*`) to start, take the input and end.
- **Who the player is:** same site, so a web project can read the login the
  bar keeps (`localStorage` `bar3dToken`, `bar3dProfile`) and call
  `account.php`; native ones use the App's account state.
- **What it gives back** (next, when a web project first needs it): a small
  message from the iframe to the bar (`postMessage({bar3d: 'earn' | 'score' |
  'close', ...})`), checked by the bar and the room (money only through the
  Ledger, never trusted from the page). Native games already report through
  the room (darts pays this way).
- **Friends:** native games share through the room (`cloud/games/`); web
  projects don't join multiplayer yet.

## Progress
- [x] Plan written (2026-10-20).
- [x] A1-A5 built (2026-10-20, `fc02cc1`): gate.php + gate.html, invites and
  admins in account.php, room "looped", classic.yml (the `classic` branch
  holds build 105f593), gh-pages a signpost; `tools/gate_test.mjs`.
  [x] A6: the repo is private (the user, 2026-10-21).
- [x] B2-B4 (2026-10-20): the layout above; `looped/` namespace stays
  `engine`; CI fails if looped/ includes bar/. Screenshots identical.
- [x] B1 (part): the scene renderer is Looped's (`looped/scene_renderer.*`,
  fed a `RenderScene` from `bar/bar_scene.cpp` → `renderSceneOf`). Left in
  bar/ on purpose: avatar.cpp (the bar's body; the skeleton system is
  already looped/), loader, interaction, materials (the bar's own data).
- [x] C1-C5 (2026-10-20): hub v3 (index.html; old board list.html), places
  (`hub/projects/bar3d-spots.json` + the floor plan `bar3d-map.json` from
  `tools/native/map_dump.cpp`), posts + screenshots, Publish → news/.
- [x] D1a (2026-10-20): glow and mood (`looped/post.*`, `looped/mood.h`).
- [ ] D1b-D1c: baked light, probes, reflection capture: `docs/LIGHTING.md`.
- [x] D2 first version (2026-10-21): `looped/loops.*` (swarm, pulse, flicker;
  the room's clock; beat lock from the music's real position, mailbox slots
  22-23), `assets/loops/bar.json` (8 loops; tempos measured with ffmpeg),
  `bar/loops_bar.*`; test `tools/native/loops_test.cpp`. Next: touch → react
  for more kinds, hub controls (list / on-off / speed), loops on models.
- **Replaced 2026-10-21 by the world core (docs/ENGINE.md):** looped/loops.*,
  bar/loops_bar.* and assets/loops/bar.json are gone; every loop is now a
  kind's state in `assets/world/kinds/bar.json`.

**2026-10-21:** next is the whole engine, `docs/ENGINE.md` (D1b-D1c and the
rest of D2 are its steps E4 and E1-E2).
