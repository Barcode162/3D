<?php
// Dev's store (PLAN.md R4): what Dev shows that no other server keeps.
// On IONOS at joshuavelasco.com/dev/api.php; data outside the web root in
// private/dev/ (DEV_DATA overrides, for tests). CI puts the plan's data there
// (plan.yml: plan.json, rules.json, incidents.json, plan-meta.json) and each deploy's real state: ci-game.json and ci-site.json
// (tools/ci_status.mjs; ci.json was the old, game-only report).
//
// Public (no login):
//   GET ?site                   the site's words for the front door (R5): {"ok":1,"site":{..}}
// Owner and admins (the bar's login token), or Claude (the hub's key, X-Hub-Key):
//   POST {"a":"plan"}           -> {"ok":1,"data":{"plan":{..},"rules":{..}}|null,"at":unix,"sha":"..","incidents":[..]}
//                                  the plan's data (tools/plan.mjs), the build it came from and when,
//                                  and the incidents index (tools/records.mjs index), all put there by plan.yml
//   POST {"a":"ci"}             -> {"ok":1,"ci":{"game":R|null,"site":R|null}}, each deploy's state:
//        R = {workflow, build, live, tests: running|passed|failed|cancelled, failed:[ids], allowed:[ids], at, url}
//   POST {"a":"notes"}          -> {"ok":1,"notes":[{id,kind,text,by,at,read}]} newest first
//   POST {"a":"note","kind":"note|park","text":".."}   leave Claude a note, or an idea to park
// Claude only (the key):
//   POST {"a":"read","ids":[..]}   mark notes as read (taken into the plan at a go)
// Owner and admins:
//   POST {"a":"site","site":{headline,lede}}   the front door's words
//   POST {"a":"highlights"}     -> {"ok":1,"highlights":[{id,title,text,link,img,pushed,at,by}]} newest first
//   POST {"a":"highlight","highlight":{id?,title,text,link,pic?},"image":"data:image/jpeg;base64,.."?}
//        make or change one (pic: a site picture like "/site/img/counter.jpg"; or image: an upload, JPEG/PNG, <= 600 KB)
//   POST {"a":"push","id":"..","on":1}   put it on the front page (one at a time) or take it down
//   POST {"a":"unhighlight","id":".."}   delete it
// Public: GET ?highlight -> the pushed one {title,text,link,img} or null; GET ?hi=<id> its uploaded picture (only while pushed)
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

function hl_out($h) {
  $img = $h['pic'] !== '' ? $h['pic'] : (is_file($GLOBALS['data'] . "/highlights/{$h['id']}.jpg") ? "/dev/api.php?hi={$h['id']}" : '');
  return ['id' => $h['id'], 'title' => $h['title'], 'text' => $h['text'], 'link' => $h['link'], 'img' => $img,
          'pushed' => !empty($h['pushed']), 'at' => $h['at'], 'by' => $h['by']];
}
if (($_SERVER['REQUEST_METHOD'] ?? 'GET') === 'GET') {
  if (isset($_GET['site'])) reply(['ok' => 1, 'site' => read_json("$data/site.json") ?? (object)[]]);
  if (isset($_GET['highlight'])) {
    foreach (read_json("$data/highlights.json") ?? [] as $h) if (!empty($h['pushed'])) reply(['ok' => 1, 'highlight' => hl_out($h)]);
    reply(['ok' => 1, 'highlight' => null]);
  }
  if (isset($_GET['hi'])) {
    $id = preg_replace('/[^0-9a-f]/', '', (string)$_GET['hi']);
    foreach (read_json("$data/highlights.json") ?? [] as $h) {
      if ($h['id'] === $id && !empty($h['pushed']) && is_file("$data/highlights/$id.jpg")) {
        header('Content-Type: image/jpeg'); header('Cache-Control: public, max-age=300');
        readfile("$data/highlights/$id.jpg"); exit;
      }
    }
    fail('No such picture.', 404);
  }
  fail('Ask for ?site.', 404);
}

$in = json_decode(file_get_contents('php://input'), true);
if (!is_array($in)) fail('Send JSON.');
$a = $in['a'] ?? '';
$me = who($in);
if ($a === 'plan') {
  $plan = read_json("$data/plan.json");
  $rules = read_json("$data/rules.json");
  $meta = read_json("$data/plan-meta.json");  // {sha, at}: the build the plan came from (.github/workflows/plan.yml)
  reply(['ok' => 1, 'data' => $plan && $rules ? ['plan' => $plan, 'rules' => $rules] : null,
         'at' => $meta['at'] ?? ($plan ? filemtime("$data/plan.json") : 0), 'sha' => $meta['sha'] ?? '',
         'incidents' => read_json("$data/incidents.json") ?? []]);
}
if ($a === 'ci') {
  $game = read_json("$data/ci-game.json");
  $old = $game ? null : read_json("$data/ci.json");  // the report from before J2.3, until the next game deploy
  if ($old) $game = ['workflow' => 'game', 'build' => (string)($old['build'] ?? ''), 'live' => true,
                     'tests' => ($old['status'] ?? '') === 'success' ? 'passed' : 'failed', 'failed' => [], 'allowed' => [],
                     'at' => (int)($old['at'] ?? 0), 'url' => (string)($old['url'] ?? '')];
  reply(['ok' => 1, 'ci' => ['game' => $game, 'site' => read_json("$data/ci-site.json")]]);
}
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
  foreach (['headline' => 120, 'lede' => 300] as $k => $max) $site[$k] = clean($in_site[$k] ?? '', $max);
  $site['at'] = time(); $site['by'] = $me['name'];
  write_json("$data/site.json", $site);
  reply(['ok' => 1, 'site' => $site]);
}

