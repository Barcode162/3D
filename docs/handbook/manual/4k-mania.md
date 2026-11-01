# 4K Mania: feel, sync and mapping

How the 4-key rhythm game keeps time, how to sync it to your ears, and how
its Training maps are built: the patterns, what each one trains, and the
rules every map keeps. (System page: [4K Mania](../systems/world/4k-mania.md).
Plan: J117.)

## Timing

- **One clock.** The song's time runs on the page's precise timer and only
  eases toward the sound's own clock, which moves in steps of its buffer
  (3 % of the gap each frame; over 50 ms apart it snaps, after a pause). So
  notes move by the same step every frame, and a tap is judged at the exact
  time the finger landed (the touch's own timestamp), on the same clock.
  Before (until J117.2) both used the sound clock directly: notes sped up and
  slowed down by its steps, like scroll-speed changes.
- **Drawn for when you see it.** Notes are placed for the moment the frame
  reaches the screen (one frame ahead), not when it's started.
- **Sync (SYNC in the song select).** Clicks at 100 BPM; tap along with what
  you hear. The middle of how late your taps land (the median of the last
  32, after the first two clicks) is your phone's and headphones' delay; SAVE
  keeps it (`rhOffset`, ms; -5/+5 to nudge). The game takes it off the song's
  time, so both the notes and the judging follow the sound as heard.
  Bluetooth headphones add a lot: sync again after switching.
- **Frames.** While the game is open: one frame queued on the GPU and no
  30-a-second cap from Power: Cooler. Safari draws 60 a second unless
  Settings → Apps → Safari → Advanced → Feature Flags → "Prefer Page
  Rendering Updates near 60fps" is off (then 120 on the iPhone 15 Pro Max).
- **TEST** shows every finger, touches the phone lost or held back, frames a
  second and the longest gap, touch-to-game delay and how the sound clock
  steps; SEND REPORT sends them (they also ride with every report). Its
  GUARD switch: while you play, a listener that may cancel touches keeps the
  phone's own gestures (the three-finger Undo/Copy bar, the magnifier) out of
  the game (`rhGuard`).

## Stars

osu!mania's own star formula (ppy/osu, version 20241007, MIT), in
`bar/projects/mania/stars.cpp`: each note adds 2 to its column's strain
(fading to 12.5 % a second) and 1 to an overall strain (fading to 30 % a
second), ×1.25 while something else is held, more for a hold released apart
from the others; each 400 ms keeps its hardest moment; the sections, hardest
first, are summed with weights 0.9ⁿ and × 0.018. `tools/native/mania_test.cpp`
checks it against the plan's prototype.

What patterns cost (steady, 128 beats): 1/2 singles at 150 BPM ≈ 1.5★; 1/4
roll at 150 ≈ 2.2★; 1/4 stream at 150 ≈ 2.6★; jumpstream (a jump every half
beat) at 160 ≈ 4.0★; jumptrill at 150 ≈ 4.2★; 1/2 chordjack at 150 ≈ 3.1★;
1/4 chordjack at 150 ≈ 5.6★; dense handstream at 200 ≈ 6.3★. A real map with
rests rates a little under its hardest pattern. The auto-made charts rate
about 0.9 / 1.7 / 2.7 / 3.3★ (Easy … Insane).

## Pattern words

From the osu! wiki's pattern pages and Etterna's "Common Pattern Terminology".

| Pattern | What it is | What it trains |
|---|---|---|
| Jump / hand / quad | 2 / 3 / 4 keys at once | chords, timing under load |
| Stream | a run of notes on one snap, mostly different columns | speed, stamina |
| Roll | every column in turn (1234), split roll (1243, 4132) | flow, finger order |
| Jumpstream | a 1/4 stream with jumps in it | speed with chords |
| Handstream | a 1/4 stream with hands in it | strength in fast runs |
| Trill / jumptrill | two columns alternating / two jumps alternating ([12][34]) | alternation, speed |
| Split jumptrill | each hand trills its own two keys ([13][24] harder, [14][23]) | finger independence |
| Jack / minijack / longjack | one column repeated: 2 notes / 5-6+ | jack speed |
| Chordjack, gluts | chords that repeat columns; jumpjacks shifting columns | jack speed with chords |
| Anchor / runningman | one column busy inside a stream / an anchored stream, the anchor off the other notes | one finger steady while others move |
| Burst | a short, much faster run | reaction, speed |
| Polyrhythm | two rhythms at once (1/4 against 1/3) | reading, timing |
| Graces / flams | notes a hair apart | reading, precision |
| Hold notes | held, released on the sound | release timing |
| Shield / reverse shield | a note just before / after a hold in its column | hold control |
| Inverse | every gap filled with holds | reading, release timing |
| Holds against taps | a hold on one hand while the other taps | hand independence |

## The rules every Training map keeps

`bar/projects/mania/pack.cpp` (`stageRules`, `checkPackMap`), from
osu!mania's ranking criteria (written for ~180 BPM, scaled by tempo; 120 and
240 BPM are the breaking points), one stage per half star:

