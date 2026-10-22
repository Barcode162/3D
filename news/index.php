<?php
// The public news board (joshuavelasco.com/news, docs/LOOPED.md C4): the
// hub's news posts the owner switched to Public, newest first, with their
// screenshots. Everything else in the hub stays private.
//   GET /news/                  the page
//   GET /news/?json             the published posts as JSON
//   GET /news/?shot=<project>/<file>   a picture, only if a published post shows it
$data = getenv('HUB_DATA') ?: dirname(__DIR__, 2) . '/private/hub';

function read_json($path) {
  $s = @file_get_contents($path);
  $v = $s === false ? null : json_decode($s, true);
  return is_array($v) ? $v : null;
}
function published($data) {
  $names = ['bar3d' => 'Bar3D'];
  foreach ((read_json("$data/projects.json")['projects'] ?? []) as $p) $names[$p['id']] = $p['name'];
  $out = [];
  foreach (glob("$data/posts-*.json") ?: [] as $f) {
    if (!preg_match('#posts-([a-z0-9-]{1,24})\.json$#', $f, $m)) continue;
    foreach (read_json($f) ?? [] as $p) {
      if (empty($p['published'])) continue;
      $out[] = ['project' => $m[1], 'projectName' => $names[$m[1]] ?? $m[1], 'build' => $p['build'] ?? '',
                'title' => $p['title'] ?? '', 'text' => $p['text'] ?? '', 'at' => $p['publishedAt'] ?? ($p['at'] ?? 0),
                'shots' => array_map(fn($n) => "?shot={$m[1]}/" . rawurlencode($n), $p['shots'] ?? [])];
    }
  }
  usort($out, fn($a, $b) => $b['at'] <=> $a['at']);
  return $out;
}

if (isset($_GET['shot'])) {
  $f = is_string($_GET['shot']) ? $_GET['shot'] : '';
  $ok = false;
  if (preg_match('#^([a-z0-9-]{1,24})/([A-Za-z0-9_-]{1,60}\.(jpg|png))$#', $f, $m)) {
    foreach (published($data) as $p) if ($p['project'] === $m[1] && in_array("?shot={$m[1]}/" . rawurlencode($m[2]), $p['shots'], true)) $ok = true;
  }
  $path = $ok ? "$data/shots/{$m[1]}/{$m[2]}" : '';
  if (!$ok || !is_file($path)) { http_response_code(404); header('Content-Type: text/plain'); echo "Not found\n"; exit; }
  header('Content-Type: ' . ($m[3] === 'png' ? 'image/png' : 'image/jpeg'));
  header('Cache-Control: public, max-age=3600');
  readfile($path);
  exit;
}
if (isset($_GET['json'])) {
  header('Content-Type: application/json');
  header('Cache-Control: no-cache');
  header('Access-Control-Allow-Origin: *');
  echo json_encode(['ok' => 1, 'posts' => published($data)], JSON_UNESCAPED_UNICODE);
  exit;
}
$posts = published($data);
$e = fn($s) => htmlspecialchars($s, ENT_QUOTES);
?><!doctype html>
<html lang="en" data-look="faceplate">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0A0908">
<title>News</title>
<link rel="stylesheet" href="/ui/themes.css">
<link rel="stylesheet" href="/ui/looped.css">
<script src="/ui/looped.js" defer></script>
<style>
  /* The Faceplate look (docs/STUDIO.md §4.F, S4.4). */
  main { max-width: 640px; margin: 0 auto; padding: calc(env(safe-area-inset-top) + 28px) 16px 48px; }
  .top { display: flex; align-items: center; gap: 12px; }
  h1 { margin: 0; font-size: 48px; line-height: 1; }
  .sub { color: var(--text-2); margin: 8px 0 18px; }
  article { margin-top: 18px; border-radius: 18px; overflow: hidden; background: var(--panel); box-shadow: var(--panel-edge);
    animation: rise .5s cubic-bezier(.2,.9,.3,1.1) backwards; animation-delay: calc(var(--i) * 70ms); }
  @keyframes rise { from { opacity: 0; transform: translateY(16px); } }
  .shots { display: flex; overflow-x: auto; scroll-snap-type: x mandatory; scrollbar-width: none; background: #000; }
  .shots::-webkit-scrollbar { display: none; }
  .shots img { flex: none; width: 100%; aspect-ratio: 430 / 600; object-fit: cover; scroll-snap-align: center; display: block; }
  .body { padding: 14px 16px 18px; }
  .meta { color: var(--text-2); font-size: 12px; display: flex; gap: 8px; align-items: center; flex-wrap: wrap; font-stretch: 75%; font-weight: 650; letter-spacing: .1em; text-transform: uppercase; }
  .pill { padding: 3px 8px; border-radius: 6px; background: var(--field); box-shadow: var(--field-edge); font-family: var(--mono); letter-spacing: 0; text-transform: none; color: var(--accent); }
  h2 { margin: 10px 0 6px; font-size: 24px; line-height: 1.15; }
  p { margin: 0; white-space: pre-wrap; word-wrap: break-word; }
  .empty { text-align: center; color: var(--text-2); padding: 40px 10px; border-radius: 18px; background: var(--field); box-shadow: var(--field-edge); }
  a { color: var(--accent); }
  @media (prefers-reduced-motion: reduce) { * { animation: none !important; } }
</style>
</head>
<body class="lp">
<main>
  <div class="top"><span class="steps big" data-period="2" aria-hidden="true"><i></i><i></i><i></i><i></i></span><h1>News</h1></div>
  <p class="sub">What's new in Joshua's worlds. <a href="/">joshuavelasco.com</a></p>
<?php if (!$posts): ?>
  <div class="empty">No news yet. Check back soon.</div>
<?php endif; foreach ($posts as $i => $p): ?>
  <article style="--i:<?= min($i, 8) ?>">
<?php if ($p['shots']): ?>
    <div class="shots"><?php foreach ($p['shots'] as $s): ?><img src="<?= $e($s) ?>" alt="" loading="lazy"><?php endforeach; ?></div>
<?php endif; ?>
    <div class="body">
      <div class="meta"><span><?= $e($p['projectName']) ?></span><?php if ($p['build']): ?><span class="pill"><?= $e($p['build']) ?></span><?php endif; ?>
        <span><?= $e(date('M j, Y', (int)$p['at'])) ?></span></div>
      <h2><?= $e($p['title']) ?></h2>
      <p><?= $e($p['text']) ?></p>
    </div>
  </article>
<?php endforeach; ?>
</main>
</body>
</html>
