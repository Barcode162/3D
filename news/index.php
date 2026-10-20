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
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0b0716">
<title>News</title>
<style>
  :root { --bg: #0b0716; --ink: #f6f0ff; --dim: #b4a7d4; --edge: rgba(255,255,255,.14); --glass: rgba(255,255,255,.06);
    --pink: #ff5ab4; --gold: #ffb35a; --cyan: #5ad1ff; --violet: #b38bff; }
  * { box-sizing: border-box; }
  html, body { margin: 0; background: var(--bg); color: var(--ink);
    font: 16px/1.5 -apple-system, BlinkMacSystemFont, "SF Pro Rounded", "Segoe UI", system-ui, sans-serif; }
  .sky { position: fixed; inset: -25%; z-index: 0; filter: blur(70px); opacity: .5; pointer-events: none; }
  .sky i { position: absolute; width: 50vmax; height: 50vmax; border-radius: 50%; mix-blend-mode: screen; animation: drift 24s ease-in-out infinite alternate; }
  .sky i:nth-child(1) { background: var(--pink); left: -5%; top: 5%; }
  .sky i:nth-child(2) { background: var(--violet); right: -5%; top: -5%; animation-delay: -9s; }
  .sky i:nth-child(3) { background: var(--cyan); left: 20%; bottom: -10%; animation-delay: -15s; opacity: .6; }
  @keyframes drift { to { transform: translate(7vmax, -5vmax) scale(1.18) rotate(30deg); } }
  main { position: relative; z-index: 1; max-width: 640px; margin: 0 auto; padding: calc(env(safe-area-inset-top) + 28px) 16px 48px; }
  h1 { margin: 0; font-size: 46px; font-weight: 850; line-height: 1; }
  h1 span { display: inline-block; background: linear-gradient(180deg, #fff, var(--pink) 65%, var(--violet)); -webkit-background-clip: text;
    background-clip: text; color: transparent; animation: wave 2.6s ease-in-out infinite; animation-delay: calc(var(--i) * .11s); }
  @keyframes wave { 25% { transform: translateY(-7px) rotate(-3deg); } 50% { transform: none; } 75% { transform: translateY(2px) rotate(2deg); } }
  .sub { color: var(--dim); margin: 8px 0 18px; }
  article { margin-top: 18px; border-radius: 22px; overflow: hidden; background: var(--glass); border: 1px solid var(--edge);
    box-shadow: 0 20px 50px rgba(0,0,0,.35); animation: rise .6s cubic-bezier(.2,.9,.3,1.15) backwards; animation-delay: calc(var(--i) * 80ms); }
  @keyframes rise { from { opacity: 0; transform: translateY(20px) scale(.97); } }
  .shots { display: flex; overflow-x: auto; scroll-snap-type: x mandatory; scrollbar-width: none; background: #000; }
  .shots::-webkit-scrollbar { display: none; }
  .shots img { flex: none; width: 100%; aspect-ratio: 430 / 600; object-fit: cover; scroll-snap-align: center; display: block; }
  .body { padding: 14px 16px 18px; }
  .meta { color: var(--dim); font-size: 13px; display: flex; gap: 8px; align-items: center; flex-wrap: wrap; }
  .pill { padding: 3px 10px; border-radius: 999px; border: 1px solid var(--edge); background: rgba(0,0,0,.3); font-family: ui-monospace, Menlo, monospace; }
  h2 { margin: 8px 0 6px; font-size: 22px; line-height: 1.2; background: linear-gradient(90deg, #fff, var(--cyan)); -webkit-background-clip: text;
    background-clip: text; color: transparent; }
  p { margin: 0; white-space: pre-wrap; word-wrap: break-word; color: #e6dcff; }
  .empty { text-align: center; color: var(--dim); padding: 40px 10px; border: 1px dashed var(--edge); border-radius: 20px; }
  a { color: var(--cyan); }
  @media (prefers-reduced-motion: reduce) { * { animation: none !important; } }
</style>
</head>
<body>
<div class="sky"><i></i><i></i><i></i></div>
<main>
  <h1 aria-label="News"><?php foreach (str_split('News') as $i => $c) echo "<span style=\"--i:$i\">$c</span>"; ?></h1>
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
