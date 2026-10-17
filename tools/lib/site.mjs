// Serves a folder (default build/site) over HTTP on a free port, for the
// headless tests. serveSite() → { port, url, close() }.
import { createServer } from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, resolve, sep } from 'node:path';

const types = {
  '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm', '.json': 'application/json',
  '.png': 'image/png', '.jpg': 'image/jpeg', '.ogg': 'audio/ogg', '.m4a': 'audio/mp4', '.mp3': 'audio/mpeg',
};

export function serveSite(dir = 'build/site') {
  const root = resolve(dir);
  const server = createServer(async (req, res) => {
    const path = decodeURIComponent(new URL(req.url, 'http://x').pathname);
    const file = resolve(join(root, path === '/' ? 'index.html' : path));
    if (file !== root && !file.startsWith(root + sep)) { res.writeHead(403).end(); return; }
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
