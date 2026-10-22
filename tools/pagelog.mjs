// The bar's on-screen log for page options, for debugging:
//   node tools/pagelog.mjs "still&cam=..." [grep regex]
// Opens the bar (skipintro, server off), waits WAIT ms (default 4000), prints
// the log (only lines matching the regex, if given) and the errors.
import { serveSite } from './lib/site.mjs';
import { launchBrowser, logText, logErrors, openBar } from './lib/browser.mjs';

const [query = 'still', pattern] = process.argv.slice(2);
const site = serveSite();
const browser = await launchBrowser();
const page = await browser.newPage({ viewport: { width: 430, height: 932 }, deviceScaleFactor: 1 });
const consoleLines = [];
page.on('console', (m) => consoleLines.push(`[console.${m.type()}] ${m.text()}`));
page.on('pageerror', (e) => consoleLines.push(`[pageerror] ${e.message}`));
const loaded = await openBar(page, site.url, `skipintro&server=off&${query}`);
await page.waitForTimeout(+(process.env.WAIT ?? 4000));
const re = pattern ? new RegExp(pattern, 'i') : null;
const log = await logText(page).catch(() => '(no log)');
console.log(log.split('\n').filter((l) => !re || re.test(l)).join('\n'));
const errors = await logErrors(page).catch(() => []);
console.log(`\nloaded: ${loaded}; errors: ${errors.length}\n${errors.join('\n')}`);
if (process.env.CONSOLE) console.log(consoleLines.join('\n'));
await browser.close();
site.close();