| Stage (stars) | Keys at once | Fastest step | Fast runs | Jacks | Holds |
|---|---|---|---|---|---|
| 1 (2.0) | 2 | 1/4 | ≤ 5 notes | none faster than 1/2 | ≥ 1/2 beat, 1/2 beat clear after |
| 2 (2.5) | 2 | 1/4 | ≤ 9 | none faster than 1/2 | the same |
| 3 (3.0) | 3 | 1/4 | ≤ 16, trills ≤ 9 | none faster than 1/2 | ≥ 1/4 beat |
| 4 (3.5) | 3 | 1/4 | ≤ 32, trills ≤ 9 | 1/4 minijacks | ≥ 1/4 beat |
| 5-6 (4.0-4.5) | 3 | 1/6 | any | up to 3 notes | ≥ 1/4, 1/4 clear |
| 7-10 (5.0-7.0) | 4 | 1/8 | any | any | ≥ 1/4, 1/4 clear |

And at every stage: never two notes in one column at once; every column
gets at least 15 % of the notes; the hands within 38-62 %; the stars within
the stage's range (target ±0.2; stage 10: 6.3-7.2); no 400 ms spike far
harder than the map's own hard parts (×1.6 at stage 1 down to ×1.45).
Beyond the checker, the maps follow the music: chords on the heavy hits,
holds on long sounds, the same sound gets the same pattern, the densest part
where the song is loudest, no scroll-speed changes.

## The Training pack

In the song select, **TRAINING** (beside SONGS) lists the pack: ten stages,
three maps each (Speed, Chords & jacks, Tech), about half a star apart from
2★ to 7★ (osu!mania stars). The strip of ten shows each stage's cleared
maps; a map is cleared at 95 % accuracy (a green ring on its grade).

- **Songs:** 30 hard-hitting tracks by TeknoAXE (CC BY 4.0, each checked on
  its own page), 120-228 BPM, in `assets/music/rhythm/pack/` (credits:
  `assets/music/CREDITS.md`, Help → Credits). `python3 tools/mania/songs.py`
  checks the licence, downloads, shortens (2:30, fade-out), encodes and
  times each one (`tools/mania/timing.py`: the beat grid fitted through
  every drum hit, the tempo snapped to its whole number; ±4-11 ms, drift
  under 1.1 ms), and measures per 1/8 beat how hard the kick, snare and hats
  hit and per beat how loud it is and how it rings (`tools/mania/songs/`).
- **Maps:** `tools/mania/maps.json` (one recipe a map: stage, skill, song,
  what it trains) → `tools/native/mania_pack.cpp build` → the chart files and
  `pack.json` in `assets/music/rhythm/pack/`. The builder places rows on the
  beat grid where the music hits (steady layers by how loud each bar is,
  finer notes on clear hits), gives chords to the heavy hits, lanes by the
  skill's patterns (Speed: streams → jumpstream → handstream; Chords: jumps →
  chordjacks, gluts; Tech: rolls, trills, runningmen, split jumptrills, holds
  against taps, inverse), repairs what the stage's rules refuse, and turns
  one dial (each harder pattern spread over a growing share of the notes,
  so the stars climb smoothly) until the stars hit the stage's target.
  `mania_pack check` re-checks the saved charts; `tools/native/mania_test.cpp`
  does too, with the native tests.
- **Playing one:** the chart comes from its file (no listening), the song
  from the pack; bests are kept per map (`rhAcc.pack:<id>`).
