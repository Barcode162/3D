<?php
// The front door of joshuavelasco.com (IONOS webspace folder "home/";
// docs/STUDIO.md S1.2): three doors (Bar, Studio, News) in the shared look
// (/ui/), then every other project: each subfolder with a project.json
// ({"name", "description", "order"}) is listed by itself.
$doors = ['bar', 'studio', 'news'];
$projects = [];
foreach (glob(__DIR__ . '/*/project.json') as $file) {
  $info = json_decode(@file_get_contents($file), true);
  if (!is_array($info)) continue;
  $dir = basename(dirname($file));
  if (in_array($dir, $doors, true)) continue;
  $projects[] = [
    'url' => '/' . rawurlencode($dir) . '/',
    'name' => (string)($info['name'] ?? $dir),
    'description' => (string)($info['description'] ?? ''),
    'order' => (int)($info['order'] ?? 100),
  ];
}
usort($projects, fn($a, $b) => [$a['order'], $a['name']] <=> [$b['order'], $b['name']]);
header('Cache-Control: no-cache');
function h($s) { return htmlspecialchars($s, ENT_QUOTES, 'UTF-8'); }
?><!doctype html>
<html lang="en" data-look="faceplate">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0A0908">
<title>Joshua Velasco</title>
<link rel="stylesheet" href="/ui/themes.css">
<link rel="stylesheet" href="/ui/looped.css">
<style>
  main { max-width: 520px; margin: 0 auto; min-height: 100vh; display: grid; align-content: center; gap: 22px;
         padding-block: calc(env(safe-area-inset-top, 0px) + 36px) calc(env(safe-area-inset-bottom, 0px) + 36px); }
  h1 { font-size: clamp(46px, 13vw, 76px); line-height: .95; }
  .doors { display: grid; gap: 10px; }
  .doors .key { justify-content: flex-start; text-align: left; text-transform: none; letter-spacing: 0; font-stretch: 100%;
                min-height: 74px; padding: 12px 14px; gap: 14px; width: 100%; }
  .sq { width: 42px; height: 42px; border-radius: 8px; flex: none; background: linear-gradient(180deg, rgba(255,255,255,.35), rgba(255,255,255,0) 55%), var(--c);
        box-shadow: inset 0 1px 0 rgba(255,255,255,.35), 0 0 16px color-mix(in srgb, var(--c) 45%, transparent); }
  :root[data-look="backbar"] .sq, :root[data-look="jukebox"] .sq { border-radius: 50%; }
  .t { flex: 1; display: grid; }
  .t b { font-family: var(--display); font-weight: var(--display-w); font-stretch: var(--display-s); font-size: 21px; color: var(--text); }
  .t span { color: var(--text-2); font-size: 14px; font-weight: 500; }
  .doors .key svg { width: 20px; height: 20px; color: var(--text-2); }
  .more { display: grid; gap: 8px; }
  .more a { display: flex; justify-content: space-between; gap: 12px; padding: 12px 14px; text-decoration: none; }
  .more small { color: var(--text-2); }
</style>
</head>
<body class="lp">
<main class="gutter">
  <span class="steps big" data-period="3" aria-hidden="true"><i></i><i></i><i></i><i></i></span>
  <h1>Joshua Velasco</h1>
  <p class="dim">A bar you can walk into, the studio it's built in, and what changed this week.</p>
  <nav class="doors" aria-label="Doors">
    <a class="key" href="/bar/"><span class="sq" style="--c:var(--c1)"></span><span class="t"><b>Bar</b><span>Come in, sit down, play</span></span><svg class="ico"><use href="/ui/icons.svg#i-arrow"/></svg></a>
    <a class="key" href="/studio/"><span class="sq" style="--c:var(--c2)"></span><span class="t"><b>Studio</b><span>Build and change the world</span></span><svg class="ico"><use href="/ui/icons.svg#i-arrow"/></svg></a>
    <a class="key" href="/news/"><span class="sq" style="--c:var(--c3)"></span><span class="t"><b>News</b><span>What changed, with pictures</span></span><svg class="ico"><use href="/ui/icons.svg#i-arrow"/></svg></a>
  </nav>
<?php if ($projects): ?>
  <section class="more" aria-label="More projects">
    <span class="label">More</span>
<?php foreach ($projects as $p): ?>
    <a class="field" href="<?= h($p['url']) ?>"><span><?= h($p['name']) ?></span><small><?= h($p['description']) ?></small></a>
<?php endforeach; ?>
  </section>
<?php endif; ?>
</main>
<script src="/ui/looped.js"></script>
</body>
</html>
