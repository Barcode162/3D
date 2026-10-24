<?php
// The hub's server (joshuavelasco.com/hub/hub.php): news posts and their
// screenshots, the Claude key, and the loops' tuning. The hub page itself
// became Dev (dev/); the old board (cards, sections, projects, places) was
// taken out in restructure J5.4 (its files on the server were left as they were).
//
// POST JSON (sent as text/plain, so browsers skip the CORS preflight), by the
// owner (the bar's login token) or Claude (the key: header X-Hub-Key, or "key"
// in the body). Posts and screenshots take "project" (default "bar3d").
//   {"a":"posts"}                    -> {"ok":1,"posts":[{id,build,title,text,shots:[{name,url}],at,published}]}
//   {"a":"post","post":{id?,build,title,text,shots:[names]}}   a news post (Claude writes drafts)
//   {"a":"publish","id":"..","on":1}  owner: show it on the public news board (joshuavelasco.com/news)
//   {"a":"unpost","id":".."}          owner: delete a post
//   {"a":"shot","build":"abc1234","name":"door","image":"data:image/jpeg;base64,.."}  a screenshot
//   {"a":"tuning"} / {"a":"tune","kind":"..","speed":0.25..4,"off":bool}   the loops' tuning (owner tunes)
//   {"a":"key"}                       owner: a new Claude key (shown once)
//   GET hub.php?shot=<project>/<file>  the picture (the owner's login cookie, or the Claude key)
//   GET hub.php?tuning                 the tuning (anyone)
// Logins are the bar's accounts (bar/account.php): the owner is the bar's
// owner (accounts/admins.json, else the first account made).
// Storage (outside the web root): private/hub/posts-<project>.json,
// shots/<project>/, tuning.json, key.json.

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-Hub-Key');

const TITLE_MAX = 100;
const TEXT_MAX = 6000;
const SHOT_MAX = 900000;
const POSTS_MAX = 200;

$accounts = getenv('HUB_ACCOUNTS') ?: dirname(__DIR__) . '/bar/accounts';
$data = getenv('HUB_DATA') ?: dirname(__DIR__, 2) . '/private/hub';
if (!is_dir($data)) @mkdir($data, 0755, true);

function reply($body, $code = 200) {
  header('Content-Type: application/json');
  header('Cache-Control: no-store');
  http_response_code($code);
  echo json_encode($body);
  exit;
}
function fail($message, $code = 400) { reply(['ok' => 0, 'error' => $message], $code); }
function read_json($path) {
  $s = @file_get_contents($path);
  $v = $s === false ? null : json_decode($s, true);
  return is_array($v) ? $v : null;
}
function write_json($path, $value) {
  $tmp = $path . '.tmp' . bin2hex(random_bytes(4));
  file_put_contents($tmp, json_encode($value, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));
  rename($tmp, $path);
}
function clean($s, $max) {
  if (!is_string($s)) return '';
  $s = preg_replace('/[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]/u', '', $s);
  return trim(mb_substr($s, 0, $max, 'UTF-8'));
}

// The owner's account for a bar login token, or null.
function owner_of_token($token) {
  global $accounts;
  if (!is_string($token) || !preg_match('/^[0-9a-f]{48}$/', $token)) return null;
  $t = read_json("$accounts/tokens/" . hash('sha256', $token) . '.json');
  $u = $t ? read_json("$accounts/users/{$t['id']}.json") : null;
  if (!$u) return null;
  $admins = read_json("$accounts/admins.json");
  $names = read_json("$accounts/names.json") ?? [];
  // Only the bar's owner (the first admin): admins chosen to invite people
  // don't get the hub.
  $owner = is_array($admins) ? (string)(array_values($admins)[0] ?? '') === $u['user']
                             : (count($names) && (string)array_key_first($names) === $u['user']);
  return $owner ? $u : null;
}

// Who's asking: the owner (bar login), or Claude (the key).
function who($in) {
  global $accounts, $data;
  $key = $_SERVER['HTTP_X_HUB_KEY'] ?? ($in['key'] ?? '');
  if (is_string($key) && $key !== '') {
    $k = read_json("$data/key.json");
    if ($k && hash_equals($k['hash'], hash('sha256', $key))) return ['role' => 'claude', 'name' => 'Claude'];
    usleep(300000);
    fail('That Claude key is not valid.', 401);
  }
  $token = $in['token'] ?? '';
  if (!is_string($token) || !preg_match('/^[0-9a-f]{48}$/', $token)) fail('Please log in.', 401);
  $t = read_json("$accounts/tokens/" . hash('sha256', $token) . '.json');
  $u = $t ? read_json("$accounts/users/{$t['id']}.json") : null;
  if (!$u) fail('Please log in again.', 401);
  $admins = read_json("$accounts/admins.json");
  $names = read_json("$accounts/names.json") ?? [];
  // Only the bar's owner (the first admin): admins chosen to invite people
  // don't get the hub.
  $owner = is_array($admins) ? (string)(array_values($admins)[0] ?? '') === $u['user']
                             : (count($names) && (string)array_key_first($names) === $u['user']);
  if (!$owner) fail('This hub is private.', 403);
  return ['role' => 'owner', 'name' => $u['name'] ?: $u['user']];
}


