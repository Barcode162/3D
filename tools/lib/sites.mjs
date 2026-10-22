// Serves several folders at their site paths, like joshuavelasco.com:
// /bar/ → build/site, /studio/ → studio/, /ui/ → ui/ (the Studio's tests).
// serveSites({ '/bar/': 'build/site', ... }) → { url, close() }
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, resolve, sep } from 'node:path';

const types = {
  '.html': 'text/html', '.css': 'text/css', '.js': 'text/javascript', '.mjs': 'text/javascript', '.wasm': 'application/wasm',
  '.json': 'application/json', '.png': 'image/png', '.jpg': 'image/jpeg', '.ogg': 'audio/ogg', '.m4a': 'audio/mp4',
  '.mp3': 'audio/mpeg', '.svg': 'image/svg+xml', '.woff2': 'font/woff2', '.gz': 'application/gzip', '.ktx2': 'application/octet-stream',
};

export function serveSites(map) {
  const mounts = Object.entries(map).map(([prefix, dir]) => [prefix, resolve(dir)]).sort((a, b) => b[0].length - a[0].length);
  const server = createServer(async (req, res) => {
    const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
    const m = mounts.find(([p]) => path.startsWith(p));
    if (!m) { res.writeHead(404).end(); return; }
    const rel = path.slice(m[0].length);
    const file = resolve(join(m[1], rel === '' || rel.endsWith('/') ? rel + 'index.html' : rel));
    if (file !== m[1] && !file.startsWith(m[1] + sep)) { res.writeHead(403).end(); return; }
    try {
      const body = await readFile(file);
      res.writeHead(200, { 'content-type': types[extname(file)] ?? 'application/octet-stream' });
      res.end(body);
    } catch {
      res.writeHead(404).end();
    }
  }).listen(0);
  const port = server.address().port;
  return { port, url: `http://localhost:${port}/`, close: () => server.close() };
}
