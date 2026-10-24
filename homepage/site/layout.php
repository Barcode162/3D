<?php
// The shared frame of joshuavelasco.com (homepage/, deployed to the IONOS
// "home/" folder): the header with its tabs, the footer, and who is looking.
// Every page does:
//   require __DIR__ . '/../site/layout.php';
//   site_head('engine', 'Looped, our engine', 'One line for search and link previews');
//   ... the page ...
//   site_foot();
//
// Who is looking: the bar's own login (account.php sets the cookie bar_t).
// Staff are the bar's owner and admins (accounts/admins.json, else the first
// account made), the same people the Studio lets in. Only staff see the
// Dev tab (the portal became Dev, PLAN.md R4.7), and portal/ answers everyone else with a plain 404.
if (!defined('SITE')) define('SITE', 1);

function site_h($s) { return htmlspecialchars((string)$s, ENT_QUOTES, 'UTF-8'); }

function site_json($path) {
  $s = @file_get_contents($path);
  $v = $s === false ? null : json_decode($s, true);
  return is_array($v) ? $v : null;
}

function site_accounts() { return getenv('SITE_ACCOUNTS') ?: dirname(__DIR__) . '/bar/accounts'; }

// The signed-in person, or null: ['id','user','name','avatar','role'],
// role = 'owner' | 'admin' | 'member'.
function site_viewer() {
  static $done = false, $viewer = null;
  if ($done) return $viewer;
  $done = true;
  $t = $_COOKIE['bar_t'] ?? '';
  if (!is_string($t) || !preg_match('/^[0-9a-f]{48}$/', $t)) return null;
  $base = site_accounts();
  $rec = site_json("$base/tokens/" . hash('sha256', $t) . '.json');
  $u = $rec && preg_match('/^[0-9a-f]{12}$/', (string)($rec['id'] ?? '')) ? site_json("$base/users/{$rec['id']}.json") : null;
  if (!$u || !isset($u['user'])) return null;
  $admins = site_json("$base/admins.json");
  if (!is_array($admins)) {
    $names = site_json("$base/names.json") ?? [];
    $admins = count($names) ? [(string)array_key_first($names)] : [];
  }
  $admins = array_map('strval', array_values($admins));
  $role = ($admins[0] ?? null) === $u['user'] ? 'owner' : (in_array($u['user'], $admins, true) ? 'admin' : 'member');
  $viewer = ['id' => (string)$u['id'], 'user' => (string)$u['user'], 'name' => (string)($u['name'] ?? $u['user']),
             'avatar' => (int)($u['avatar'] ?? 0), 'role' => $role];
  return $viewer;
}
function site_is_staff() { $v = site_viewer(); return $v && $v['role'] !== 'member'; }

// The tabs. Portal is added for staff only.
function site_tabs() {
  $tabs = [
    'home'     => ['/', 'Home'],
    'projects' => ['/projects/', 'Projects'],   // Bar3D (play/), Looped (engine/), where we're going (vision/): PLAN.md R5.4
    'news'     => ['/news/', 'News'],
  ];
  if (site_is_staff()) $tabs['portal'] = ['/dev/', 'Dev'];
  return $tabs;
}

function site_mark() {
  // The mark: a lit key with a loop in it (the engine is called Looped).
  return '<svg class="mark" viewBox="0 0 32 32" aria-hidden="true"><rect x="1" y="1" width="30" height="30" rx="8" fill="url(#mk)"/>'
    . '<path d="M8.4 16c0-2.2 1.6-4 3.6-4 3.2 0 4.8 8 8 8 2 0 3.6-1.8 3.6-4s-1.6-4-3.6-4c-3.2 0-4.8 8-8 8-2 0-3.6-1.8-3.6-4z" fill="none" stroke="#1C0E06" stroke-width="2.6" stroke-linecap="round"/>'
    . '<defs><linearGradient id="mk" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#FFB466"/><stop offset=".55" stop-color="#F28C2C"/><stop offset="1" stop-color="#C9600C"/></linearGradient></defs></svg>';
}

