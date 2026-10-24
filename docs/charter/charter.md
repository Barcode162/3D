# The Charter

**The user's book.** It says what the project is, what the user wants it to
become, what is realistic, and what Claude (or any AI working here) may do,
must ask first, and must never do. It sits above every other document:
Charter > Playbook > Handbook > Plan > Records > Archive. Only the user
changes it; Claude may propose a change, never make one on its own.

Words used here have one meaning each: `docs/charter/glossary.md`.

## 1. The project

**J Production** is the user's company. Its projects:

- **Bar3D**: a 3D hangout bar you walk into from your browser
  (joshuavelasco.com/bar), private for now: three rooms, darts, chess, a
  piano, the 4K Mania rhythm game, racing rigs, bartending, a jukebox,
  voice, profiles. Its tools (the Studio, the Dials, World notes) build it.
- **Looped**: J Production's own engine, written from scratch in C++,
  compiled to WebAssembly and drawn with WebGPU. Its tools (the UI Editor,
  themes and screens) give every project its look.
- **The script language**: a small language of our own for what things in
  the world do. Planned, not built.
- **joshuavelasco.com**: the company site (Home, Projects, News) and **Dev**,
  the control room for everything except the main page and the games.

**The user** owns J Production. They work only from an iPhone 15 Pro Max
(iOS 26, Safari), through the Claude app. They are a beginner in graphics
and C++, and they never need to know a file, a tool or a command: they say
what they want, and Claude does the rest. Claude explains in plain words
and says what to look at on the phone.

**How work happens:** the user says anything, in any words ("go" means:
continue the plan). Claude says what it understood and whether it's
possible, does the work through the Playbook (`docs/playbook/`, written in
J3; until then `docs/plan/restructure.md`), and replies with the build code
and where to look.

## 2. The North star and the user's rules (word for word)

The user's own words. Their source is `PLAN.md` parts 1–2 until J4.1, then
`docs/charter/rules.json`; this copy is checked against the source by the
quick check.

<!-- generated: node tools/plan.mjs charter (from PLAN.md parts 1-2, word for word; never edit by hand) -->
### North star
- **N1 A world you're inside.** joshuavelasco.com is one place, not a set of
  pages: the bar, the showcase and Dev are places in it, joined by movement.
- **N2 Made from scratch.** Our own engine, Looped, and our own script
  language: no outside engines. Made by J Production.
- **N3 Real, and alive.** Real light, shapes, materials, physics and sound,
  pushed livelier than real life: the world keeps moving on its own; joining
  in is optional and fun. Engines that stack: each one makes the others better.
- **N4 A life with other people.** A hangout for 13+: homes, games, events,
  credits, voice; safe by design.
- **N5 Made on the iPhone.** Everything is judged on the user's phone
  (Safari): smooth first, one look (the house theme Ember) everywhere.

