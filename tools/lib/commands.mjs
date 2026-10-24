// Every command of the front door (node tools/project.mjs <command>), once
// (restructure J4.2). Each: name, a one-line summary, its usage, and what it
// runs (another tool, with its first arguments) or `own` (project.mjs does it).
// tools/playbook_test.mjs checks that the Playbook names only commands that are
// here, and the Handbook's command list is written from this (J6).
export const COMMANDS = [
  { name: 'help', own: true, summary: 'this list', usage: 'help' },
  { name: 'status', own: true, summary: 'the whole truth on one screen, each part with its age (--brief: a few lines)', usage: 'status [--brief]' },
  { name: 'plan', run: ['tools/plan.mjs'], summary: 'the plan: now, check, tick, add, park, rule, start, next, chapter, seal, render', usage: 'plan <command> …' },
  { name: 'piece', run: ['tools/piece.mjs'], summary: 'one piece: its checks, pictures, commit, push (docs/playbook/job.md)', usage: 'piece <ID> "<title>" --check "<phone check>" [--no-shots | --pages <page>]' },
  { name: 'check', run: ['tools/check.mjs'], summary: 'the checks for what changed (or named systems, or all)', usage: 'check [changed | all | <systems…>]' },
  { name: 'ship', run: ['tools/ship.mjs'], summary: 'finish a job: checks, merge, push main once, verify live, record, reply', usage: 'ship "<what the job did>" [--news "<friendly lines>"]' },
  { name: 'request', run: ['tools/records.mjs', 'request'], summary: 'record what the user said, word for word', usage: 'request "<the user\'s words>" --understood "<one line>"' },
  { name: 'incident', run: ['tools/records.mjs', 'incident'], summary: 'start an incident record (docs/playbook/incident.md)', usage: 'incident "<slug>" "<what happened>"' },
  { name: 'decision', run: ['tools/records.mjs', 'decision'], summary: 'start a decision record', usage: 'decision "<slug>" "<the decision>"' },
  { name: 'session', run: ['tools/records.mjs', 'session'], summary: 'the session line for the Records', usage: 'session --asked "<request ids>" --done "<pieces>" --shipped "<builds>" [--errors "<incident files>"]' },
  { name: 'handbook', run: ['tools/handbook.mjs'], summary: 'the Handbook\'s generated pages: write them, or check them (current, every folder described, links)', usage: 'handbook <write | check>' },
  { name: 'world', run: ['tools/studio.mjs'], summary: 'the world store: status, pull, push, propose, log', usage: 'world <status | pull | push --note … | propose --note … | log>' },
  { name: 'ui', run: ['tools/uikit.mjs'], summary: 'the UI store and the theme CSS: status, pull, push, css, check', usage: 'ui <status | pull | push --note … | css | check>' },
  { name: 'notes', run: ['tools/dev.mjs'], summary: 'the user\'s notes from Dev → Plan: notes, read <ids>', usage: 'notes <notes [--all] | read <ids…>>' },
  { name: 'news', run: ['tools/hub.mjs'], summary: 'news drafts: post, posts (the user publishes)', usage: 'news <posts | post --build <sha> --title … --text …>' },
  { name: 'live', run: ['tools/check_live.mjs'], summary: 'open the live bar headless and check the build (EXPECT_SHA=<sha>)', usage: 'live' },
  { name: 'bench', run: ['tools/bench.mjs'], summary: 'the engine\'s benchmarks, against the last run', usage: 'bench [native]' },
];