function site_head($active, $title, $description, $opts = []) {
  header('Cache-Control: no-cache');
  $v = site_viewer();
  $tabActive = in_array($active, ['play', 'engine', 'vision', 'projects'], true) ? 'projects' : $active;
  $full = $active === 'home' ? 'Joshua Velasco: worlds built from scratch' : "$title · Joshua Velasco";
  $bodyClass = 'pg-' . preg_replace('/[^a-z0-9-]/', '', $active) . (isset($opts['class']) ? ' ' . $opts['class'] : '');
  ?><!doctype html>
<html lang="en" data-look="ember" data-scheme="dark">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0E0906">
<meta name="color-scheme" content="dark">
<script>document.documentElement.classList.add('js')</script>
<title><?= site_h($full) ?></title>
<meta name="description" content="<?= site_h($description) ?>">
<meta property="og:title" content="<?= site_h($full) ?>">
<meta property="og:description" content="<?= site_h($description) ?>">
<meta property="og:image" content="/site/img/og.jpg">
<meta property="og:type" content="website">
<meta name="twitter:card" content="summary_large_image">
<link rel="icon" href="/site/img/mark.svg" type="image/svg+xml">
<link rel="apple-touch-icon" href="/site/img/touch.png">
<link rel="preload" href="/ui/fonts/Archivo.woff2" as="font" type="font/woff2" crossorigin>
<link rel="stylesheet" href="/ui/themes.css?v=2">
<link rel="stylesheet" href="/site/site.css?v=2">
</head>
<body class="<?= site_h($bodyClass) ?>">
<a class="skip" href="#main">Skip to content</a>
<header class="top" id="top">
  <div class="top-in wrap">
    <a class="brand" href="/" aria-label="Joshua Velasco, home"><?= site_mark() ?><span>Joshua Velasco</span></a>
    <nav class="tabs" aria-label="Main">
<?php foreach (site_tabs() as $key => [$href, $label]): ?>
      <a href="<?= site_h($href) ?>"<?= $key === $tabActive ? ' aria-current="page"' : '' ?><?= $key === 'portal' ? ' class="tab-portal"' : '' ?>><?= site_h($label) ?></a>
<?php endforeach; ?>
    </nav>
    <div class="top-act">
<?php if ($v): ?>
      <a class="who" href="<?= $v['role'] === 'member' ? '/bar/' : '/dev/' ?>" title="Signed in as @<?= site_h($v['user']) ?>"><span class="dot" aria-hidden="true"><?= site_h(mb_strtoupper(mb_substr($v['name'], 0, 1))) ?></span><span class="who-n"><?= site_h($v['name']) ?></span></a>
<?php else: ?>
      <a class="link-quiet signin-link" href="/signin/">Sign in</a>
<?php endif; ?>
      <a class="btn btn-go btn-sm" href="/bar/">Enter the bar</a>
      <button class="burger" type="button" aria-expanded="false" aria-controls="drawer" aria-label="Menu"><i></i><i></i></button>
    </div>
  </div>
  <div class="drawer" id="drawer" hidden>
    <nav class="wrap" aria-label="Menu">
<?php $i = 0; foreach (site_tabs() as $key => [$href, $label]): $i++; ?>
      <a href="<?= site_h($href) ?>"<?= $key === $tabActive ? ' aria-current="page"' : '' ?>><span class="num"><?= sprintf('%02d', $i) ?></span><?= site_h($label) ?></a>
<?php endforeach; ?>
<?php if ($v): ?>
      <button type="button" class="drawer-out" data-signout>Sign out @<?= site_h($v['user']) ?></button>
<?php else: ?>
      <a href="/signin/"><span class="num">→</span>Sign in</a>
<?php endif; ?>
      <a class="btn btn-go btn-lg" href="/bar/">Enter the bar</a>
    </nav>
  </div>
</header>
<main id="main">
<?php
}

