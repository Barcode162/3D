<!-- generated: node tools/handbook.mjs write (from docs/handbook/systems/); never edit by hand -->
# The systems

Every system of J Production, one page each, by chapter (the five chapters of the plan). Each page has the same ten parts: What it is · Status · What you can do with it · How it works · Data · Commands · Limits · Checks · Known traps · History.

## The engine

| System | Status | What it is |
|---|---|---|
| [Audio](../systems/engine/audio.md) | Live | Every sound in the bar: positional sound through Web Audio, synthesized instruments and effects, MIDI, the jukebox's songs. |
| [Driving](../systems/engine/driving.md) | Partial | Car physics for the racing sim: the original `looped/vehicle.*` and the new driving physics in `looped/drive/`, plus steering that feels like a real wheel for any input. |
| [Frame and core](../systems/engine/frame-and-core.md) | Live | The heart of Looped: the WebGPU connection, the one clock per frame, the frame's phases, logging, maths, files and the page mailbox. Everything else runs inside a frame this sets up. |
| [Input](../systems/engine/input.md) | Live | Touch, mouse, keyboard and game controllers turned into walking, looking and taps. |
| [Light](../systems/engine/light.md) | Live | How light works in the bar: real units, eyes that adapt, bounced light, lights with size, moods. The new lighting is the default; Classic stays one tap away. |
| [Loading and jobs](../systems/engine/loading-and-jobs.md) | Live | Start-up without freezes: background jobs over frames, asynchronous downloads, the loading screen's stages up to "Tap to enter". |
| [Materials](../systems/engine/materials.md) | Live | One record per kind of stuff (oak, glass, brass, leather...) for every sense: the look for the renderer, the feel for physics, the sound for audio. |
| [Network codec and protocol](../systems/engine/net-codec-and-protocol.md) | Live | How the phones and the room talk: one WebSocket, binary messages for movement, text messages for everything else, the same codec on both sides. |
| [Physics](../systems/engine/physics.md) | Live | Rigid bodies written from scratch: glasses that stand, tip, roll and clink; walls and furniture you can't walk through; impacts the sound engine hears. |
| [Post (eyes and mood)](../systems/engine/post.md) | Live | What happens after the scene is drawn: the eyes' exposure, the film curve and the mood (tint, saturation, contrast, vignette). |
| [Profiler and phone reports](../systems/engine/profiler-and-phone-reports.md) | Live | How long each part of a frame takes, and the reports a phone sends so Claude can see what happened there. |
| [Rendering](../systems/engine/rendering.md) | Live | Drawing the scene: models, shapes, mirrors, avatars, text and pictures over heads, with culling, compressed textures and background loading. |
| [The script language](../systems/engine/script-language.md) | Partial | Looped Script: J Production's own language, where **every word is a small script made of other words** (requests 2026-10-27 #10, #11). Places, things, lights and what they do are written as plain phrases ("a very warm candle that flickers fast"); each word's meaning lives in a dictionary file, written in other words, down to a few dozen roots the engine provides (the settings, the units, the shapes, a model, a box, a light). One phrase has one meaning; a mistake is caught before it runs, with its line, column, plain words and a fix hint. Not a replacement for C++ (the engine) or JavaScript (the pages). |
| [Skeleton and animation](../systems/engine/skeleton-and-animation.md) | Live | Bodies that bend: skeletons, skinned meshes, walking, sitting and emotes. |
| [Text](../systems/engine/text.md) | Live | The engine's own font and text: on the screen (prompts, HUD) and in the world (labels, signs), all in one draw. |
| [The UI kit](../systems/engine/ui-kit.md) | Live | Looped UI: the themes, parts and screens every page wears (the site, Dev, the Studio, the Toolbelt, the bar's menus), and the engine's reader of the same themes. |
| [The world model](../systems/engine/world-model.md) | Live | Looped's world core: every thing in a place, each a kind, each in a state, each state playing its loop on the shared clock. The bar, the Studio, the Dials and the room all speak it. |

## The world

| System | Status | What it is |
|---|---|---|
| [4K Mania](../systems/world/4k-mania.md) | Live | The 4-key rhythm game on the "4K MANIA" cabinet, made to look and play like osu!mania. |
| [The arcade](../systems/world/arcade.md) | Live | The arcade room and its machine: the user's platformer "Test 2", added as is. |
| [Avatars](../systems/world/avatars.md) | Live | Your body and look: presets or a custom look made in the customizer; others see it. |
| [The bar and its rooms](../systems/world/bar-and-rooms.md) | Live | Bar3D's place: the main bar, the arcade behind doorway A, room 3, and a second place, home. Walk in from the browser at joshuavelasco.com/bar. |
| [Bartending](../systems/world/bartending.md) | Live | Shifts behind the bar for players with the bartender badge: make drinks step by step, put them on the counter for anyone. |
| [Chess](../systems/world/chess.md) | Live | The lounge's chessboard: real pieces on the coffee table; play the computer, or the person already playing white. |
| [Darts](../systems/world/darts.md) | Live | The dartboard on the front wall: 301, three darts a turn, finish on exactly zero. |
| [The jukebox and music](../systems/world/jukebox-and-music.md) | Live | The jukebox: a song list played from its spot in the room, the same song at the same moment for everyone. |
| [Menus and HUD](../systems/world/menus-and-hud.md) | Live | The Menu button and its panels (HTML over the game), and the text the game draws on screen. |
| [The piano](../systems/world/piano.md) | Live | The grand piano: it plays itself, or you play it from the bench with the on-screen keyboard or a real MIDI keyboard. |
| [Racing and Meridian](../systems/world/racing-and-meridian.md) | Live | Two racing rigs: sit in one and the whole screen becomes a race (the racing sim), or open Meridian, the user's own driving game. |
| [Seats and taps](../systems/world/seats-and-taps.md) | Live | Sitting and using things: which seat or thing a tap means, and who has it. |
| [Voice chat](../systems/world/voice.md) | Live | Talking to other players, phone to phone (WebRTC), positional if you want. |

## The site

| System | Status | What it is |
|---|---|---|
| [Highlights and the front page's words](../systems/site/highlights.md) | Live | What the front page says: its headline and line, and one highlight (a picture, a few friendly words, a link). |
| [News](../systems/site/news.md) | Live | The public news board: posts Claude drafts after a job and the owner publishes. |
| [The site's pages](../systems/site/pages.md) | Live | joshuavelasco.com: the front page and its sections (Play, Engine, Vision, Projects, News, Sign in), in one frame and one look. |
| [Projects](../systems/site/projects.md) | Live | J Production's projects in one list: Bar3D, Looped, the script language, the workshop. |
| [Sign in and the portal](../systems/site/signin-and-portal.md) | Live | Signing in and making an account on the site; the staff portal that leads to Dev. |

## The tools

| System | Status | What it is |
|---|---|---|
| [The checks](../systems/tools/checks.md) | Live | Every test and check, listed once, run by system: only the ones covering what changed before each piece, all of them on GitHub at deploy. |
| [The command-line tools](../systems/tools/command-line-tools.md) | Live | Claude's tools, all through one front door: `node tools/project.mjs <command>`. |
| [Dev](../systems/tools/dev.md) | Live | The control room at joshuavelasco.com/dev: is everything up, what needs you, the plan, publishing, people. For the owner and the admins. |
| [The Dials](../systems/tools/dials.md) | Live | The world's numbers as dials (joshuavelasco.com/hub/dials.html): every field of kinds (loops, lanes) and things (where, turn, size). |
| [The gallery](../systems/tools/gallery.md) | Live | Every piece's before/after pictures and stats, and the benchmarks' history, kept forever on the `gallery` branch. |
| [The Studio](../systems/tools/studio.md) | Live | The world editor at joshuavelasco.com/studio: the bar's own engine in studio mode, with tools around it to pick, move, build, light, give life to and publish the world. |
| [The Toolbelt](../systems/tools/toolbelt.md) | Live | The list of every tool and every project (`toolbelt/tools.json`); its page now opens Dev → Projects. |
| [The UI Editor](../systems/tools/ui-editor.md) | Live | The editor for themes and screens (joshuavelasco.com/toolbelt/ui-editor/): the look every page and the bar's menus wear. |
| [World notes](../systems/tools/world-notes.md) | Live | Every kind of thing in the bar in plain words: its states and rules, how many there are (joshuavelasco.com/hub/world.html). |

## The backbone

| System | Status | What it is |
|---|---|---|
| [Accounts and the door](../systems/backbone/accounts-and-door.md) | Live | Who you are (accounts, profiles, badges) and who may come into the private bar. |
| [CI and deploys](../systems/backbone/ci-and-deploys.md) | Live | How a change goes live: one push to `main` per job, then four workflows, each for its own folders. |
| [Bar3D classic](../systems/backbone/classic.md) | Live | The last public build from before the bar went private, kept on the `classic` branch and served at joshuavelasco.com/classic/. |
| [Hosting](../systems/backbone/hosting.md) | Live | Where everything runs: IONOS for the domain, the site, the bar's files and the stores; Cloudflare for the room; GitHub for the code and the deploys. |
| [The ledger](../systems/backbone/ledger.md) | Live | The money and the things people own: a Durable Object with its own database, where every change is one transaction. |
| [The room server](../systems/backbone/room-server.md) | Live | The real-time server: one small server per room on Cloudflare, the authority for who is where, who holds what, and the shared world. |
| [Stores and keys](../systems/backbone/stores-and-keys.md) | Live | Where the tools keep their data on IONOS (the world, the UI, Dev, the hub) and the one key Claude uses. |