// The project a post belongs to: its id (letters, digits, dashes), "bar3d" when not given.
function project_id($in) {
  $p = $in['project'] ?? 'bar3d';
  if (!is_string($p) || !preg_match('/^[a-z0-9-]{1,24}$/', $p)) fail('No such project.', 404);
  return $p;
}

$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
if ($method === 'OPTIONS') exit;
// A screenshot of a news post: for the owner (login cookie) or Claude.
if ($method === 'GET' && isset($_GET['shot'])) {
  $f = is_string($_GET['shot']) ? $_GET['shot'] : '';
  if (!preg_match('#^([a-z0-9-]{1,24})/([A-Za-z0-9_-]{1,60}\.(jpg|png))$#', $f, $m)) fail('No such picture.', 404);
  $ok = false;
  $key = $_SERVER['HTTP_X_HUB_KEY'] ?? '';
  if (is_string($key) && $key !== '') {
    $k = read_json("$data/key.json");
    $ok = $k && hash_equals($k['hash'], hash('sha256', $key));
  } else {
    $ok = owner_of_token($_COOKIE['bar_t'] ?? '') !== null;
  }
  if (!$ok) fail('This hub is private.', 403);
  $path = "$data/shots/{$m[1]}/{$m[2]}";
  if (!is_file($path)) fail('No such picture.', 404);
  header('Content-Type: ' . ($m[3] === 'png' ? 'image/png' : 'image/jpeg'));
  header('Cache-Control: private, max-age=86400');
  readfile($path);
  exit;
}
// Live tuning of the bar's loops (docs/archive/ENGINE.md E8.2): anyone may read it
// (the bar does when it loads); only the owner changes it (action "tune").
if ($method === 'GET' && isset($_GET['tuning'])) {
  header('Content-Type: application/json');
  header('Cache-Control: no-store');
  echo json_encode(read_json("$data/tuning.json") ?? ['kinds' => new stdClass()]);
  exit;
}
if ($method !== 'POST') fail('POST only', 405);
$in = json_decode(file_get_contents('php://input'), true);
if (!is_array($in)) fail('Bad request.');
$a = $in['a'] ?? '';
$me = who($in);
$pid = project_id($in);

// ---- News posts (Home's feed; published ones on joshuavelasco.com/news) ----
function with_posts($pid, $fn) {
  global $data;
  $f = fopen("$data/posts-$pid.json", 'c+');
  flock($f, LOCK_EX);
  $posts = json_decode(stream_get_contents($f), true);
  if (!is_array($posts)) $posts = [];
  $result = $fn($posts);
  if ($result['_save'] ?? false) {
    ftruncate($f, 0);
    rewind($f);
    fwrite($f, json_encode(array_values($posts), JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));
    fflush($f);
  }
  unset($result['_save']);
  flock($f, LOCK_UN);
  fclose($f);
  return $result;
}
function post_out($pid, $p) {
  $p['shots'] = array_map(fn($n) => ['name' => $n, 'url' => "hub.php?shot=$pid/" . rawurlencode($n)], $p['shots'] ?? []);
  return $p;
}

if ($a === 'posts') {
  reply(with_posts($pid, function (&$posts) use ($pid) {
    $out = array_map(fn($p) => post_out($pid, $p), $posts);
    usort($out, fn($x, $y) => $y['at'] <=> $x['at']);
    return ['ok' => 1, 'posts' => $out];
  }));
}

