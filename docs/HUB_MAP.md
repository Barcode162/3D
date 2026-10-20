# The hub map: what goes where, and how the cards connect (2026-10-20)

The hub (joshuavelasco.com/hub) is the board of the whole world. This file is
the set of rules that keeps it organised, so nothing is planned twice, nothing
gets lost between sections, and every card knows what it waits on.
`node tools/hub.mjs check` enforces these rules; run it after any board change.

## 1. Two kinds of sections
- **World tab (what a player sees):** Arriving, Your character, The bar, Games,
  Social, Your home, Shops & money, Ideas.
- **Advanced tab (what makes it work):** Performance, Engine & code,
  Multiplayer server, Accounts & ledger, Tests & tools.

A feature that needs both is **split**: one card in the World tab for what the
player gets, and one card in the Advanced tab for the machinery, joined by
NEEDS. Example: "More games pay credits" (Games) NEEDS "Room checks chess moves
and 4K Mania scores" (Multiplayer server) and "The ledger" (Accounts & ledger).

## 2. Where a card goes (ask in order; the first yes wins)
**Does a player see or do it?**
1. Not sorted yet, no clear home? → **Ideas**
2. On top of the world, not in it (loading, the first screen, the home screen,
   Happening now, menus, HUD, buttons)? → **Arriving**
3. Money or items changing hands (credits, the mall, casino, VIP, trading)? → **Shops & money**
4. Your own place (your home, placing furniture, levels, invites)? → **Your home**
5. Something you play or work at (a score, a turn, a win; bartending too)? → **Games**
6. Other people (voice, profiles, lobbies, events, photos, messages, safety)? → **Social**
7. Your body and look (avatar, animation, poses, what you wear)? → **Your character**
8. A shared place and the things in it (rooms, props, lights, mirrors, jukebox, the street)? → **The bar**

**No, it's machinery:**
9. Speed, memory, loading time, quality settings? → **Performance**
10. Data kept forever per person (logins, ages, money, owned items, payments)? → **Accounts & ledger**
11. The live shared room (what it decides and checks, the protocol, voice relays)? → **Multiplayer server**
12. Code structure with no visible change? → **Engine & code**
13. How we work (tests, deploys, phone reports, this hub, the AI dock)? → **Tests & tools**

**Bugs** go in the section of the thing that is broken, titled "Bug: ...".

## 3. Stage "Idea" vs the Ideas section
- **Ideas section:** no home yet (e.g. a separate kids' game, the world's name).
  Each says `HOME:` with where it would go, or "not decided".
- **Stage Idea in any other section:** sorted, not planned or started yet.
  `PROGRESS.md` → Parked items are stage Idea cards.
When an Ideas card gets a home, it **moves** to that section (stage Idea).

## 4. One card per deliverable
- A card is one thing the user can check on the phone when it's done.
- Two cards never plan the same work. When work touches another card, the plan
  says `NOT HERE: <what> → <card> (#id)` instead of repeating it.
- Remaster ("do it properly later") items live in the plan of the card they
  improve, under LATER, not in a separate catch-all card.
- Big plans (racing, driving, restructure) stay in `docs/`; the card says which
  part of that file it covers (`DETAILS:`), so a fresh session finds it.

## 5. The card format (in the plan field)
```
YOUR NOTES: the user's own words, kept as written (only when the user wrote some)
WHAT: one line, what the player gets
NOW: what exists today
STEPS: 1. ... 2. ...          (or LATER: for done cards)
NEEDS: Title (#id); ...       cards that must be done first, or "nothing"
UNLOCKS: Title (#id); ...     written by `hub.mjs links` from other cards' NEEDS
NOT HERE: thing → Title (#id)
DETAILS: docs/FILE.md §...
HOME: section (Ideas cards only)
```
Only NEEDS is written by hand; UNLOCKS is always generated, so the two sides can
never disagree. A card in Ready or In progress whose NEEDS aren't Done is
flagged by `check` as blocked.

## 6. The backbone (what waits on what)
```
Clean core ─▶ Clean core leftovers ─▶ Room decides every shared thing (Phase 5)
                                          └▶ Live machine screens + fair queue
                                          └▶ Lobbies
Age at sign-up ─▶ Safety ─▶ Lobbies ─▶ Home screen, Events, No-friends lobby, Homes
                         └▶ Messages, Memory book, Trading
The ledger ─▶ Homes ─▶ The mall ─▶ Outfits and gadgets
           └▶ Casino (+ age), More games pay (+ room checks), Trading
Move accounts off IONOS ─▶ Payments (Stripe) ─▶ VIP marketplace
Performance pass ─▶ Faster drawing part 2 ─▶ Better light, The street
```
This is the same order as `PROGRESS.md`'s master list (3 accounts → 4 safety →
5 lobbies → 6 homes → 7 economy → 8 VIP → 9 events), now written as links.

## 7. Where things are written down
- **The hub:** status of each piece (stage, plan, Claude's report, questions).
- **PROGRESS.md:** the master list's order, Parked, what got built and when.
- **VISION.md:** the user's words and the product's direction.
- **DECISIONS.md:** what was chosen and why, gotchas.
- **docs/*.md plans:** the long specs that cards point to.
When a card's plan changes, update the doc it points to in the same session.
