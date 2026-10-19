// 4K Mania (upgrade plan item 7): opens the song select (upright), checks
// it lists the game's own songs, then plays one on autoplay for a while and
// checks the chart was made and every note so far was a MAX. Headless
// Chromium can't decode AAC (.m4a; Safari can), so the test hands it an Ogg
// copy of the song (made with ffmpeg) instead.
// Saves build/rhythm-menu.png and build/rhythm-play.png.
// Usage: NODE_PATH=$(npm root -g) node tools/rhythm_test.mjs
import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, readFileSync } from 'node:fs';
import { launchBrowser, logText, logErrors, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const results = [];
const check = (name, ok) => { results.push(ok); console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}`); };
mkdirSync('build', { recursive: true });
const ogg = 'build/rhythm-test-song.ogg';
if (!existsSync(ogg)) {
  execFileSync('ffmpeg', ['-loglevel', 'error', '-y', '-i', 'assets/music/rhythm/run-amok.m4a', '-t', '40', '-c:a', 'libopus',
                          '-b:a', '48k', ogg]);
}
const song = readFileSync(ogg);
const { url, close } = serveSite();
const browser = await launchBrowser();

// The song select, from the cabinet (stand in front of it, tap it).
let page = await browser.newPage({ viewport: { width: 430, height: 932 } });
check('the bar loads', await openBar(page, url, 'skipintro&cam=7.0,1.65,-5.9,135,-8'));
await page.waitForTimeout(1500);
await page.mouse.click(215, 466);
await page.waitForTimeout(300);
await page.mouse.click(215, 466);
await page.waitForTimeout(1500);
await page.screenshot({ path: 'build/rhythm-menu.png' });
const opened = /Rhythm game opened \((\d+) songs\)/.exec(await logText(page));
check(`tapping the cabinet opens the song select with its own songs (${opened ? opened[1] : 0})`, opened && +opened[1] >= 12);
await page.close();

// A song on autoplay (HARD).
page = await browser.newPage({ viewport: { width: 430, height: 932 } });
await page.route('**/assets/music/rhythm/*.m4a', (r) => r.fulfill({ body: song, contentType: 'audio/ogg' }));
await openBar(page, url, 'skipintro&rhythm=2,2&autoplay');
const charted = await page.waitForFunction(() => /Chart made/.test(document.getElementById('log').textContent), null,
                                           { timeout: 60000 }).then(() => true, () => false);
check('the song is analyzed into a chart', charted);
await page.mouse.click(215, 700);  // (sound starts after a tap, like on the phone)
await page.waitForTimeout(14000);
await page.screenshot({ path: 'build/rhythm-play.png' });
const playing = await page.evaluate(() => {
  const c = document.querySelector('canvas');
  return !!c;
});
check('it plays', playing);
const errors = await logErrors(page);
check(`no errors${errors.length ? ': ' + errors.join(' | ') : ''}`, errors.length === 0);
await browser.close();
close();
const ok = results.every(Boolean);
console.log(ok ? 'RHYTHM TEST PASSED' : 'RHYTHM TEST FAILED');
process.exit(ok ? 0 : 1);
