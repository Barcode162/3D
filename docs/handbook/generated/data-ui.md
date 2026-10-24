<!-- generated: node tools/handbook.mjs write (from ui/system/tokens.json and ui/system/parts.json); never edit by hand -->
# Data: themes and screens

What a theme sets and what a screen is made of ([the UI kit](../systems/engine/ui-kit.md)). Both files come from the UI store (`node tools/project.mjs ui pull`); never edited by hand.

## Theme tokens

Every token a Looped theme sets, once: its group, its kind (which control the UI Editor shows) and what it does. A theme file gives each one a value; tools/uikit.mjs writes ui/themes.css from them; looped.css builds every control from the tokens.

| Token | Group | Kind | What it does |
|---|---|---|---|
| `bg` | Colour | color | The page behind everything. |
| `text` | Colour | color | Main text. |
| `text-2` | Colour | color | Quieter text: notes, labels. |
| `text-3` | Colour | color | Quietest text: hints. |
| `rule` | Colour | color | Thin lines between things. |
| `accent` | Colour | color | The one colour that means 'this one': selection, focus, fills. |
| `key-text` | Colour | color | Words on a key. |
| `go-text` | Colour | color | Words on the main key (Publish, Enter). |
| `ok` | Colour | color | Good: saved, up, passing. |
| `warn` | Colour | color | Careful: waiting, unsaved. |
| `bad` | Colour | color | Wrong: failed, down. |
| `bg-glow` | Surfaces | paint | Light falling on the page from above. |
| `panel` | Surfaces | paint | A panel's fill. |
| `over` | Surfaces | color | A panel floating over the 3D view (see-through). |
| `field` | Surfaces | color | A well: text fields, meters, chips. |
| `key-bg` | Surfaces | paint | A key's fill. |
| `key-gloss` | Surfaces | paint | The shine on top of a key. |
| `go-bg` | Surfaces | paint | The main key's fill. |
| `knob` | Surfaces | paint | A knob's cap. |
| `panel-r` | Shape | length | Panel corner roundness. |
| `field-r` | Shape | length | Field corner roundness. |
| `key-r` | Shape | length | Key corner roundness (999px = a pill). |
| `panel-bw` | Shape | length | Panel frame thickness (for metal or wood frames). |
| `key-bw` | Shape | length | Key frame thickness. |
| `panel-edge` | Depth | paint | A panel's edge light and shadow. |
| `panel-blur` | Depth | paint | Frosted glass behind a panel ('none' for solid). |
| `field-edge` | Depth | paint | A well's sunken edge. |
| `key-shadow` | Depth | paint | A key standing up. |
| `key-shadow-down` | Depth | paint | A key pressed down. |
| `key-travel` | Depth | length | How far a key moves when pressed. |
| `go-glow` | Depth | paint | The glow around the main key. |
| `display` | Type | font | The big-title font, with fallbacks. |
| `display-w` | Type | number | Big-title weight (100 thin to 900 heavy). |
| `display-s` | Type | length | Big-title width (100% normal, 125% wide). |
| `display-ls` | Type | length | Big-title letter spacing. |
| `text-font` | Type | font | The font for words, with fallbacks. |
| `key-case` | Type | choice | Key, chip and choice words in capitals, or as typed. |
| `key-size` | Type | length | Key word size (chips and choices are a little smaller). |
| `key-weight` | Type | number | Key word weight. |
| `key-ls` | Type | length | Key word letter spacing. |
| `key-stretch` | Type | length | Key word width (75% narrow, 100% normal). |
| `c1` | Lights | color | Step light 1 (and 'hot' on meters). |
| `c2` | Lights | color | Step light 2. |
| `c3` | Lights | color | Step light 3. |
| `c4` | Lights | color | Step light 4. |
| `off` | Lights | color | A light that's off. |
| `led` | Lights | color | A lit LED in a key. |
| `step-shape` | Lights | choice | The step light's shape. |
| `bg-size` | Motion | paint | How big the background glow is drawn ('auto', or bigger than the page like '160% 160%' so it can drift). |
| `bg-motion` | Motion | choice | The background glow slowly drifts (lp-drift) or stays still. |
| `title-fill` | Motion | paint | Paint inside the big titles: 'none' for plain, or a gradient that moves through them. |
| `title-ink` | Motion | paint | The titles' own colour: 'currentColor', or 'transparent' to show the paint above. |
| `title-glow` | Motion | color | The light around a glowing title. |
| `title-motion` | Motion | choice | How titles move: a light passing through, a breathing glow, both, or a gentle float. |
| `title-speed` | Motion | length | Seconds for one round of the title motion. |

