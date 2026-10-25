// Every check, once (restructure J2.2). tools/check.mjs runs them by
// system; tools/workflows_test.mjs makes sure GitHub runs each one in exactly
// one workflow (.github/workflows/deploy.yml or hub.yml) unless NOT_ON_CI
// says why not, and that every tools/*_test.mjs is listed here (or in
// NOT_LISTED, with the reason).
export const EMSDK = 'source /opt/emsdk/emsdk_env.sh >/dev/null 2>&1;';
export const NP = 'NODE_PATH=$(npm root -g):$PWD/node_modules';
export const CHECKS = {
  // The quick set: no game running in a browser, so a few minutes even on the
  // cloud machine (no graphics card). Each piece runs this; the slow browser
  // tests run on CI at deploy (user, 2026-10-23).
  quick:    [['build', `${EMSDK} [ -f build/build.ninja ] || emcmake cmake -B build -G Ninja && cmake --build build`],
             ['native', `${EMSDK} FAST=1 bash tools/native/run.sh`], ['schema', 'node tools/schema_test.mjs'],
             ['albedo', 'node tools/albedo_test.mjs'], ['wgsl', `${NP} node tools/wgsl_check.mjs`],
             ['toolbelt', 'node tools/toolbelt_test.mjs'], ['uikit', 'node tools/uikit.mjs check'], ['plan', 'node tools/plan_test.mjs'],
             ['workflows', 'node tools/workflows_test.mjs'], ['playbook', 'node tools/playbook_test.mjs'], ['locks', 'node tools/locks_test.mjs'], ['handbook', 'node tools/handbook.mjs check'], ['engine', 'node tools/lint_engine.mjs']],
  plan:     [['plan', 'node tools/plan_test.mjs']],
  workflows: [['workflows', 'node tools/workflows_test.mjs']],
  workshop: [['playbook', 'node tools/playbook_test.mjs'], ['locks', 'node tools/locks_test.mjs'], ['handbook', 'node tools/handbook.mjs check']],
  build:    [['build', `${EMSDK} [ -f build/build.ninja ] || emcmake cmake -B build -G Ninja && cmake --build build`],
             ['lint_emasm', 'node tools/lint_emasm.mjs']],
  native:   [['native', `${EMSDK} bash tools/native/run.sh`], ['schema', 'node tools/schema_test.mjs'], ['albedo', 'node tools/albedo_test.mjs']],
  game:     [['wgsl', `${NP} node tools/wgsl_check.mjs`], ['smoke', `${NP} node tools/smoke.mjs`], ['live_world', `${NP} node tools/live_world_test.mjs`], ['touch', `${NP} node tools/touch_test.mjs`], ['travel', `${NP} node tools/travel_test.mjs`]],
  room:     [['proto', 'node tools/proto_test.mjs'], ['room', 'node tools/room_test.mjs'],
             ['ledger', 'node tools/ledger_test.mjs'], ['mp', `${NP} node tools/mp_test.mjs`]],
  hub:      [['hub', 'node tools/hub_test.mjs']],
  ui:       [['uikit', 'node tools/uikit.mjs check'], ['ui', `${NP} node tools/ui_test.mjs`]],
  uikit:    [['uikit', 'node tools/uikit.mjs check']],
  dev:      [['dev', `${NP} node tools/dev_test.mjs`]],
  labels:   [['ui_labels', `${NP} node tools/ui_labels_test.mjs`]],  // the Handbook's pages and controls (J6.4)
  toolbelt: [['toolbelt', 'node tools/toolbelt_test.mjs'], ['ui_php', 'php -l toolbelt/ui-editor/api.php >/dev/null'],
             ['ui_api', 'node tools/ui_api_test.mjs'], ['ui_editor', `${NP} node tools/ui_editor_test.mjs`]],
  site:     [['site_php', 'for f in $(find homepage -name "*.php"); do php -l "$f" >/dev/null || exit 1; done'], ['site', `${NP} node tools/site_test.mjs`]],
  studio:   [['studio_api', 'node tools/studio_api_test.mjs'], ['ops', 'node tools/ops_test.mjs'], ['studio', `${NP} node tools/studio_test.mjs`]],
  server:   [['php', 'for f in server/*.php hub/*.php; do php -l "$f" >/dev/null || exit 1; done'],
             ['hub', 'node tools/hub_test.mjs'], ['gate', `${NP} node tools/gate_test.mjs`]],
  mania:    [['rhythm', `${NP} node tools/rhythm_test.mjs`]],
  chess:    [['chess', `${NP} node tools/chess_test.mjs`]],
  arcade:   [['arcade', `${NP} node tools/arcade_test.mjs`]],
  meridian: [['meridian', `${NP} node tools/meridian_test.mjs`]],
  race:     [['race_mp', `${NP} node tools/race_mp_test.mjs`]],
  menu:     [['menu', `${NP} node tools/menu_test.mjs`], ['menu_games', `${NP} node tools/menu_games_test.mjs`]],
  avatar:   [['avatar', `${NP} node tools/avatar_test.mjs`]],
  voice:    [['voice', `${NP} node tools/voice_test.mjs`]],
};
// Which systems a changed file needs (first match wins; unknown files → game).
export const RULES = [
  [/^PLAN\.md$|^tools\/plan/, ['plan']],
  [/^\.github\/workflows\/|^tools\/workflows_test|^tools\/lib\/checks/, ['workflows']],
  [/^tools\/(check|piece|step|ci_status|dev|studio|hub|project|ship|records)\.mjs$|^tools\/lib\/(checks|gallery|commands|attribution)\.mjs$/, ['workflows']],  // the workshop's own tools: not the game
  [/^docs\/charter\/|^docs\/plan\/|^CLAUDE\.md$|^tools\/lib\/plandata/, ['plan']],                       // the Charter's copy of the rules
  [/^docs\/records\/known-issues\.md$/, ['workflows']],  // allowed-to-fail tests name open issues
  [/^docs\/playbook\/|^tools\/lib\/commands\.mjs$/, ['workshop']],                                    // the Playbook names only what exists
  [/^docs\/handbook\/ui\/|^tools\/lib\/labels\.mjs$/, ['workshop', 'labels']],                                // the pages and their controls
  [/^docs\/handbook\/|^tools\/handbook\.mjs$|^tools\/lib\/handbook_data\.mjs$/, ['workshop']],                                         // the Handbook's generated pages and links
  [/^tools\/lib\/docmoves\.mjs$/, ['workshop', 'plan']],  // where moved docs went: the Handbook's links and the plan's relink
  [/^tools\/githooks\/|^tools\/lib\/(guard|freeze)\.mjs$|^\.claude\/|^tools\/setup\.sh$/, ['workshop']],          // the locks
  [/\.md$|^docs\//, []],
  [/^homepage\/|^tools\/site_test/, ['site', 'labels']],
  [/^hub\/.*\.php$|^server\//, ['server']],
  [/^hub\/|^tools\/hub|^news\//, ['hub', 'labels']],  // the news page is tested by tools/hub_test.mjs
  [/^ui\/system\/|^tools\/uikit/, ['uikit', 'ui']],
  [/^ui\//, ['ui', 'build', 'game']],  // the bar's site carries /ui too (bar/ui/, S4.3)
  [/^tools\/ui_test/, ['ui']],
  [/^toolbelt\/|^tools\/(toolbelt|ui_editor|ui_api)_test/, ['toolbelt', 'labels']],
  [/^dev\/|^tools\/dev_test|^tools\/lib\/webspace/, ['dev', 'labels']],
  [/^studio\/|^tools\/studio/, ['build', 'studio']],
  [/^assets\/world\/|^bar\/studio_mode/, ['build', 'native', 'game', 'studio']],
  [/^cloud\/|^proto\//, ['room', 'build', 'native']],
  [/^bar\/projects\/mania\//, ['build', 'game', 'mania']],
  [/^bar\/projects\/chess\//, ['build', 'native', 'game', 'chess']],
  [/^bar\/projects\/platformer\//, ['build', 'arcade']],
  [/^bar\/projects\/meridian\//, ['build', 'meridian']],
  [/^bar\/projects\/racing\/|^looped\/(drive|vehicle)/, ['build', 'native', 'game', 'race']],
  [/^bar\/web\/menu\.js$/, ['build', 'game', 'menu']],
  [/^bar\/web\/voice\.js$/, ['build', 'voice']],
  [/^bar\/web\/net\.js$|^looped\/netproto|^bar\/(players|shared_world)/, ['build', 'native', 'game', 'room']],
  [/avatar/, ['build', 'native', 'game', 'avatar']],
  [/^tools\/(\w+)_test\.mjs$/, null], // a changed test runs itself (below)
  [/./, ['build', 'native', 'game']],
];

// Checks GitHub doesn't run, and why (each one is a known issue or a deliberate choice).
export const NOT_ON_CI = {
  'tools/workflows_test.mjs': 'it checks the workflows themselves; it runs in every quick check before a piece is committed',
  'tools/playbook_test.mjs': 'it checks the Playbook against the tools, nothing that deploys; it runs in every quick check before a piece is committed',
  'tools/handbook.mjs': 'it checks the Handbook against the files (generated pages current, folders described, links); it runs in every quick check',
  'tools/locks_test.mjs': 'it checks the locks of the work machine (git hooks, Claude Code hooks), nothing that deploys; it runs in every quick check',
};
// Test files that aren't in CHECKS at all, and why.
export const NOT_LISTED = {
  'tools/account_test.mjs': 'built for the old multiplayer relay (server/relay.php), retired on 2026-10-18; it needs rewriting for the room server before it can run (KI-27)',
};
