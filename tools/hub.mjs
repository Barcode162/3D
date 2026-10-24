// Claude's side of the news (hub/hub.php): drafts the user publishes from
// Dev → Publish. The old board's commands were taken out in restructure J5.4.
// Needs the Claude key in the environment as HUB_KEY.
//   node tools/hub.mjs post --build abc1234 --title ".." --text ".." [--shots a.png,b.png]   a news draft
//        (pictures are made JPEG, 1000 px at most, and uploaded first; the
//         owner's Publish switch puts it on joshuavelasco.com/news)
//   node tools/hub.mjs posts                 the news posts
// Every command takes --project <id> (default HUB_PROJECT, else bar3d).
// HUB_URL overrides the address (tests).
import { readFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { basename, extname } from 'node:path';
const URL_ = process.env.HUB_URL || 'https://joshuavelasco.com/hub/hub.php';
const KEY = process.env.HUB_KEY || '';
if (!KEY) {
  console.log('No HUB_KEY: the owner makes one (a new Claude key) and adds it to the environment.');
  process.exit(2);
}
const argv = process.argv.slice(2);
const PROJECT = (() => { const i = argv.indexOf('--project'); return i >= 0 ? argv.splice(i, 2)[1] : process.env.HUB_PROJECT || 'bar3d'; })();
const call = async (body) => {
  body.project ??= PROJECT;
  const r = await fetch(URL_, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain', 'X-Hub-Key': KEY } });
  const j = await r.json();
  if (!j.ok) { console.log('Hub: ' + (j.error || r.status)); process.exit(1); }
  return j;
};
const [cmd, ...args] = argv;
const flag = (name) => { const i = args.indexOf('--' + name); return i >= 0 ? args[i + 1] : undefined; };
const when = (at) => new Date(at * 1000).toISOString().slice(0, 16).replace('T', ' ');

if (cmd === 'post') {
  const build = flag('build') || '';
  const names = [];
  for (const file of (flag('shots') || '').split(',').map((x) => x.trim()).filter(Boolean)) {
    // JPEG, at most 1000 px tall or wide (python3 + PIL), else as it is.
    let data, type = 'jpeg';
    try {
      data = execFileSync('python3', ['-c', 'import sys,io;from PIL import Image;im=Image.open(sys.argv[1]).convert("RGB");im.thumbnail((1000,1000));b=io.BytesIO();im.save(b,"JPEG",quality=84);sys.stdout.buffer.write(b.getvalue())', file], { maxBuffer: 1 << 24 });
    } catch (e) {
      data = readFileSync(file);
      type = extname(file).toLowerCase() === '.png' ? 'png' : 'jpeg';
    }
    const name = basename(file, extname(file)).toLowerCase().replace(/[^a-z0-9_-]/g, '-').slice(0, 40);
    const r = await call({ a: 'shot', build, name, image: `data:image/${type};base64,` + data.toString('base64') });
    names.push(r.file);
  }
  const r = await call({ a: 'post', post: { build, title: flag('title') || '', text: flag('text') || '', shots: names } });
  console.log(`post ${r.post.id}: ${r.post.title} (${r.post.shots.length} pictures, private until the owner publishes)`);
} else if (cmd === 'posts') {
  const r = await call({ a: 'posts' });
  for (const p of r.posts) console.log(`${p.id}  ${when(p.at)}  ${p.build || '-------'}  ${p.published ? 'PUBLIC ' : 'private'}  ${p.title}  (${p.shots.length} pictures)`);
} else {
  console.log('Usage: node tools/hub.mjs [--project id] post --build <sha> --title ".." --text ".." [--shots a.png,b.png] | posts');
  process.exit(2);
}
