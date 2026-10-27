# The script language

*System page · chapter: engine* · [all systems](../../generated/systems.md)

## What it is

Looped Script: J Production's own language, where **every word is a small script made of other words** (requests 2026-10-27 #10, #11). Places, things, lights and what they do are written as plain phrases ("a very warm candle that flickers fast"); each word's meaning lives in a dictionary file, written in other words, down to a few dozen roots the engine provides (the settings, the units, the shapes, a model, a box, a light). One phrase has one meaning; a mistake is caught before it runs, with its line, column, plain words and a fix hint. Not a replacement for C++ (the engine) or JavaScript (the pages).

## Status

**Partial**: being built in J116 (Looped, rebuilt; [its plan](../../../plan/jobs/J116.md) → The language everywhere), as the word lab (J116.2). It covers the old road jobs J43 (the design), J78 (the reader and checker) and J79 (the safe runner); J80 (in the Studio) is parked with the Studio. Not in the live bar yet: the bar inherits it in J116.6, and switches when the user says (J116.8).

## What you can do with it

- **The word lab** (joshuavelasco.com/bar/labs/words/): type a phrase and see what it means (a trail of why each number is what it is), how its lights glow over time, and every word of the dictionary with its meaning and the words it's like.
- **Write words** in `assets/words/*.looped`.
- **Write the bar's lights** as a light rig (`assets/world/rigs/bar.looped`, J116.2's first step): 10 light channels, moving exactly as the world's loops do.

## How it works

`looped/words/`, C++ → WebAssembly, built into the labs (not the live bar until the switch):
- `syntax.*` reads text into lines (indentation makes a line belong to the one above) and expressions; every problem has a line, a column, plain words and a hint ("did you mean …?").
- `program.*` compiles expressions (`1 + amount * flicker(period every)`) to a small tree of steps, with the engine's shapes (wave, noise, flicker, thump: the world's loop shapes, so a word and a kind's loop move alike), made shapes (`shape breathe (by 6%): …`) and settings bound by name; worked out within a step budget (8,000 steps an update).
- `words.*` is the dictionary and the phrase reader. Kinds of words: a thing (`made of the model …`, `made of a box …`, `glows …`, `gives light: …`, `has <a part> 9 cm above it`), describing words (`describes a light`: colour up/down K, lumens times; `describes a doing`: every/amount/speed times), doings (`settings: …`, `brightness times <expression>`), amounts (`very`: times 2), place words (J116.3). A phrase: how many, describing words and doings, the thing, then `, <settings>`, `that <doing> <how>`. Words that describe a light act on the thing's own light, or on its parts' (a warm candle warms its flame); a doing the thing already does is changed, not added twice. Things get their plural, doings their -s and -ing forms, by themselves.
- `rig.*` is the light rig: `light <name>: <lamps>`, `state`, `when <event>: go to … / fade to … over …`, `mood`, `brightness [each] …`; its channels feed the light formula.

## Data

- `assets/words/first.looped`: the first dictionary (31 words: things that give light and don't, colour and brightness words, words for doings, amounts, five doings).
- `assets/world/rigs/bar.looped`: the bar's light rig.

## Commands

- Native tests: `tools/native/run.sh` runs `words_test` (the dictionary checks; phrases mean what they say; 12 broken phrases and 8 broken words caught with their hint) and `rig_test` (the rig moves every lamp exactly as the world's loops; 34 broken copies caught).

## Limits

One meaning per phrase; the language grows by adding words, not grammar. Meanings are written, not guessed while the bar runs. No files, network, accounts, credits or other players' data. Places (place words), cause words (`when`, `makes`) in phrases and whole worlds in words come with J116.3.

## Checks

`words_test`, `rig_test` (native, g++ and em++).

## Known traps

- A word's name can't be an engine shape's inside an expression: `wave` the doing and `wave(…)` the shape live apart (a doing's meaning uses the shape).
- Subtracting needs spaces (`1 - x`): `light-sign` is one word.

## History

Named in the North star (N2); road item T1 until J7.1 renumbered it J43; scoped in J7.3 (2026-10-24). Started in J116.2 (2026-10-27): the light rig first, then every word a script (requests #10, #11) and the language everywhere (#13).
