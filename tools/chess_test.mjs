// The lounge's chessboard (upgrade plan item 13): sits down at the board
// (?chess), taps e2 then e4 on the 3D board, checks the move happened and
// the computer answered. Screenshots: build/chess.png (after e4 and the reply).
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
