# The design lab (job J106)

A side space where Claude explores bolder looks for Bar3D's screens and
shows them to the user as look-books to react to on the phone. **Nothing
here is loaded by the game, the site or any tool**, and nothing here is
used until the user says "use it"; then the chosen direction becomes its own
job in the plan (built with the UI Editor's themes and screens, `ui/`).

## How a round works
1. Claude makes a look-book: the same content drawn several ways, so only
   the style changes. Each direction says how big it is to build (theme
   only, new controls, or engine work).
2. It is shown to the user privately (a claude.ai page, never on
   joshuavelasco.com).
3. The user says what they like, in any words; it goes in `likes.md`.
4. The next round refines what they picked, then carries it to other
   screens.

## The rounds
- `round-1/`: the bar's Jukebox menu, seven ways (today's Ember, Coaster,
  Neon Sign, Thumb Wheel, In-world Phone, Liquid Dock, Poster). Page:
  https://claude.ai/artifact/Qf8ApZP9okCJ7GsgXBh26S

## Rules for the lab
- Only fonts the repo already has a licence for (`ui/fonts/`,
  `ui/CREDITS.md`); a look-book loads them from `fonts/` when it's
  published (they are not copied here). No outside picture, font or library
  without asking (Charter §5).
- The lab stays on its branch, `job/J106`. Before it ever reaches `main`,
  `design/` is added to the "game changed" filter in
  `.github/workflows/deploy.yml`, so it can't start a game deploy.
