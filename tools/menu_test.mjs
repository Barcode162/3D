// Menu test: the HTML menus and the game talk through the page mailbox.
// Opens the menu, wears another look, adds a MIDI file whose name has a
// backslash and quotes (it must reach the piano's song list unbroken), does
// an emote in front of the mirror (build/menu-emote.png), and checks that
// the menu and an account card never cover the game together.
// Usage: NODE_PATH=$(npm root -g) node tools/menu_test.mjs
import { readFileSync } from 'node:fs';
import { serveSite } from './lib/site.mjs';
import { launchBrowser, logErrors, openBar } from './lib/browser.mjs';
import { SPOTS } from './lib/spots.mjs';

const site = serveSite();
const browser = await launchBrowser();
const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
let failed = false;
const check = (ok, what) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${what}`); if (!ok) failed = true; };
const until = async (fn, ms = 20000) => {
  for (let t = 0; t < ms; t += 250) { if (await page.evaluate(fn)) return true; await page.waitForTimeout(250); }
  return false;
};

await openBar(page, site.url, `skipintro&server=off&cam=${SPOTS.mirror}`);
await page.waitForTimeout(1500);
check(await until(() => (bar3dUi.info().looks || []).length > 1 && (bar3dUi.info().songs || []).length >= 10),
      'the game sent the menu its lists (looks, songs)');

// Wear another look.
await page.click('#hudMenu');
await page.click('.mTile:has-text("Avatar")');
await page.click('.mGrid .mTile >> nth=2');
check(await until(() => bar3dUi.info().look === 2), 'the game wears look 2 and tells the menu');
await page.click('.mX');  // the avatar panel stays open for trying looks

// A MIDI file with a backslash and quotes in its name.
const before = await page.evaluate(() => bar3dUi.info().songs.length);
await page.click('#hudMenu');
await page.click('.mTile:has-text("Piano")');
await page.setInputFiles('#midiPicker', { name: 'my \\ "odd" song.mid', mimeType: 'audio/midi',
                                          buffer: readFileSync('assets/midi/01-fur-elise.mid') });
check(await until(() => bar3dUi.info().songs.slice(-1)[0] === 'my \\ "odd" song', 30000),
      `the file reaches the piano's list unbroken (${before} → ${await page.evaluate(() => bar3dUi.info().songs.length)} songs)`);

// One panel at a time: opening a card closes the menu, and the game knows.
await page.click('#hudMenu');
check(await page.evaluate(() => bar3dModal.name()) === 'menu', 'menu open (modal manager)');
await page.evaluate(() => bar3dAccount.showCard('', 'Bob'));
check(await page.evaluate(() => bar3dModal.name() === 'account' && !bar3dUi.isOpen()), 'a card replaces the menu');
await page.evaluate(() => bar3dModal.open('x', () => {}));  // anything else closes the card
check(await page.evaluate(() => !bar3dAccount.isOpen()), 'and the card closes when something else opens');
await page.evaluate(() => bar3dModal.closed('x'));

// An emote, seen in the mirror.
await page.click('#hudMenu');
await page.click('.mTile:has-text("Emotes")');
await page.click('.mGrid .mTile >> nth=0');
await page.waitForTimeout(1500);
await page.screenshot({ path: 'build/menu-emote.png' });
check(!(await page.evaluate(() => bar3dModal.isOpen())), 'the menu closed after the emote');

const errors = await logErrors(page);
check(errors.length === 0, 'no errors in the log' + (errors.length ? ':\n' + errors.join('\n') : ''));
await browser.close();
site.close();
console.log(failed ? 'MENU TEST FAILED' : 'MENU TEST PASSED');
process.exit(failed ? 1 : 0);
