# Sky platform demo

## Purpose

Prove two things in one small scene:

1. **Kinds share properties.** Every `sky-marker` starts with the same parts, loops and states.
2. **Things can override only what they need.** `marker-blue` and `marker-green` change colour (and one changes scale/phase) without rewriting the whole kind.

## What you get

- A 6 m wooden platform floating at y = 40
- One hanging lamp (shared kind)
- Three markers:
  - `marker-default` — pure shared amber
  - `marker-blue` — same kind, blue + larger scale
  - `marker-green` — same kind, green + phase offset

## Files

- `assets/world/kinds/sky.json` — the three kinds
- `assets/world/places/sky.json` — the place that uses them

## How to test (phone)

1. Merge or check out the `sky-platform-demo` branch.
2. In the engine / Dev / place loader, open the place named **sky** (or point the current viewer at `assets/world/places/sky.json`).
3. You should spawn on the platform. The three markers pulse; the blue one is larger and a different colour; the green one is out of phase.
4. If the bar still boots into `bar` by default, temporarily set the start place to `sky` in the same way the showroom is selected, then switch back.

## What this does *not* change

- No engine C++ changes.
- No change to the bar or showroom.
- The existing kind/thing/schema model is left intact; this only demonstrates it cleanly.

## Next step if it works

Use the same pattern for any new prop: put the shared behaviour in a kind, put only the differences on the thing.