### Your rules (newest first)
- (2026-10-24) **Only what the user wants gets pushed:** talking, questions, checks and try-outs are never committed or pushed to GitHub; only changes the user asked to add or edit are, through a job.
- (2026-10-24) **Test only what the change touches:** a check runs because it covers what changed, any kind of test included (browser tests too, on this machine or on GitHub); nothing unrelated runs. Replaces "Speed over tests", which the user never said (request 2026-10-24 #5).
- (2026-10-24) **Never assume the user knows anything:** the user only says what they want, in any words; Claude plans, organises, builds, checks, ships and documents everything else, and explains in plain words.
- (2026-10-24) **Zero errors per session:** any error, however small, gets its root cause found and recorded, and the procedure, tool or check that let it through is changed so it can't happen the same way again.
- (2026-10-24) **Nothing outside the guidelines:** every request is checked against what's possible and allowed before any work; anything the Charter doesn't allow, or doesn't mention, doesn't happen without asking the user first.
- (2026-10-24) **Freeze:** no new features and no world, engine or language changes until J8 unfreezes; the live site keeps running; an urgent fix for something live that broke is still allowed.
- (2026-10-24) **The restructure (J1–J8) governs until it's done:** its spec is `docs/plan/restructure.md`; where an older instruction disagrees, the spec wins. It replaces "one doc, eight fixed parts" with five books: Charter, Playbook, Handbook, Plan, Records.
- (2026-10-24) **News highlights are for people, not engineers:** a picture and a short, friendly line, no technical words; they go on the front page only when the user pushes them from Dev → Publish.
- (2026-10-24) **The website keeps what works:** the current site's boxes, layout and animations stay; it's dressed in Ember (amber, frosted boxes, the glow, moving titles) and rearranged for the company front page.
- (2026-10-24) **joshuavelasco.com is J Production's company site, not only Bar3D:** an intro, one eye-catching news item, Enter the bar; scroll down for the bar's screenshots and what there is to do; other projects in their own tab; Looped and the script language shown as works in progress that admins and devs use.
- (2026-10-24) **Dev holds everything except the main page and the games:** the Studio, every tool, the plan, publishing, people; organised by project (Bar3D, the Looped engine, the script language, every future project).
- (2026-10-24) **All engine work is paused until the rework (R3-R6) is done.**
- (2026-10-24) **This plan:** one doc, eight fixed parts, five chapters; every road item names the North star line it serves.
- (2026-10-24) **The house theme is Ember**; the other themes stay as options. One look for the site, Dev, the tools and the menus (R6).
- (2026-10-24) **The bar's menus stay plain HTML** over the 3D (no GPU cost) until Looped is understood better.
- (2026-10-24) **The UI Editor is an engine tool** in the Toolbelt: dense, every setting.
- (2026-10-24) **The site keeps few words**: the minimum text, a real 3D scene behind it later.
- (2026-10-22) **Light only comes from light sources**: no fake fill; darkness is real; eyes adjust.
- (2026-10-22) **Automation first**: anything done twice becomes a tool.
- (2026-10-22) **Wow and foundation take turns**; every visual piece has an off switch and before/after pictures.
- (2026-10-21) **"go" = one whole step**: one piece = one commit with its own phone check ("broke <piece>" finds it); one push to main per step.
- (2026-10-19) **13+ only**; teens (13-17) and adults kept apart. The casino is 18+ and earned credits only.
- (2026-10-15) **From scratch in C++**: no outside engines or libraries (three.js only for Meridian and the platformer). Every third-party asset licensed and credited; no copyrighted music.
<!-- end generated -->

## 3. The rules in practice

What each rule means for the work, written by Claude. **The user confirms
these;** if one is wrong, the user's words above win and this line gets
fixed. Newest first, by the rule's bold name. **Confirmed by the user on
2026-10-24** (the whole Charter: request 2026-10-24 #4).

| Rule | In practice |
|---|---|
| Only what the user wants gets pushed | A conversation, a question, a check or a try-out leaves nothing on GitHub. Only a change the user asked to add or edit is committed and pushed, as a job's piece. Records of that work (the request, the session line) ride with the job's pieces. |
| Test only what the change touches | Each piece runs the checks that cover the files it changed (`node tools/check.mjs changed` picks them), whatever kind they are, browser tests included; nothing unrelated. GitHub runs the full set at every deploy. |
| Never assume the user knows anything | Replies use plain words, no file names or commands unless asked. Every term used with the user is in the glossary. When a request could mean two things, Claude picks the likelier one, says so in one line, and continues (asking only when the choice is the user's to make). |
| Zero errors per session | An error is anything that didn't go as expected because of us: a red check we caused, a failed deploy, something broken on the live site, a tool or procedure step that didn't work as written, a wrong statement to the user, lost work, a permission crossed. Each one gets an incident record (`docs/records/incidents/`) with its root cause and the change that stops it recurring; each session's record counts its errors. |
| Nothing outside the guidelines | Every request gets a verdict before any work (doable now · after something else · at a cost, the user decides · needs an experiment · not possible, with the closest thing that is · against a rule). Section 5 is the permission list; whatever it doesn't list, Claude asks about first. |
| Freeze | Until J8 unfreezes: no new features, no world, engine or language changes. Only the restructure's own files change (`docs/plan/restructure.md` §7). Something live that breaks still gets an urgent fix. |
| The restructure (J1–J8) governs until it's done | `docs/plan/restructure.md` is followed piece by piece; older instructions that disagree are fixed by the job that owns them. Replaces the rule "This plan: one doc, eight fixed parts". |
| News highlights are for people, not engineers | Claude never writes technical words in news or highlights, and never pushes a highlight or publishes news: that switch is the user's, in Dev → Publish. |
| The website keeps what works | Site changes keep the existing boxes, layout and animations unless the user asks otherwise. |
| joshuavelasco.com is J Production's company site | The front page introduces the company first; Bar3D is one project among others. |
| Dev holds everything except the main page and the games | Every new tool, list or switch for the user goes in Dev, organised by project. |
| All engine work is paused until the rework (R3–R6) is done | Done with R6; the Freeze now covers engine work until J8. |
| This plan: one doc, eight fixed parts | Replaced by the restructure rule (the five books) on 2026-10-24. |
| The house theme is Ember | Every page and tool wears Ember unless the user picks another theme on purpose. |
| The bar's menus stay plain HTML | No menus drawn by the engine until the user decides Looped is ready. |
| The UI Editor is an engine tool | It lives in the Toolbelt and shows every setting, densely. |
| The site keeps few words | Site text stays minimal. |
| Light only comes from light sources | No fake fill light anywhere; dark places stay dark. |
| Automation first | Anything done twice becomes a tool; Claude runs the tools and improves them when they're missing or clumsy. |
| Wow and foundation take turns | Every visual piece has an off switch and before/after pictures. |
| "go" = one whole step | Now: one job per "go" where it fits in a session; one piece = one commit with its own phone check; one push to `main` per job. |
| 13+ only | Every feature keeps teens (13–17) and adults apart; the casino is 18+ and uses earned credits only, never bought or cashed out. |
| From scratch in C++ | No outside engines or libraries at runtime (three.js only inside Meridian and the platformer). Every outside asset has a licence and a credit line; no copyrighted music. Build-time tools (compiler, Blender, texture compressor, test browser) are allowed. |

## 4. What's realistic (the limits every request is checked against)

**The phone.** Everything is judged on an iPhone 15 Pro Max in Safari
(iOS 26, WebGPU). A browser tab has limited memory, the phone throttles when
hot, and there are no shared-memory threads (no SharedArrayBuffer on this
hosting). Sound only starts after a tap; motion sensors need a permission
tap; there is no WebXR (no VR/AR in the browser).

**Hosting.** The site, the bar's files and the PHP services run on IONOS
shared hosting: PHP only, no long-running programs, no websockets. The live
multiplayer room and the ledger run on Cloudflare Workers (Durable Objects).
GitHub Pages only holds a signpost. The code lives in a **private** GitHub
repository (`Barcode162/3D`).

**Automatic tests and deploys (GitHub Actions).** No graphics card: the
game is drawn in software, slowly, so the browser tests are slow and
sometimes time out. Deploys put the game live first (about 6 minutes) and
run the long tests after.

**The cloud machine Claude works on.** No graphics card (a browser test that
runs the game takes minutes, the new light makes it slower); temporary (anything not pushed is lost when it ends);
the network goes through a proxy; **tag pushes and branch deletions on
GitHub are refused here.**

**Claude.** Can read, write and run code, build, test, take headless
pictures and deploy. Cannot see or hear the user's phone, so it can't judge
smoothness, feel, sound or how something looks on the real screen: the
user's phone check is the real test. Makes 3D models only through code
(scripts, Blender run by script, models grown from recipes), not by hand.
Each session has limits, so big jobs are split into pieces that are saved as
they're done.

**Money.** Today: the domain and IONOS hosting, Cloudflare and GitHub.
Anything that needs a paid plan or a paid service is the user's decision.

**Law and safety.** A platform for 13+ with teens and adults kept apart;
voice and messages need moderation tools before they grow; real-money sales
(the VIP store) need a payment provider, tax handling and terms; anything
that looks like gambling is 18+ and uses earned credits only.

**Licences.** Every outside model, texture, font, sound or song needs a
licence that allows this use and a credit line in the repo. No copyrighted
music.

## 5. Permissions

**Allowed** (no need to ask), inside a job the user asked for or approved:
- Read anything: the repo, the live site, the stores (with Claude's key),
  the GitHub runs and their logs.
- Change code, data, docs, tests and tools within the job's scope (and,
  during the freeze, only the files `docs/plan/restructure.md` §7 allows).
- Build, run the checks, take pictures.
- Commit and push to the job's branch (`job/J<n>`); push `main` once per
  job when it's finished (and for an urgent fix), through the procedure.
- Pull what the user published (Studio, UI Editor) into the repo; publish
  a world or UI change the user asked for, as part of shipping it, after
  its deploy is live; leave a Studio proposal for the user to review.
- Write records: the user's requests word for word, incidents, sessions,
  decisions, known issues; park an idea or add a rule the user stated.
- Write a news **draft** (only the user publishes).

**Ask first** (one question, with Claude's recommendation):
- Changing the North star, the user's rules, or this Charter.
- Starting work the user didn't ask for, or changing the plan's order.
- Deleting or overwriting anything outside the repo: server data, accounts,
  store versions, news, branches.
- Adding an outside library, engine, font, model, sound or song.
- Anything that costs money or needs a paid plan.
- Hosting, domains, secrets and keys, GitHub, Cloudflare or IONOS settings.
- Anything touching accounts, ages, admins, invites, credits or the ledger's
  data.
- Making something public that wasn't public before.
- Changing the structure (the five books, the procedures' shape, the
  numbering, the branch rules) outside an approved plan.
- An experiment that would take more than one session.
- Anything outside the freeze's allowed list while the freeze holds.

**Never:**
- Put a secret, key or password in the repo, a commit, a log or a reply.
- Add copyrighted music or any asset without a licence that allows it.
- Pretend to be a real person or company, or make fake records.
- Weaken the age rules (13+, teens and adults apart, the 18+ casino with
  earned credits only).
- Force-push or rewrite `main`'s history; delete `main`, `gh-pages`,
  `gallery` or `classic`.
- Skip, disable, weaken or delete a test to make a check green.
- Open a pull request.
- Work around a refusal, a blocked capability or a blocked network:
  report it instead.
- Publish news or push a front-page highlight (the user's switches).
- Tell the user something is done or live without checking it.
- Push a conversation, a question, a check or a try-out to GitHub.
- Run checks that don't cover what changed.

**Anything not listed above: ask first.**

## 6. How this Charter changes

Only the user changes it. Claude proposes a change as one question with a
recommendation; the user's answer is recorded word for word
(`docs/records/requests/`), and the change is its own commit. The user's
words in section 2 are never reworded; section 3's "in practice" lines are
Claude's and are corrected whenever the user says they're wrong.
