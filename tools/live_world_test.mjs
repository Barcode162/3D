// The bar loads the Studio's live world (docs/archive/STUDIO.md S1.5) only with
// ?liveworld=1 since the Studio is parked (J114): a fake store answers
// "version 7" with the bar minus one thing; without the option the bar keeps
// the world built in, with it the bar says it uses v7 and builds one thing
// fewer. Without a store (404) it uses the world built in; a bundle needing a
// newer engine is ignored.
// Usage: NODE_PATH=$(npm root -g) node tools/live_world_test.mjs
import { readFileSync } from 'node:fs';
import { launchBrowser, logText, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const { url, close } = await serveSite();
const browser = await launchBrowser();
let failed = 0;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed++; };
const read = (f) => JSON.parse(readFileSync(f, 'utf8'));
const place = read('assets/world/places/bar.json');
const fewer = { ...place, things: place.things.filter((t) => t.id !== 'dartboard') };

async function run(live, bundle, query = 'skipintro&server=off&liveworld=1') {
  const page = await browser.newPage({ viewport: { width: 430, height: 932 } });
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  if (live) {
    await page.route('**/studio/api.php?live', (r) => r.fulfill({ contentType: 'application/json', body: JSON.stringify(live) }));
    await page.route('**/studio/api.php?bundle=*', (r) => r.fulfill({ contentType: 'application/json', body: JSON.stringify(bundle) }));
  }
  await openBar(page, url, query);
  await page.waitForFunction(() => /World: \d+ things/.test(document.getElementById('log').textContent), null, { timeout: 120000, polling: 250 }).catch(() => {});
  const log = await logText(page);
  await page.close();
  return { log, errors, things: +((log.match(/World: (\d+) things/) || [])[1] || 0) };
}

const builtIn = await run(null);
check(/World: built in\./.test(builtIn.log) && builtIn.things > 150, `no store: the world built in (${builtIn.things} things)`);
const live = await run({ ok: 1, version: 7, author: 'Test' },
                       { format: 1, version: 7, author: 'Test', files: { 'places/bar.json': fewer } });
check(/World: v7 \(live, by Test\)/.test(live.log), 'a live world: the bar says it uses v7');
check(live.things === builtIn.things - 1, `and builds it (${live.things} things, one fewer)`);
check(live.errors.length === 0 && !/World:.*couldn't/.test(live.log), 'no errors');
const parked = await run({ ok: 1, version: 7, author: 'Test' },
                         { format: 1, version: 7, author: 'Test', files: { 'places/bar.json': fewer } }, 'skipintro&server=off');
check(/World: built in\./.test(parked.log) && !/World: v7/.test(parked.log) && parked.things === builtIn.things,
      `without ?liveworld=1 the store is left alone: the world built in (${parked.things} things)`);
const newer = await run({ ok: 1, version: 8, author: 'Test' }, { format: 99, version: 8, author: 'Test', files: { 'places/bar.json': fewer } });
check(/needs a newer engine/.test(newer.log) && newer.things === builtIn.things, 'a newer format is ignored (the world built in)');

await browser.close();
close();
console.log(failed ? 'LIVE WORLD TEST FAILED' : 'LIVE WORLD TEST PASSED');
process.exit(failed ? 1 : 0);
