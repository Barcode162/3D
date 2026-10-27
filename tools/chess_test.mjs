// The lounge's chessboard (upgrade plan item 13): sits down at the board
// (?chess); the full-screen board opens on the opponent picker (J28: five
// bots with their ratings, and a person), picks Rosa; then, back in the 3D
// view, taps e2 then e4 on the 3D board, checks the move happened and the
// computer answered. Screenshots: build/chess-picker.png, build/chess.png.
// Usage: NODE_PATH=$(npm root -g) node tools/chess_test.mjs
import { launchBrowser, logText, logErrors, openBar } from './lib/browser.mjs';
import { serveSite } from './lib/site.mjs';

const results = [];
const check = (name, ok) => { results.push(ok); console.log(`${ok ? 'ok  ' : 'FAIL'} ${name}`); };
const W = 430, H = 932;
const { url, close } = serveSite();
const browser = await launchBrowser();
const page = await browser.newPage({ viewport: { width: W, height: H } });
check('the bar loads', await openBar(page, url, 'skipintro&chess'));
await page.waitForTimeout(2500);
// The picker (J28.2): every bot with its rating, and a person.
const pick = await page.waitForSelector('#chessFull .cfPick', { timeout: 10000 }).then(() => true, () => false);
const rows = pick ? await page.$$eval('#chessFull .cfBot', (b) => b.map((x) => x.textContent)) : [];
check(`the full-screen board opens on the picker (${rows.length} choices)`, pick && rows.length === 6);
check('each bot shows its rating', rows.slice(0, 5).every((t) => /\d+ Elo/.test(t)) && /person/i.test(rows[5] || ''));
await page.screenshot({ path: 'build/chess-picker.png' });
if (pick) await page.click('#chessFull .cfBot >> nth=1');
const picked = await page.waitForFunction(() => /Chess: a new game against Rosa/.test(document.getElementById('log').textContent),
                                          null, { timeout: 5000 }).then(() => true, () => false);
check('picking Rosa starts a game against her', picked);
await page.click('#chessFull button:has-text("3D view")').catch(() => {});
await page.waitForTimeout(2500);  // the camera settles over the board
// Where a square is on screen: the game's chess camera (bar/projects/chess/chess.cpp):
// above white's edge, 0.5 m up and 0.38 m back, pitched down 0.92 rad, 70 deg tall.
function screenOf(file, rank) {
  const sq = 0.055, p = [(file - 3.5) * sq, 0, (3.5 - rank) * sq], e = [0, 0.5, 0.38];
  const d = [p[0] - e[0], p[1] - e[1], p[2] - e[2]];
  const pitch = -0.92, fwd = [0, Math.sin(pitch), -Math.cos(pitch)], right = [1, 0, 0];
  const up = [right[1] * fwd[2] - right[2] * fwd[1], right[2] * fwd[0] - right[0] * fwd[2], right[0] * fwd[1] - right[1] * fwd[0]];
  const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  const z = dot(d, fwd), th = Math.tan((70 * Math.PI / 180) / 2);
  return { x: (dot(d, right) / z / (th * W / H) * 0.5 + 0.5) * W, y: (0.5 - dot(d, up) / z / th * 0.5) * H };
}
const e2 = screenOf(4, 1), e4 = screenOf(4, 3);
await page.mouse.click(e2.x, e2.y);
await page.waitForTimeout(400);
await page.screenshot({ path: 'build/chess-selected.png' });
await page.mouse.click(e4.x, e4.y);
const replied = await page.waitForFunction(() => /Chess: the computer plays/.test(document.getElementById('log').textContent),
                                           null, { timeout: 10000 }).then(() => true, () => false);
check('tapping e2 then e4 moves the pawn, and the computer answers', replied);
await page.waitForTimeout(800);
await page.screenshot({ path: 'build/chess.png' });
console.log((await logText(page)).split('\n').filter((l) => /Chess/.test(l)).join('\n'));
const errors = await logErrors(page);
check(`no errors${errors.length ? ': ' + errors.join(' | ') : ''}`, errors.length === 0);
await browser.close();
close();
const ok = results.every(Boolean);
console.log(ok ? 'CHESS GAME TEST PASSED' : 'CHESS GAME TEST FAILED');
process.exit(ok ? 0 : 1);
