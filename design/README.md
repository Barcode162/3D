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
- `coaster/`: **Coaster**, the bar's menus as printed bar cards that slide
  up from the bottom, opened by a round coaster in the corner. Page:
  https://claude.ai/artifact/Qf8ApZP9okCJ7GsgXBh26S
  (Picked by Claude: the repo already has a Neon theme. Its first page, a
  seven-way comparison, is in this folder's history.)

## Rules for the lab
- Only fonts the repo already has a licence for (`ui/fonts/`,
  `ui/CREDITS.md`); a look-book loads them from `fonts/` when it's
  published (they are not copied here). No outside picture, font or library
  without asking (Charter §5).
- The lab stays on its branch, `job/J106`. Before it ever reaches `main`,
  `design/` is added to the "game changed" filter in
  `.github/workflows/deploy.yml`, so it can't start a game deploy.
