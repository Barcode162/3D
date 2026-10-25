# Dev → Overview

*Page and controls* · joshuavelasco.com/dev/#overview · [all pages](README.md)

The Live panel first: one tile per answer, coloured by it (green up, amber waiting, red broken). It checks again every 15 s while you look at it ("Updates by itself"); a tile whose answer changed glows once and its number counts to the new one. Tap a tile for its details and the reactions that go to Claude ([a Live tile](dev-live-tile.md)). Then what needs you (news to publish, world changes to look at, your calls) and the job in Now (its ring, the next piece, what comes after; tap it for the job's page).

| Control | What it does |
|---|---|
| `Claude's latest` | The last job Claude finished (its number), what it did, its build. |
| `Errors this week` | Claude's errors in the last 7 days (from the incidents), the latest by name. |
| `Game deploy` | The game's last deploy: Green, Red (the failed tests by name, and the ones allowed to fail), Running (stripes slide under it) or Stopped. |
| `In the bar now` | How many people are in the bar, from the room server; the heartbeat under it beats faster with more people, flat when it's empty. |
| `Site deploy` | The site's last deploy, the same way. |
| `The bar` | The bar is up, and the build it runs. |

The rows under Needs you come from data; one with an arrow opens what it points to.
