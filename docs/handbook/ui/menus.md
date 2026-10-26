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
| `Settings` | Controls, Sound, Graphics, Screen, Switches; Send report; Reset all settings. Graphics: `Quality`, `Lighting`, `Lamp shadows` (On, Off: J97, the brightest lamps' shadows), `Soft shade` (On, Off: J91, ambient occlusion), `Light beams` (On, Off: J91.2, lamp light in the haze), `Mirror`, `Field of view`. |
| `Stand up` | Get up from your seat (when seated). |
| `Stop the piano` | Stops the piano's song. |
| `Test my mic (record 4 s, play back)` | Records 4 seconds and plays them back. |
| `Visit the test home` | Go to the test home (its own room). |
| `Voice chat` | Mic on/off, push to talk, the noise gate, mic boost, test your mic. Voice joins only signed-in people of your own age group (the room enforces it, J84): a guest is told to log in, an older account to add its birth date. |

## The bookshelf (J99)

Tap the lounge bookshelf: **The bookshelf** lists the books (title, author and translator, year, and where you are or how long it reads). Tap one to read: `‹` back to the list, `☰` contents (tap a chapter), `A−` / `A+` text size, `☀` / `☾` light or dark paper; tap the right side or swipe left for the next page, the left side or swipe right for the one before; the bottom bar shows the chapter and page with `‹` `›`. Your place in each book, the size and the paper are kept on the phone (`bar/web/reader.js`; books in `bar/web/books/`, made by `tools/books.py`).

## Chess, full screen (J101)

At the lounge chessboard, `FULL SCREEN` (between the game's two buttons) lays the board flat over the game, like the chess sites (`bar/web/chess.js`): the status on top, the board (yours at the bottom; files and ranks on the edge squares; the last move marked; dots where the tapped piece can go, rings on captures; the king in check glows red), then `Leave`, `New game` (white starts new games) and `3D view` (back to the table). Themes under it, kept on the phone: `Walnut`, `Sage`, `Dusk`, `Coral`, `Slate`, `Night`; each sets the square colours (always far apart in lightness), the pieces (classic or round, their colours and shine) and the background. The game keeps the rules and the room's game (bar/projects/chess: page operation `chess`, commands `chessMove`, `chessNew`, `chessFull`, `chessLeave`).

## Help → Credits (J100)

`Help` ends with **Credits**: the piano's recorded notes (Salamander Grand Piano V3 by Alexander Holm, CC BY 3.0; the licence asks for this credit), the music (Kevin MacLeod, CC BY 4.0), the models (Poly Haven CC0 and our own), the books (public domain).
