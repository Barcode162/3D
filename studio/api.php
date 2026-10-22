<?php
// The world store (docs/STUDIO.md 7.2): the live world every phone and the
// room load, its versions, and each editor's draft. On IONOS at
// joshuavelasco.com/studio/api.php; data outside the web root in
// private/studio/ (STUDIO_DATA overrides, for tests).
//
// Public (no login):
//   GET ?live                 {"ok":1,"version":n,"at","author","note"} (version 0: none yet → built-in world)
//   GET ?bundle=n | ?bundle=live   the bundle itself (a version never changes: cached for a year)
// Editors (the owner and the admins, by bar login token; Claude by the hub's key, X-Hub-Key):
//   POST {"a":"me"}                         who you are
//   POST {"a":"draft"}                      your draft (or null) and the live version
//   POST {"a":"save","bundle":{..},"base":n}   save your draft (started from version n)
//   POST {"a":"discard"}                    drop your draft
//   POST {"a":"publish","base":n,"note":"..","bundle":{..}?}  your draft (or the bundle given) becomes
//                                           version live+1; refused if the live version isn't n anymore
//   POST {"a":"history"}                    every version: number, when, who, note
//   POST {"a":"restore","v":n,"note":".."}  version n again, as a new version
// A bundle: {"format":1,"files":{"places/bar.json":{..},"kinds/bar.json":{..},"catalog.json":{..},"songs.json":{..}}}
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-Hub-Key');
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'OPTIONS') exit;

const FORMAT_MAX = 1;          // the newest bundle format this store accepts
const BUNDLE_MAX = 2000000;    // bytes
const KEEP = 100;              // versions kept (older ones are deleted, never the live one)
const NOTE_MAX = 200;

$accounts = getenv('HUB_ACCOUNTS') ?: dirname(__DIR__) . '/bar/accounts';
$hubData = getenv('HUB_DATA') ?: dirname(__DIR__, 2) . '/private/hub';
$data = getenv('STUDIO_DATA') ?: dirname(__DIR__, 2) . '/private/studio';
foreach (["$data", "$data/versions", "$data/drafts"] as $d) if (!is_dir($d)) @mkdir($d, 0755, true);

