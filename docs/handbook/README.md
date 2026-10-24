# The Handbook

**How everything works**: facts, not instructions (what to do, and when, is
the Playbook's: `docs/playbook/`). Order of authority: Charter > Playbook >
Handbook > Plan > Records > Archive. Pages under `generated/` are written
from the files by `node tools/project.mjs handbook write` (the piece tool
does it in every commit); never edit them by hand. `handbook check` (in
every quick check) keeps them current, every folder described, every link
and every doc path in the repo pointing somewhere real.

## Where things are
- [The map: every folder](generated/map.md): what it is, its files and languages, who uploads it where.
- [The technology](tech.md): every language and outside tool, its version, why.
- [The data map](data-map.md): every store, who writes and reads it, what wins.
- [The deploy map](deploy-map.json): which workflow uploads each folder where.

## How it works
- [The systems](generated/systems.md): one page per system (engine, world, site, tools, backbone), each with its status.
- [Every page and its controls](ui/README.md): by exact label.
- [The commands](generated/commands.md): every tool through the front door.
- [Build and test](build-and-test.md): building, the checks, the page options.
- [The phone](phone.md): what iPhone Safari allows and doesn't.
- [The room protocol](protocol.md): every byte between a phone and the room.
- [The manual](manual/README.md): light, materials, the world, avatars, recipes and the timeline in depth, with every dial.

## The data
- [The world's files](generated/data-world.md) · [themes and screens](generated/data-ui.md) · [the lists and the config](generated/data-lists.md) · [the protocol's messages](generated/data-protocol.md)
- [The world inventory](generated/inventory.md): every thing in every place and its model.

## Doing things
- [Recipes](recipes.md): common changes, step by step (inside the Playbook's job).
- [Known traps from before the restructure](traps.md).
