<?php
// Landing page for joshuavelasco.com (IONOS webspace folder "home/").
// Every project's own repo deploys into a subfolder here (home/bar/, ...)
// together with a project.json: {"name": ..., "description": ..., "order": n}.
// This page lists every subfolder that has one, so a new project appears
// as soon as its first deploy lands. Nothing here needs editing for that.
$projects = [];
foreach (glob(__DIR__ . '/*/project.json') as $file) {
  $info = json_decode(@file_get_contents($file), true);
  if (!is_array($info)) continue;
  $dir = basename(dirname($file));
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
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0d0f14">
<title>Joshua Velasco</title>
<style>
  :root { --bg: #0d0f14; --panel: #171a22; --line: #2a2f3c; --text: #eef0f5;
          --dim: #8b92a5; --accent: #f2b544; }
  * { box-sizing: border-box; }
  html, body { margin: 0; height: 100%; }
  body { background: radial-gradient(ellipse at 50% 35%, #1c2030 0%, var(--bg) 70%);
         color: var(--text); font: 16px/1.4 -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
         display: flex; align-items: center; justify-content: center; padding: 16px;
         -webkit-tap-highlight-color: transparent; }
  .wrap { width: min(100%, 360px); position: relative; }
  #toggle { width: 100%; padding: 18px 22px; border-radius: 16px; border: 1px solid var(--line);
            background: var(--panel); color: var(--text); font: inherit; font-size: 19px; font-weight: 600;
            display: flex; justify-content: space-between; align-items: center; cursor: pointer; }
  #toggle:active { transform: scale(0.98); }
  #toggle .arrow { color: var(--accent); transition: transform .2s; }
  #toggle[aria-expanded="true"] .arrow { transform: rotate(180deg); }
  #menu { list-style: none; margin: 10px 0 0; padding: 6px; border-radius: 16px;
          border: 1px solid var(--line); background: var(--panel);
          position: absolute; left: 0; right: 0; max-height: 60vh; overflow-y: auto; }
  #menu[hidden] { display: none; }
  #menu a { display: block; padding: 14px 16px; border-radius: 11px; color: var(--text); text-decoration: none; }
  #menu a:active, #menu a:hover { background: #222736; }
  #menu .name { font-weight: 600; }
  #menu .desc { color: var(--dim); font-size: 14px; margin-top: 2px; }
  #menu .empty { padding: 14px 16px; color: var(--dim); }
</style>
</head>
<body>
<div class="wrap">
  <button id="toggle" aria-expanded="false" aria-controls="menu">
    <span>Projects</span><svg class="arrow" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M6 9l6 6 6-6"/></svg>
  </button>
  <ul id="menu" hidden>
<?php if (!$projects): ?>
    <li class="empty">No projects yet.</li>
<?php endif; foreach ($projects as $p): ?>
    <li><a href="<?= h($p['url']) ?>">
      <div class="name"><?= h($p['name']) ?></div>
<?php if ($p['description'] !== ''): ?>
      <div class="desc"><?= h($p['description']) ?></div>
<?php endif; ?>
    </a></li>
<?php endforeach; ?>
  </ul>
</div>
<script>
  var toggle = document.getElementById('toggle'), menu = document.getElementById('menu');
  function setOpen(open) { menu.hidden = !open; toggle.setAttribute('aria-expanded', open); }
  toggle.addEventListener('click', function (e) { e.stopPropagation(); setOpen(menu.hidden); });
  document.addEventListener('click', function (e) { if (!menu.contains(e.target)) setOpen(false); });
  // Coming back with the Back button shouldn't leave the menu open.
  window.addEventListener('pageshow', function () { setOpen(false); });
</script>
</body>
</html>