function reply($body, $code = 200) {
  header('Content-Type: application/json');
  header('Cache-Control: no-store');
  http_response_code($code);
  echo json_encode($body, JSON_UNESCAPED_UNICODE);
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
function live_meta() {
  global $data;
  return read_json("$data/live.json") ?? ['version' => 0, 'at' => 0, 'author' => '', 'note' => 'The world built into the game'];
}
function index_list() { global $data; return read_json("$data/versions.json") ?? []; }

// A bundle is only data: known file names, objects inside, a size cap.
function check_bundle($b) {
  if (!is_array($b) || !isset($b['files']) || !is_array($b['files']) || !count($b['files'])) fail('A world needs its files.');
  $format = $b['format'] ?? 1;
  if (!is_int($format) || $format < 1 || $format > FORMAT_MAX) fail('This world needs a newer studio.');
  foreach ($b['files'] as $name => $content) {
    if (!is_string($name) || !preg_match('#^((places|kinds)/[a-z0-9_-]{1,40}\.json|catalog\.json|songs\.json|schema\.json|materials\.json)$#', $name)) fail("Unknown file \"$name\".");
    if (!is_array($content)) fail("$name isn't a world file.");
  }
  if (!isset($b['files']['places/bar.json'])) fail('The world needs the bar (places/bar.json).');
  $size = strlen(json_encode($b));
  if ($size > BUNDLE_MAX) fail('This world is too big (' . round($size / 1e6, 1) . ' MB, the most is 2 MB).');
  return ['format' => $format, 'files' => $b['files']];
}

// Who's asking: the owner or an admin (bar login), or Claude (the hub's key).
function who($in) {
  global $accounts, $hubData;
  $key = $_SERVER['HTTP_X_HUB_KEY'] ?? ($in['key'] ?? '');
  if (is_string($key) && $key !== '') {
    $k = read_json("$hubData/key.json");
    if ($k && hash_equals($k['hash'], hash('sha256', $key))) return ['id' => 'claude', 'name' => 'Claude', 'role' => 'claude'];
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
  if (!in_array($u['user'], array_map('strval', $list), true)) fail('The Studio is for the bar\'s owner and admins.', 403);
  $role = (string)($list[0] ?? '') === $u['user'] ? 'owner' : 'admin';
  return ['id' => preg_replace('/[^a-z0-9_-]/', '', strtolower($u['user'])), 'name' => $u['name'] ?: $u['user'], 'role' => $role];
}

function publish_bundle($bundle, $me, $note) {
  global $data;
  $fp = fopen("$data/.lock", 'c');
  flock($fp, LOCK_EX);
  $live = live_meta();
  $v = (int)$live['version'] + 1;
  $meta = ['version' => $v, 'at' => time(), 'author' => $me['name'], 'role' => $me['role'], 'note' => $note];
  write_json("$data/versions/$v.json", ['format' => $bundle['format'], 'version' => $v, 'author' => $me['name'], 'at' => $meta['at'], 'note' => $note, 'files' => $bundle['files']]);
  $index = index_list();
  $index[] = $meta;
  while (count($index) > KEEP) { $old = array_shift($index); @unlink("$data/versions/{$old['version']}.json"); }
  write_json("$data/versions.json", $index);
  write_json("$data/live.json", $meta);
  flock($fp, LOCK_UN);
  return $meta;
}

$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
if ($method === 'GET' && isset($_GET['live'])) { $m = live_meta(); reply(['ok' => 1] + $m); }
if ($method === 'GET' && isset($_GET['bundle'])) {
  $v = $_GET['bundle'] === 'live' ? (int)live_meta()['version'] : (int)$_GET['bundle'];
  $path = "$data/versions/$v.json";
  if ($v < 1 || !is_file($path)) fail('No such version.', 404);
  header('Content-Type: application/json');
  header($_GET['bundle'] === 'live' ? 'Cache-Control: no-store' : 'Cache-Control: public, max-age=31536000, immutable');
  readfile($path);
  exit;
}
if ($method !== 'POST') fail('POST only', 405);
$raw = file_get_contents('php://input');
if (strlen($raw) > BUNDLE_MAX + 100000) fail('Too big.', 413);
$in = json_decode($raw, true);
if (!is_array($in)) fail('Bad request.');
$me = who($in);
$a = $in['a'] ?? '';
$draftPath = "$data/drafts/{$me['id']}.json";

if ($a === 'me') reply(['ok' => 1, 'you' => $me['name'], 'role' => $me['role'], 'live' => live_meta()]);
if ($a === 'draft') reply(['ok' => 1, 'draft' => read_json($draftPath), 'live' => live_meta()]);
if ($a === 'save') {
  $b = check_bundle($in['bundle'] ?? null);
  $base = (int)($in['base'] ?? 0);
  write_json($draftPath, ['base' => $base, 'at' => time(), 'format' => $b['format'], 'files' => $b['files']]);
  reply(['ok' => 1, 'saved' => time(), 'live' => live_meta()]);
}
if ($a === 'discard') { @unlink($draftPath); reply(['ok' => 1]); }
if ($a === 'publish') {
  $base = (int)($in['base'] ?? -1);
  $live = live_meta();
  if ($base !== (int)$live['version']) reply(['ok' => 0, 'error' => 'The world changed since you started (now version ' . $live['version'] . ' by ' . $live['author'] . '). Update your draft first.', 'live' => $live], 409);
  if (isset($in['bundle'])) $b = check_bundle($in['bundle']);
  else { $d = read_json($draftPath); if (!$d) fail('Nothing to publish: no draft.'); $b = check_bundle($d); }
  $meta = publish_bundle($b, $me, clean($in['note'] ?? '', NOTE_MAX));
  @unlink($draftPath);
  reply(['ok' => 1, 'live' => $meta]);
}
if ($a === 'history') reply(['ok' => 1, 'versions' => array_reverse(index_list()), 'live' => live_meta()]);
if ($a === 'restore') {
  $v = (int)($in['v'] ?? 0);
  $old = read_json("$data/versions/$v.json");
  if (!$old) fail('No such version.', 404);
  $note = clean($in['note'] ?? '', NOTE_MAX) ?: "Back to version $v";
  $meta = publish_bundle(['format' => $old['format'] ?? 1, 'files' => $old['files']], $me, $note);
  reply(['ok' => 1, 'live' => $meta]);
}
fail('Unknown action.');