## Screen parts

The parts a screen is made of (docs/archive/UI_PLAN.md R1.3). Each part has a type and props; containers also have children. ui/screen.js draws them with looped.css; the UI Editor shows a control per prop from its kind (text, number, bool, choice, icon, list).

| Part | Name | What it is | Holds parts | Props (kind, default) |
|---|---|---|---|---|
| `stack` | Stack | Parts one under another. | yes | gap (number, 12); pad (number, 0); align (choice, "stretch") |
| `row` | Row | Parts side by side; they wrap on a narrow screen. | yes | gap (number, 8); justify (choice, "start"); wrap (bool, true) |
| `grid` | Grid | Parts in columns that fit the screen. | yes | min (number, 240); gap (number, 12) |
| `backdrop` | Backdrop | A picture behind parts, darkened toward the bottom so words read. | yes | src (text, "img/door.jpg"); height (number, 640); shade (number, 0.7); align (choice, "end"); pad (number, 20); gap (number, 12) |
| `panel` | Panel | A raised surface holding parts. | yes | pad (number, 16); gap (number, 12); over (bool, false) |
| `title` | Title | A heading in the display font. |  | text (text, "Title"); size (choice, "l") |
| `text` | Text | A line or paragraph. |  | text (text, "Some words."); tone (choice, "normal"); mono (bool, false) |
| `label` | Label | Small capitals above something. |  | text (text, "Label") |
| `key` | Key | A button. |  | text (text, "Key"); style (choice, "normal"); icon (icon, ""); led (bool, false); on (bool, false); disabled (bool, false); href (text, "") |
| `chip` | Chip | A small tag. |  | text (text, "Chip"); on (bool, false); led (bool, false) |
| `seg` | Choice | Pick one of a few. |  | options (list, ["One","Two","Three"]); on (number, 0) |
| `field` | Field | A text box. |  | placeholder (text, "Type here"); value (text, ""); mono (bool, false) |
| `switch` | Switch | On or off, with its words beside it. |  | text (text, "Switch"); on (bool, false) |
| `fader` | Fader | Slide to set a number. |  | min (number, 0); max (number, 100); value (number, 50); dec (number, 0); unit (text, "%") |
| `knob` | Knob | Turn to set a number. |  | min (number, 0); max (number, 1); value (number, 0.5); dec (number, 2); unit (text, "") |
| `steps` | Step light | Four lights stepping at a real loop speed. |  | period (number, 2); big (bool, false) |
| `meter` | Meter | How strong, as a row of bars. |  | level (number, 0.6) |
| `lamps` | Save lamps | Edit, saving, saved, live. |  | stage (choice, "saved") |
| `icon` | Icon | One icon. |  | name (icon, "note"); size (number, 24) |
| `image` | Picture | A picture, cropped to a shape. |  | src (text, ""); alt (text, ""); ratio (choice, "16:9") |
| `space` | Space | Empty room. |  | size (number, 16) |
| `listrow` | List row | One line of a list: an icon, words, a value, an arrow. |  | text (text, "Row"); detail (text, ""); icon (icon, ""); value (text, ""); tone (choice, "normal"); chevron (bool, false) |
| `tabbar` | Tab bar | The row of tabs at the bottom of a phone screen. |  | options (list, ["One","Two","Three"]); icons (list, []); on (number, 0) |

## The screens and themes there are

- Screens (9): `bar-menu`, `dev-overview`, `dev-people`, `dev-plan`, `dev-projects`, `dev-publish`, `front-door`, `index`, `sampler`
- Themes (12): `backbar`, `bold`, `clean`, `ember`, `faceplate`, `glass`, `index`, `jukebox`, `neon`, `obsidian`, `soft`, `warm`
