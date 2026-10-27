# The design lab (job J106)

A side space where Claude explores bolder looks for Bar3D's screens and
shows them to the user as look-books to react to on the phone. **Nothing
here is loaded by the game, the site or any tool**, and nothing here is
used until the user says "use it"; then the chosen direction becomes its own
job in the plan (built with the UI Editor's themes and screens, `ui/`).

## How it works
The lab works on **one theme at a time** (the user's word, request
2026-10-26 #13). Claude shows it on a page the user opens on the phone; the
user says what to change, in any words (it goes in `likes.md`); Claude
changes the page at the same link.

## The theme
- `theme/`: **one theme, seven parts**, each idea the user liked in the
  place they gave it (request 2026-10-26 #14): the wheel (the only menu
  button), the glass strip (small menus), the phone (everything at once),
  clean glass (everyday panels), the card (notes you pick up), the game
  tablet (the arcade, neon), the poster (big news). Shared: one amber, a
  slight sideways tilt, small live details. Since J106.4: tap the menu
  button for the quick actions, hold it for everything in groups (slide and
  let go to pick), and a new selection marker in the bar. Page:
  https://claude.ai/artifact/Qf8ApZP9okCJ7GsgXBh26S
  (Earlier pages, the seven-way comparison and Coaster alone, are in this
  folder's history.)

## Rules for the lab
- Only fonts the repo already has a licence for (`ui/fonts/`,
  `ui/CREDITS.md`); a look-book loads them from `fonts/` when it's
  published (they are not copied here). No outside picture, font or library
  without asking (Charter §5).
- The lab stays on its branch, `job/J106`. Before it ever reaches `main`,
  `design/` is added to the "game changed" filter in
  `.github/workflows/deploy.yml`, so it can't start a game deploy.