if ($a === 'post') {
  $in_p = is_array($in['post'] ?? null) ? $in['post'] : fail('No post.');
  reply(with_posts($pid, function (&$posts) use ($in_p, $pid, $me) {
    $id = is_string($in_p['id'] ?? null) ? $in_p['id'] : '';
    $i = -1;
    foreach ($posts as $k => $p) if ($p['id'] === $id) $i = $k;
    if ($id !== '' && $i < 0) fail('That post is gone.', 404);
    if ($i < 0) {
      $posts[] = ['id' => bin2hex(random_bytes(5)), 'build' => '', 'title' => '', 'text' => '', 'shots' => [],
                  'at' => time(), 'published' => false, 'by' => $me['name']];
      $i = count($posts) - 1;
      if (count($posts) > POSTS_MAX) { array_shift($posts); $i--; }
    }
    $p = &$posts[$i];
    if (isset($in_p['build'])) $p['build'] = preg_replace('/[^0-9a-f]/', '', strtolower(clean($in_p['build'], 12)));
    if (isset($in_p['title'])) $p['title'] = clean($in_p['title'], TITLE_MAX);
    if (isset($in_p['text'])) $p['text'] = clean($in_p['text'], TEXT_MAX);
    if (isset($in_p['shots']) && is_array($in_p['shots'])) {
      $p['shots'] = array_values(array_filter(array_slice($in_p['shots'], 0, 12),
                                              fn($n) => is_string($n) && preg_match('/^[A-Za-z0-9_-]{1,60}\.(jpg|png)$/', $n)));
    }
    if ($p['title'] === '') $p['title'] = $p['build'] !== '' ? "Build {$p['build']}" : 'Update';
    $p['updated'] = time();
    $out = post_out($pid, $p);
    unset($p);
    return ['ok' => 1, 'post' => $out, '_save' => true];
  }));
}

if ($a === 'publish' || $a === 'unpost') {
  if ($me['role'] !== 'owner') fail('Only the owner publishes news.', 403);
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  reply(with_posts($pid, function (&$posts) use ($a, $id, $in, $pid) {
    foreach ($posts as $k => &$p) {
      if ($p['id'] !== $id) continue;
      if ($a === 'unpost') { array_splice($posts, $k, 1); return ['ok' => 1, '_save' => true]; }
      $p['published'] = !empty($in['on']);
      $p['publishedAt'] = $p['published'] ? time() : 0;
      return ['ok' => 1, 'post' => post_out($pid, $p), '_save' => true];
    }
    fail('That post is gone.', 404);
  }));
}

if ($a === 'shot') {
  $build = preg_replace('/[^0-9a-f]/', '', strtolower(is_string($in['build'] ?? null) ? $in['build'] : ''));
  $name = is_string($in['name'] ?? null) ? $in['name'] : '';
  if (!preg_match('/^[a-z0-9_-]{1,40}$/', $name) || strlen($build) > 12) fail('Bad picture name.');
  $img = $in['image'] ?? '';
  if (!is_string($img) || strlen($img) > SHOT_MAX * 4 / 3 + 64
      || !preg_match('#^data:image/(jpeg|png);base64,([A-Za-z0-9+/=]+)$#', $img, $m)) fail('That picture is too big or not a JPEG/PNG.');
  $bytes = base64_decode($m[2], true);
  $info = $bytes !== false ? @getimagesizefromstring($bytes) : false;
  if (!$info || !in_array($info[2], [IMAGETYPE_JPEG, IMAGETYPE_PNG], true)) fail('That isn\'t a picture.');
  $dir = "$data/shots/$pid";
  if (!is_dir($dir)) @mkdir($dir, 0755, true);
  $file = ($build !== '' ? "$build-" : '') . $name . ($info[2] === IMAGETYPE_PNG ? '.png' : '.jpg');
  file_put_contents("$dir/$file", $bytes);
  reply(['ok' => 1, 'file' => $file, 'url' => "hub.php?shot=$pid/$file"]);
}

if ($a === 'tuning') reply(['ok' => 1, 'tuning' => read_json("$data/tuning.json") ?? ['kinds' => new stdClass()]]);
if ($a === 'tune') {
  if ($me['role'] !== 'owner') fail('Only the owner tunes the bar.', 403);
  $kind = is_string($in['kind'] ?? null) ? $in['kind'] : '';
  if (!preg_match('/^[a-z0-9_-]{1,40}$/', $kind)) fail('Which kind?');
  $speed = is_numeric($in['speed'] ?? null) ? max(0.25, min(4.0, (float)$in['speed'])) : 1.0;
  $off = !empty($in['off']);
  $t = read_json("$data/tuning.json") ?? ['kinds' => []];
  if (!is_array($t['kinds'] ?? null)) $t['kinds'] = [];
  if (abs($speed - 1.0) < 0.001 && !$off) unset($t['kinds'][$kind]);
  else $t['kinds'][$kind] = ['speed' => round($speed, 2), 'off' => $off];
  if (!count($t['kinds'])) $t['kinds'] = new stdClass();
  write_json("$data/tuning.json", $t);
  reply(['ok' => 1, 'tuning' => $t]);
}

if ($a === 'key') {
  if ($me['role'] !== 'owner') fail('Only the owner makes Claude keys.', 403);
  $key = bin2hex(random_bytes(24));
  write_json("$data/key.json", ['hash' => hash('sha256', $key), 'made' => time()]);
  reply(['ok' => 1, 'key' => $key]);
}

fail('Unknown action.');
