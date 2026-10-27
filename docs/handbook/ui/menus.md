# The bar's menus

*Page and controls* · joshuavelasco.com/bar → Menu · [all pages](README.md)

The Menu button and its panels over the game ([menus and HUD](../systems/world/menus-and-hud.md)). Listed from its code (`bar/web/menu.js`): **change the menus and this page together** (the piece tool refuses otherwise, unless `--docs-unchanged "<why>"`).

| Control | What it does |
|---|---|
| `Add my birth date` | Voice chat, on an account made before birth dates were asked: sets your birth date once (13 and up, never changed after); only your age group is used, so teens talk with teens and adults with adults (J84). |
| `Avatar` | Presets, or customize your look. |
| `Back to driving` | Back to the race. |
| `Back to the bar` | From another place, back to the bar. |
| `Bartender` | The drink list (bartenders); End my shift. |
| `Emotes` | Wave, dance and the other emotes. |
| `End my shift` | Ends a bartending shift. |
| `Hear myself live (use headphones)` | Hear your own mic. |
| `Help` | How things work. |
| `Load a MIDI file (.mid)` | Your own piano song. |
| `Menu` | The button in the top bar: opens the menu. |
| `Meridian` | In a racing rig: opens Meridian. |
| `Piano` | The piano's songs; load a MIDI file; stop the piano. |
| `Places` | Every place: the bar, the test home, places made in the Studio. |
| `Profile` | Your card: picture, name, bio, drink, song, colour. |
| `Reset all settings` | Every setting back to its default. |
| `Reset to stock` | Car setup back to stock. |
| `Send report` | Sends a report to help Claude fix lag and bugs; shows a code. |
| `Settings` | Controls, Pointer, Sound, Graphics, Screen, Switches; Send report; Reset all settings. Graphics: `Quality` (`Auto`, `Ultra`, `High`, `Medium`, `Low`: each level sets the sharpness, lamps per pixel, soft shade, light beams and the mirror together, J107; the table is in bar/game.cpp applyQuality; Ultra and Auto ease while pictures fall behind, J111.2), `Power` (`Cooler`: 30 pictures a second; `Balanced`; `EXTREME`: never eases, J111.3), `Lighting`, `Lamp shadows` (On, Off: J97, the brightest lamps' shadows), `Soft shade` (On, Off: J91, ambient occlusion), `Light beams` (On, Off: J91.2, lamp light in the haze), `Mirror`, `Field of view`. |
| `Stand up` | Get up from your seat (when seated). |
| `Stop the piano` | Stops the piano's song. |
| `Test my mic (record 4 s, play back)` | Records 4 seconds and plays them back. |
| `Visit the test home` | Go to the test home (its own room). |
| `Voice chat` | Mic on/off, push to talk, the noise gate, mic boost, test your mic. Voice joins only signed-in people of your own age group (the room enforces it, J84): a guest is told to log in, an older account to add its birth date. |

## The bookshelf (J99)

Tap the lounge bookshelf: **The bookshelf** lists the books (title, author and translator, year, and where you are or how long it reads). Tap one to read: `‹` back to the list, `☰` contents (tap a chapter), `A−` / `A+` text size, `☀` / `☾` light or dark paper; tap the right side or swipe left for the next page, the left side or swipe right for the one before; the bottom bar shows the chapter and page with `‹` `›`. Your place in each book, the size and the paper are kept on the phone (`bar/web/reader.js`; books in `bar/web/books/`, made by `tools/books.py`).

## Chess, full screen (J101)

At the lounge chessboard the board opens full screen (since J28; `3D view` goes back to the table, `FULL SCREEN` there returns), flat over the game like the chess sites (`bar/web/chess.js`). On a fresh game, and from `New game`, **Who do you want to play?** lists the bots, each with a letter face, a line about how it plays and its rating: `Pip` 400 Elo, `Rosa` 800, `Theo` 1200, `Vera` 1500, `Odin` 1800 (the ratings are labels for how hard each plays; bar/projects/chess/chess.cpp kBots), and `A person (1v1)` (the next one to sit across from you plays black); the line under the status names your opponent. The board: the status on top, the board (yours at the bottom; files and ranks on the edge squares; the last move marked; dots where the tapped piece can go, rings on captures; the king in check glows red), then `Leave`, `New game` (white starts new games) and `3D view` (back to the table). Themes under it, kept on the phone: `Walnut`, `Sage`, `Dusk`, `Coral`, `Slate`, `Night`; each sets the square colours (always far apart in lightness), the pieces (classic or round, their colours and shine) and the background. The game keeps the rules and the room's game (bar/projects/chess: page operation `chess`, commands `chessMove`, `chessNew` (a: 0 the same opponent, 1–5 a bot, 100 a person), `chessFull`, `chessLeave`).

## Help → Credits (J100)

`Help` ends with **Credits**: the piano's recorded notes (Salamander Grand Piano V3 by Alexander Holm, CC BY 3.0; the licence asks for this credit), the music (Kevin MacLeod, CC BY 4.0), the models (Poly Haven CC0 and our own), the books (public domain).

## Settings → Pointer (2026-10-26 #12)

The crosshair and its editor (`bar/web/crosshair.js`). A preview of the crosshair on a dark disc stays docked at the top of the panel while you scroll through these settings and scrolls away after them. `Crosshair: tap anywhere to use what it points at` (on: drawn at the screen's centre; a tap anywhere uses what the centre points at), `Shape` (`Dot`, `Cross`, `Cross + dot`, `Circle`, `Circle + dot`), `Size`, `Thickness`, `Gap` (px), `Colour` (eight swatches), `Opacity`, `Dark outline`.

## Settings → Help Claude fix lag (J107.2)

`Check the GPU (25 s), then send a report` closes the menu and, for about 25 seconds, draws the bar as it is, then with one feature off at a time (soft shade, light beams, lamp shadows, bounced light, 6 lamps a pixel, 1x pixels, 1 frame queued instead of 2), measuring each step's frames, how many drew a picture, the late ones and the wait (`bar/gpu_check.cpp`); a toast says when it's done. `Send report` then carries the results (`perf.extra.gpuCheck`), so the code names what costs the most on that phone.