// ---- Highlights: what the front page shows, made for people (a picture, a few friendly words) ----
function with_highlights($fn) {
  global $data;
  $fp = fopen("$data/.lock", 'c'); flock($fp, LOCK_EX);
  $list = read_json("$data/highlights.json") ?? [];
  $out = $fn($list);
  write_json("$data/highlights.json", $list);
  flock($fp, LOCK_UN);
  return $out;
}
if (in_array($a, ['highlights', 'highlight', 'push', 'unhighlight'], true) && $me['role'] === 'claude' && $a !== 'highlights') fail('Highlights are yours to make and push.', 403);
if ($a === 'highlights') {
  $list = array_map('hl_out', read_json("$data/highlights.json") ?? []);
  usort($list, fn($x, $y) => $y['at'] <=> $x['at']);
  reply(['ok' => 1, 'highlights' => $list]);
}
if ($a === 'highlight') {
  $in_h = is_array($in['highlight'] ?? null) ? $in['highlight'] : fail('No highlight.');
  $title = clean($in_h['title'] ?? '', 80);
  if ($title === '') fail('Give it a short title.');
  $pic = clean($in_h['pic'] ?? '', 200);
  if ($pic !== '' && !preg_match('#^/site/img/[a-z0-9_-]+\.(jpg|png)$#', $pic)) fail('That picture is not one of the site\'s.');
  $bytes = null;
  if (!empty($in['image'])) {
    if (!is_string($in['image']) || strlen($in['image']) > 820000 || !preg_match('#^data:image/(jpeg|png);base64,([A-Za-z0-9+/=]+)$#', $in['image'], $m)) fail('That picture is too big or not a JPEG/PNG.');
    $bytes = base64_decode($m[2], true);
    $info = $bytes !== false ? @getimagesizefromstring($bytes) : false;
    if (!$info || !in_array($info[2], [IMAGETYPE_JPEG, IMAGETYPE_PNG], true)) fail('That isn\'t a picture.');
  }
  $link = clean($in_h['link'] ?? '', 300);
  if ($link !== '' && !preg_match('#^(/|https://)#', $link)) fail('A link starts with / or https://');
  reply(with_highlights(function (&$list) use ($in_h, $title, $pic, $bytes, $link, $me, $data) {
    $id = is_string($in_h['id'] ?? null) ? $in_h['id'] : '';
    $i = -1;
    foreach ($list as $k => $h) if ($h['id'] === $id) $i = $k;
    if ($id !== '' && $i < 0) fail('That highlight is gone.', 404);
    if ($i < 0) { $list[] = ['id' => bin2hex(random_bytes(5)), 'pushed' => false, 'pic' => '']; $i = count($list) - 1; }
    $h = &$list[$i];
    $h['title'] = $title; $h['text'] = clean($in_h['text'] ?? '', 240); $h['link'] = $link;
    $h['at'] = time(); $h['by'] = $me['name'];
    if ($bytes !== null) {
      if (!is_dir("$data/highlights")) @mkdir("$data/highlights", 0755, true);
      file_put_contents("$data/highlights/{$h['id']}.jpg", $bytes); $h['pic'] = '';
    } elseif ($pic !== '') { $h['pic'] = $pic; @unlink("$data/highlights/{$h['id']}.jpg"); }
    $out = hl_out($h); unset($h);
    return ['ok' => 1, 'highlight' => $out];
  }));
}
if ($a === 'push' || $a === 'unhighlight') {
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  reply(with_highlights(function (&$list) use ($a, $id, $in, $data) {
    $found = false;
    foreach ($list as $k => &$h) {
      if ($h['id'] === $id) {
        $found = true;
        if ($a === 'unhighlight') { @unlink("$data/highlights/$id.jpg"); array_splice($list, $k, 1); break; }
        $h['pushed'] = !empty($in['on']);
      } elseif ($a === 'push' && !empty($in['on'])) $h['pushed'] = false;   // one on the front page at a time
    }
    unset($h);
    if (!$found) fail('That highlight is gone.', 404);
    return ['ok' => 1];
  }));
}
fail('Unknown action.');