function site_foot() {
  $v = site_viewer();
  $year = date('Y');
  ?>
</main>
<footer class="foot">
  <div class="wrap foot-in">
    <div class="foot-brand">
      <a class="brand" href="/"><?= site_mark() ?><span>Joshua Velasco</span></a>
      <p>Worlds built from the first line of code. Played in your browser, on your phone, right now.</p>
      <p class="live" data-live><span class="pulse" aria-hidden="true"></span><span data-live-text>Doors open</span></p>
    </div>
    <nav class="foot-col" aria-label="Explore">
      <span class="eyebrow">Explore</span>
      <a href="/projects/">Projects</a>
      <a href="/play/">Bar3D</a>
      <a href="/engine/">Looped, the engine</a>
      <a href="/vision/">Where we're going</a>
      <a href="/news/">News</a>
    </nav>
    <nav class="foot-col" aria-label="Play">
      <span class="eyebrow">Play</span>
      <a href="/bar/">Enter the bar</a>
      <a href="/play/#games">Games inside</a>
      <a href="/play/#join">How to join</a>
      <a href="/classic/">Bar3D classic</a>
    </nav>
    <nav class="foot-col" aria-label="Account">
      <span class="eyebrow">Account</span>
<?php if ($v): ?>
      <span class="foot-me">@<?= site_h($v['user']) ?></span>
<?php if ($v['role'] !== 'member'): ?>
      <a href="/dev/">Dev</a>
<?php endif; ?>
      <button type="button" class="link-btn" data-signout>Sign out</button>
<?php else: ?>
      <a href="/signin/">Sign in</a>
      <a href="/signin/#new">Make an account</a>
<?php endif; ?>
    </nav>
  </div>
  <div class="wrap foot-base">
    <span>© <?= $year ?> Joshua Velasco. Everything here was built in house.</span>
    <span class="mono">No outside engines were harmed.</span>
  </div>
</footer>
<script src="/site/site.js?v=1" defer></script>
</body>
</html>
<?php
}

// Line icons (24 px grid, drawn for this site).
function site_icon($name) {
  static $p = [
    'door'    => '<path d="M5 21V4.5A1.5 1.5 0 0 1 6.5 3h11A1.5 1.5 0 0 1 19 4.5V21M3 21h18M14.5 12.5v.01"/>',
    'people'  => '<circle cx="9" cy="8" r="3.2"/><path d="M3 20c.6-3.4 3-5.5 6-5.5s5.4 2.1 6 5.5"/><circle cx="17" cy="9" r="2.4"/><path d="M16.5 14.6c2.3.2 4 1.9 4.5 4.6"/>',
    'pad'     => '<path d="M7 8h10a5 5 0 0 1 4.9 6l-.6 3a2.6 2.6 0 0 1-4.6 1.1L15 16H9l-1.7 2.1A2.6 2.6 0 0 1 2.7 17l-.6-3A5 5 0 0 1 7 8z"/><path d="M7.5 11v3M6 12.5h3M15.5 12h.01M17.5 13.5h.01"/>',
    'bolt'    => '<path d="M13 2 4 14h7l-1 8 9-12h-7l1-8z"/>',
    'light'   => '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M2 12h2M20 12h2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/>',
    'sound'   => '<path d="M4 9v6M8 6v12M12 3v18M16 7v10M20 10v4"/>',
    'hand'    => '<path d="M8 13V5.5a1.5 1.5 0 0 1 3 0V11m0-1.5V4a1.5 1.5 0 0 1 3 0v7m0-5.5a1.5 1.5 0 0 1 3 0V13c0 4.4-2.6 8-7 8-2.6 0-4.3-1.3-5.6-3.4L3 14.4a1.5 1.5 0 0 1 2.5-1.7L8 15"/>',
    'phone'   => '<rect x="6" y="2.5" width="12" height="19" rx="3"/><path d="M10.5 18.5h3"/>',
    'layers'  => '<path d="m12 3 9 5-9 5-9-5 9-5z"/><path d="m3 13 9 5 9-5"/>',
    'globe'   => '<circle cx="12" cy="12" r="9"/><path d="M3 12h18M12 3c2.5 2.7 3.8 5.7 3.8 9s-1.3 6.3-3.8 9c-2.5-2.7-3.8-5.7-3.8-9S9.5 5.7 12 3z"/>',
    'lock'    => '<rect x="4.5" y="10.5" width="15" height="10.5" rx="2.5"/><path d="M8 10.5V7.5a4 4 0 0 1 8 0v3"/>',
    'cube'    => '<path d="m12 2.8 8 4.6v9.2l-8 4.6-8-4.6V7.4l8-4.6z"/><path d="m4 7.4 8 4.6 8-4.6M12 12v9.2"/>',
    'cursor'  => '<path d="m5 3 14 7-6 2-2 6L5 3z"/>',
    'chart'   => '<path d="M4 20V10M10 20V4M16 20v-7M22 20H2"/>',
    'news'    => '<rect x="3" y="4" width="18" height="16" rx="2.5"/><path d="M7 9h10M7 13h10M7 17h6"/>',
    'mic'     => '<rect x="9" y="3" width="6" height="11" rx="3"/><path d="M5.5 11a6.5 6.5 0 0 0 13 0M12 17.5V21"/>',
    'music'   => '<path d="M9 18V5l11-2v13"/><circle cx="6.5" cy="18" r="2.5"/><circle cx="17.5" cy="16" r="2.5"/>',
    'mirror'  => '<rect x="5" y="2.5" width="14" height="19" rx="7"/><path d="m9 9 3-3M9 13l6-6"/>',
    'flame'   => '<path d="M12 22c4 0 7-2.7 7-6.6 0-3.7-2.6-6-4.1-8.9-.4 2-1.6 3.4-2.9 3.9.3-3.2-1.2-6.4-4-8.4.3 3.5-3 6.3-3 10.4C5 18.8 8 22 12 22z"/>',
    'arrow'   => '<path d="M5 12h14M13 6l6 6-6 6"/>',
    'out'     => '<path d="M7 17 17 7M9 7h8v8"/>',
    'sliders' => '<path d="M4 6h10M18 6h2M4 12h4M12 12h8M4 18h12M20 18h0"/><circle cx="16" cy="6" r="2"/><circle cx="10" cy="12" r="2"/><circle cx="18" cy="18" r="2"/>',
    'pulse'   => '<path d="M2 12h4l3-7 4 14 3-7h6"/>',
    'key'     => '<circle cx="8" cy="15" r="4.5"/><path d="m11.2 11.8 8.3-8.3M16.5 6.5l2.5 2.5M14 9l2 2"/>',
    'spark'   => '<path d="M12 3v4M12 17v4M3 12h4M17 12h4M6 6l2.5 2.5M15.5 15.5 18 18M6 18l2.5-2.5M15.5 8.5 18 6"/>',
    'eye'     => '<path d="M2 12s3.6-7 10-7 10 7 10 7-3.6 7-10 7S2 12 2 12z"/><circle cx="12" cy="12" r="3"/>',
    'rocket'  => '<path d="M5 15c-1.5 1.5-2 5-2 5s3.5-.5 5-2M9 15l-3-3c1-4 4.5-9 13-9 0 8.5-5 12-9 13l-1-1z"/><circle cx="14.5" cy="9.5" r="1.6"/>',
  ];
  $d = $p[$name] ?? $p['spark'];
  return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">' . $d . '</svg>';
}

