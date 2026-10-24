<?php
// Dev's store (PLAN.md R4): what Dev shows that no other server keeps.
// On IONOS at joshuavelasco.com/dev/api.php; data outside the web root in
// private/dev/ (DEV_DATA overrides, for tests). CI puts plan.md (the repo's
// PLAN.md) and ci.json (the last deploy's result) there.
//
// Public (no login):
//   GET ?site                   the site's words for the front door (R5): {"ok":1,"site":{..}}
// Owner and admins (the bar's login token), or Claude (the hub's key, X-Hub-Key):
//   POST {"a":"plan"}           -> {"ok":1,"plan":"<PLAN.md>","at":unix}
//   POST {"a":"ci"}             -> {"ok":1,"ci":{build,status,at,url}|null}
//   POST {"a":"notes"}          -> {"ok":1,"notes":[{id,kind,text,by,at,read}]} newest first
//   POST {"a":"note","kind":"note|park","text":".."}   leave Claude a note, or an idea to park
// Claude only (the key):
//   POST {"a":"read","ids":[..]}   mark notes as read (taken into the plan at a go)
// Owner and admins:
//   POST {"a":"site","site":{headline,lede,news,newsLink}}   the front door's words
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-Hub-Key');
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'OPTIONS') exit;
$accounts = getenv('HUB_ACCOUNTS') ?: dirname(__DIR__) . '/bar/accounts';
$hubData = getenv('HUB_DATA') ?: dirname(__DIR__, 2) . '/private/hub';
$data = getenv('DEV_DATA') ?: dirname(__DIR__, 2) . '/private/dev';
if (!is_dir($data)) @mkdir($data, 0755, true);
const TEXT_MAX = 2000;
const NOTES_KEEP = 300;

function reply($body, $code = 200) {
  header('Content-Type: application/json');
  header('Cache-Control: no-store');
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

// The owner and admins (accounts/admins.json; without it, the first account), or Claude.
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
  if (!in_array($u['user'], array_map('strval', $list), true)) fail('Dev is for the owner and admins.', 403);
  return ['name' => $u['name'] ?: $u['user'], 'role' => (string)($list[0] ?? '') === $u['user'] ? 'owner' : 'admin'];
}

if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'GET') {
  if (isset($_GET['site'])) reply(['ok' => 1, 'site' => read_json("$data/site.json") ?? (object)[]]);
  fail('Ask for ?site.', 404);
}

$in = json_decode(file_get_contents('php://input'), true);
if (!is_array($in)) fail('Send JSON.');
$a = $in['a'] ?? '';
$me = who($in);
if ($a === 'plan') {
  $s = @file_get_contents("$data/plan.md");
  reply(['ok' => 1, 'plan' => $s === false ? '' : $s, 'at' => $s === false ? 0 : filemtime("$data/plan.md")]);
}
if ($a === 'ci') reply(['ok' => 1, 'ci' => read_json("$data/ci.json")]);
if ($a === 'notes') reply(['ok' => 1, 'notes' => array_reverse(read_json("$data/notes.json") ?? [])]);
if ($a === 'note') {
  $text = clean($in['text'] ?? '', TEXT_MAX);
  if ($text === '') fail('Nothing to send.');
  $kind = ($in['kind'] ?? '') === 'park' ? 'park' : 'note';
  $fp = fopen("$data/.lock", 'c'); flock($fp, LOCK_EX);
  $notes = read_json("$data/notes.json") ?? [];
  $note = ['id' => bin2hex(random_bytes(5)), 'kind' => $kind, 'text' => $text, 'by' => $me['name'], 'at' => time(), 'read' => 0];
  $notes[] = $note;
  while (count($notes) > NOTES_KEEP) array_shift($notes);
  write_json("$data/notes.json", $notes);
  flock($fp, LOCK_UN);
  reply(['ok' => 1, 'note' => $note]);
}
if ($a === 'read') {
  if ($me['role'] !== 'claude') fail('Only Claude marks notes as read.', 403);
  $ids = array_map('strval', is_array($in['ids'] ?? null) ? $in['ids'] : []);
  $fp = fopen("$data/.lock", 'c'); flock($fp, LOCK_EX);
  $notes = read_json("$data/notes.json") ?? [];
  foreach ($notes as &$n) if (in_array($n['id'], $ids, true)) $n['read'] = 1;
  unset($n);
  write_json("$data/notes.json", $notes);
  flock($fp, LOCK_UN);
  reply(['ok' => 1]);
}
if ($a === 'site') {
  if ($me['role'] === 'claude') fail('The site\'s words are yours to write.', 403);
  $in_site = is_array($in['site'] ?? null) ? $in['site'] : [];
  $site = [];
  foreach (['headline' => 120, 'lede' => 300, 'news' => 200, 'newsLink' => 300] as $k => $max) $site[$k] = clean($in_site[$k] ?? '', $max);
  $site['at'] = time(); $site['by'] = $me['name'];
  write_json("$data/site.json", $site);
  reply(['ok' => 1, 'site' => $site]);
}
fail('Unknown action.');
