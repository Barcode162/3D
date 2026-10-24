<?php
// The UI store (docs/archive/UI_PLAN.md R1.7): the UI Editor's published themes and
// screens, every version kept. On IONOS at joshuavelasco.com/toolbelt/ui-editor/api.php;
// data outside the web root in private/ui/ (UI_DATA overrides, for tests).
// The repo's ui/system/ is the truth the site is built from; `node tools/uikit.mjs pull`
// brings a published version into it (ui/system/live-version.json says which).
//
// Public (no login):
//   GET ?live                 {"ok":1,"version":n,"at","author","note"} (version 0: nothing published yet)
//   GET ?bundle=n | ?bundle=live   a version (never changes: cached for a year)
// Editors (the owner and the admins, by bar login token; Claude by the hub's key, X-Hub-Key):
//   POST {"a":"me"}                                    who you are, and the live version
//   POST {"a":"publish","base":n,"note":"..","bundle":{..}}   becomes version live+1;
//                                                      refused if the live version isn't n anymore
//   POST {"a":"history"}                               every version: number, when, who, note
// A bundle: {"format":1,"files":{"themes/<id>.json":{..},"screens/<id>.json":{..}}}
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-Hub-Key');
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'OPTIONS') exit;
const BUNDLE_MAX = 1000000;    // bytes
const KEEP = 200;              // versions kept (older ones are deleted, never the live one)
const NOTE_MAX = 200;
$accounts = getenv('HUB_ACCOUNTS') ?: dirname(__DIR__, 2) . '/bar/accounts';
$hubData = getenv('HUB_DATA') ?: dirname(__DIR__, 3) . '/private/hub';
$data = getenv('UI_DATA') ?: dirname(__DIR__, 3) . '/private/ui';
foreach (["$data", "$data/versions"] as $d) if (!is_dir($d)) @mkdir($d, 0755, true);

function reply($body, $code = 200) {
  header('Content-Type: application/json');
  if (!headers_sent() && !in_array('Cache-Control', array_map(fn($h) => explode(':', $h)[0], headers_list()), true)) header('Cache-Control: no-store');
  http_response_code($code);
  echo json_encode($body, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
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
  file_put_contents($tmp, json_encode($value, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES));
  rename($tmp, $path);
}
function clean($s, $max) {
  if (!is_string($s)) return '';
  $s = preg_replace('/[\x00-\x08\x0B\x0C\x0E-\x1F\x7F]/u', '', $s);
  return trim(mb_substr($s, 0, $max, 'UTF-8'));
}
function live_meta() { global $data; return read_json("$data/live.json") ?? ['version' => 0]; }

// Only theme and screen files, each an object whose id matches its file name.
function check_bundle($b) {
  if (!is_array($b) || ($b['format'] ?? 0) !== 1 || !is_array($b['files'] ?? null) || !count($b['files'])) fail('That is not a UI bundle.');
  if (strlen(json_encode($b)) > BUNDLE_MAX) fail('That bundle is too big.');
  foreach ($b['files'] as $path => $f) {
    if (!preg_match('#^(themes|screens)/([a-z][a-z0-9-]{0,39})\.json$#', (string)$path, $m)) fail("Not a theme or screen file: $path");
    if (!is_array($f) || ($f['id'] ?? '') !== $m[2] || ($f['format'] ?? 0) !== 1) fail("$path: its id must be {$m[2]} and its format 1.");
    if ($m[1] === 'themes' && !is_array($f['tokens'] ?? null)) fail("$path: a theme needs its tokens.");
    if ($m[1] === 'screens' && !is_array($f['root'] ?? null)) fail("$path: a screen needs its root part.");
  }
  return $b;
}

function who($in) {
  global $accounts, $hubData;
  $key = $_SERVER['HTTP_X_HUB_KEY'] ?? ($in['key'] ?? '');
  if (is_string($key) && $key !== '') {
    $k = read_json("$hubData/key.json");
    if ($k && hash_equals($k['hash'], hash('sha256', $key))) return ['name' => 'Claude', 'role' => 'claude'];
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
  $list = is_array($admins) ? array_values($admins) : (count($names) ? [(string)array_key_first($names)] : []);
  if (!in_array($u['user'], array_map('strval', $list), true)) fail('The UI Editor is for the bar\'s owner and admins.', 403);
  return ['name' => $u['name'] ?: $u['user'], 'role' => (string)($list[0] ?? '') === $u['user'] ? 'owner' : 'admin'];
}

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'GET') {
  if (isset($_GET['live'])) reply(['ok' => 1] + live_meta());
  if (isset($_GET['bundle'])) {
    $v = $_GET['bundle'] === 'live' ? (int)live_meta()['version'] : (int)$_GET['bundle'];
    $b = $v > 0 ? read_json("$data/versions/$v.json") : null;
    if (!$b) fail('No such version.', 404);
    if ($_GET['bundle'] !== 'live') header('Cache-Control: public, max-age=31536000, immutable');
    reply($b);
  }
  fail('Ask for ?live or ?bundle=n.', 404);
}

$in = json_decode(file_get_contents('php://input'), true);
if (!is_array($in)) fail('Send JSON.');
$a = $in['a'] ?? '';
$me = who($in);
if ($a === 'me') reply(['ok' => 1, 'you' => $me['name'], 'role' => $me['role'], 'live' => live_meta()]);
if ($a === 'history') reply(['ok' => 1, 'versions' => array_reverse(read_json("$data/versions.json") ?? [])]);
if ($a === 'publish') {
  $bundle = check_bundle($in['bundle'] ?? null);
  $note = clean($in['note'] ?? '', NOTE_MAX);
  $fp = fopen("$data/.lock", 'c');
  flock($fp, LOCK_EX);
  $live = live_meta();
  if ((int)($in['base'] ?? -1) !== (int)$live['version']) { flock($fp, LOCK_UN); fail("Someone published version {$live['version']} since you started. Load it, then publish again.", 409); }
  $v = (int)$live['version'] + 1;
  $meta = ['version' => $v, 'at' => time(), 'author' => $me['name'], 'role' => $me['role'], 'note' => $note];
  write_json("$data/versions/$v.json", ['format' => 1, 'version' => $v, 'author' => $me['name'], 'at' => $meta['at'], 'note' => $note, 'files' => $bundle['files']]);
  $index = read_json("$data/versions.json") ?? [];
  $index[] = $meta;
  while (count($index) > KEEP) { $old = array_shift($index); @unlink("$data/versions/{$old['version']}.json"); }
  write_json("$data/versions.json", $index);
  write_json("$data/live.json", $meta);
  flock($fp, LOCK_UN);
  reply(['ok' => 1, 'live' => $meta]);
}
fail('Unknown action.');