// J Production's projects: the one list, toolbelt/tools.json → projects
// (restructure J5.5; Dev and the news page read it too). Each project's
// "site" says how the site shows it (none: not on the site).
function site_project_list() {
  $file = getenv('SITE_TOOLS') ?: dirname(__DIR__) . '/toolbelt/tools.json';
  $out = [];
  foreach ((site_json($file)['projects'] ?? []) as $p) {
    if (!is_array($p['site'] ?? null)) continue;
    $s = $p['site'];
    $out[] = ['id' => (string)$p['id'], 'name' => (string)($s['name'] ?? $p['name']), 'place' => (string)($s['place'] ?? 'more'),
              'url' => (string)($s['href'] ?? ''), 'img' => (string)($s['img'] ?? ''), 'alt' => (string)($s['alt'] ?? ''),
              'icon' => (string)($s['icon'] ?? ''), 'tag' => (string)($s['tag'] ?? ''), 'description' => (string)($s['line'] ?? $p['about'])];
  }
  return $out;
}
// The small "More projects" list (the front page, Projects): the projects placed "more".
function site_projects() {
  return array_values(array_filter(site_project_list(), fn($p) => $p['place'] === 'more'));
}

// A plain "nothing here" page (also what non-staff get from portal/).
function site_404() {
  http_response_code(404);
  site_head('lost', 'Not found', 'Nothing here.');
  ?>
<section class="wrap lost">
  <span class="eyebrow plain">Error 404</span>
  <p class="code">404</p>
  <h1 class="h-md">Nothing behind this door.</h1>
  <p class="dim">The page you wanted isn't here. The bar is, though.</p>
  <div class="hero-cta" style="justify-content:center"><a class="btn btn-go btn-lg" href="/bar/">Enter the bar <span class="arr">→</span></a><a class="btn btn-ghost btn-lg" href="/">Home</a></div>
</section>
<?php
  site_foot();
  exit;
}
