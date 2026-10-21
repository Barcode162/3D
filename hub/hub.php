<?php
// The project hub (joshuavelasco.com/hub): the owner's boards, one per
// project (Bar3D first), card by card, and how Claude reads and updates them.
//
// POST JSON (sent as text/plain, so browsers skip the CORS preflight). Every
// action takes "project" (default "bar3d").
//   {"a":"projects"}                           -> {"ok":1,"projects":[{id,name,about,link,repo,image,counts,updated}]}
//   {"a":"project","project":{id?,name,about,link,repo,image?}}   owner: make or edit a project
//        (image: a data: URL, JPEG/PNG, up to 400 KB; "" removes it)
//   {"a":"board"}                              -> {"ok":1,"role":..,"sections":[..],"cards":[..]}
//   {"a":"save","card":{id?,section,title,stage,notes,plan,answer?}}  -> {"ok":1,"card":..}
//   {"a":"say","id":"..","text":".."}          a message on the card's conversation
//   {"a":"move","id":"..","dir":-1|1}          owner: up / down within its section
//   {"a":"delete","id":".."}                   owner
//   {"a":"key"}                                owner: a new Claude key (shown once)
//   {"a":"section","id"?:"..","name"?:..,"about"?:..,"advanced"?:bool}
//        owner: add (no id) or rename a section; Claude: only "about"
// Claude, with the key (header X-Hub-Key, or "key" in the body):
//   {"a":"queue"}                              the "Ready for Claude" cards, in order
//   {"a":"inbox"}                              cards where the owner wrote since Claude last did
//   {"a":"report","id":"..","stage":..,"report":..,"question":..,"build":..,"choices":["Yes","Not now"]}
// Hub v3 (docs/LOOPED.md C): the map, the news feed, one-tap answers.
//   {"a":"spots"}                    -> {"ok":1,"spots":[..],"rules":[..],"map":{..}} the project's places
//        (hub/projects/<project>-spots.json, + what Claude wrote: about, status, the talk)
//   {"a":"spot","id":"..","about"?:..,"status"?:"live|building|planned|idea"}   a place's line
//   {"a":"spotsay","id":"..","text":".."}    a message in a place's talk
//   {"a":"posts"}                    -> {"ok":1,"posts":[{id,build,title,text,shots:[{name,url}],at,published}]}
//   {"a":"post","post":{id?,build,title,text,shots:[names]}}   a news post (Claude or the owner)
//   {"a":"publish","id":"..","on":1}  owner: show it on the public news board (joshuavelasco.com/news)
//   {"a":"unpost","id":".."}          owner: delete a post
//   {"a":"shot","build":"abc1234","name":"door","image":"data:image/jpeg;base64,.."}  a screenshot
//   GET hub.php?shot=<project>/<file>  the picture (the owner's login cookie, or the Claude key)
//   A card's "spot" (save) puts it on a place; "choices" (report) are the
//   one-tap answers to its question.
// A card: notes (the owner's own words: what they want), plan (Claude's
// structured plan, docs/HUB_MAP.md), report/question/build (Claude's latest),
// thread (the conversation: questions, answers, reports, messages), unread
// (the owner wrote something Claude hasn't answered yet).
// Logins are the bar's accounts (bar/account.php): the owner is the bar's
// owner (accounts/admins.json, else the first account made).
// Storage (outside the web root): private/hub/projects.json, board.json
// (Bar3D) and board-<project>.json, img-<project>.txt, key.json.
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-Hub-Key');

const STAGES = ['idea', 'review', 'ready', 'doing', 'check', 'done'];
const TITLE_MAX = 100;
const TEXT_MAX = 6000;
const HISTORY_MAX = 40;
const THREAD_MAX = 80;
const IMAGE_MAX = 400000;
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

// ---- Projects ----
function default_projects() {
  return ['projects' => [[
    'id' => 'bar3d', 'name' => 'Bar3D',
    'about' => 'A from-scratch 3D engine (C++ → WebAssembly → WebGPU) building a VRChat-style hangout bar: walk around, play games, music, friends.',
    'link' => 'https://joshuavelasco.com/bar/', 'repo' => 'barcode162/3D', 'image' => 'projects/bar3d.jpg', 'created' => time()]]];
}
function projects_list() {
  global $data;
  return read_json("$data/projects.json") ?? default_projects();
}
function board_path($pid) {
  global $data;
  return $pid === 'bar3d' ? "$data/board.json" : "$data/board-$pid.json";
}
function project_id($in) {
  $p = $in['project'] ?? 'bar3d';
  if (!is_string($p) || !preg_match('/^[a-z0-9-]{1,24}$/', $p)) fail('No such project.', 404);
  foreach (projects_list()['projects'] as $x) if ($x['id'] === $p) return $p;
  fail('No such project.', 404);
}
// A new project's starting sections (the owner can add more).
function starting_board() {
  return ['sections' => [
    ['id' => 'ideas', 'name' => 'Ideas', 'about' => 'Not sorted yet.', 'advanced' => false],
    ['id' => 'features', 'name' => 'Features', 'about' => 'What people will see and use.', 'advanced' => false],
    ['id' => 'bugs', 'name' => 'Bugs', 'about' => 'Something broken.', 'advanced' => false],
    ['id' => 'tools', 'name' => 'Tools & setup', 'about' => 'How it\'s built, tested and shipped.', 'advanced' => true]],
    'cards' => []];
}

// The board, read and written under a lock so two saves can't mix.
function with_board($pid, $fn) {
  $f = fopen(board_path($pid), 'c+');
  flock($f, LOCK_EX);
  $board = json_decode(stream_get_contents($f), true);
  $seeded = false;
  if (!is_array($board) || !isset($board['cards'])) {
    $seeded = true;  // the first visit: the starting cards, saved right away
    $board = $pid === 'bar3d' ? (read_json(__DIR__ . '/seed.json') ?? ['sections' => [], 'cards' => []]) : starting_board();
    foreach ($board['cards'] as $i => &$c) $c += ['id' => bin2hex(random_bytes(5)), 'order' => $i];
    unset($c);
  }
  foreach ($board['cards'] as &$c) {  // fields added later (v2: notes, thread, unread)
    $c += ['notes' => '', 'plan' => '', 'report' => '', 'question' => '', 'build' => '', 'thread' => [],
           'unread' => false, 'history' => [], 'updated' => time(), 'order' => 0, 'spot' => '', 'choices' => []];
  }
  unset($c);
  $result = $fn($board);
  if ($seeded || ($result['_save'] ?? false)) {
    ftruncate($f, 0);
    rewind($f);
    fwrite($f, json_encode($board, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));
    fflush($f);
  }
  unset($result['_save']);
  flock($f, LOCK_UN);
  fclose($f);
  return $result;
}

function find_card(&$board, $id) {
  foreach ($board['cards'] as $i => $c) if ($c['id'] === $id) return $i;
  return -1;
}
function note(&$card, $who, $what) {
  $card['history'][] = ['at' => time(), 'who' => $who, 'what' => $what];
  if (count($card['history']) > HISTORY_MAX) $card['history'] = array_slice($card['history'], -HISTORY_MAX);
  $card['updated'] = time();
}
// A message on the card's conversation; who wrote last decides "unread".
function say(&$card, $me, $kind, $text) {
  $card['thread'][] = ['at' => time(), 'who' => $me['name'], 'role' => $me['role'], 'kind' => $kind, 'text' => $text];
  if (count($card['thread']) > THREAD_MAX) $card['thread'] = array_slice($card['thread'], -THREAD_MAX);
  $card['unread'] = $me['role'] === 'owner';
  $card['updated'] = time();
}
function ordered($cards) {
  usort($cards, fn($a, $b) => $a['order'] <=> $b['order']);
  return $cards;
}
function in_board_order($b, $cards) {
  $sectionIndex = array_flip(array_map(fn($s) => $s['id'], $b['sections']));
  usort($cards, fn($x, $y) => [$sectionIndex[$x['section']] ?? 99, $x['order']] <=> [$sectionIndex[$y['section']] ?? 99, $y['order']]);
  return array_values($cards);
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
if ($method !== 'POST') fail('POST only', 405);
$in = json_decode(file_get_contents('php://input'), true);
if (!is_array($in)) fail('Bad request.');
$a = $in['a'] ?? '';
$me = who($in);

if ($a === 'projects') {
  $out = [];
  foreach (projects_list()['projects'] as $p) {
    $counts = array_fill_keys(STAGES, 0);
    $questions = 0;
    $updated = $p['created'] ?? 0;
    $b = read_json(board_path($p['id']));
    if (!$b && $p['id'] === 'bar3d') $b = with_board('bar3d', fn(&$x) => $x);
    foreach ($b['cards'] ?? [] as $c) {
      if (isset($counts[$c['stage']])) $counts[$c['stage']]++;
      if (($c['question'] ?? '') !== '') $questions++;
      $updated = max($updated, $c['updated'] ?? 0);
    }
    $img = @file_get_contents("$data/img-{$p['id']}.txt");
    $p['image'] = $img !== false ? $img : ($p['image'] ?? '');
    $p['counts'] = $counts;
    $p['questions'] = $questions;
    $p['updated'] = $updated;
    $out[] = $p;
  }
  reply(['ok' => 1, 'role' => $me['role'], 'you' => $me['name'], 'projects' => $out]);
}

if ($a === 'project') {
  if ($me['role'] !== 'owner') fail('Only the owner makes projects.', 403);
  $in_p = is_array($in['project'] ?? null) ? $in['project'] : fail('No project.');
  $list = projects_list();
  $id = is_string($in_p['id'] ?? null) ? $in_p['id'] : '';
  $i = -1;
  foreach ($list['projects'] as $k => $p) if ($p['id'] === $id) $i = $k;
  if ($id !== '' && $i < 0) fail('No such project.', 404);
  if ($i < 0) {
    $name = clean($in_p['name'] ?? '', 60);
    if ($name === '') fail('Give the project a name.');
    $base = trim(preg_replace('/[^a-z0-9]+/', '-', strtolower($name)), '-') ?: 'project';
    $base = substr($base, 0, 20);
    $id = $base;
    $ids = array_map(fn($p) => $p['id'], $list['projects']);
    for ($n = 2; in_array($id, $ids, true); $n++) $id = "$base-$n";
    $list['projects'][] = ['id' => $id, 'name' => $name, 'about' => '', 'link' => '', 'repo' => '', 'image' => '', 'created' => time()];
    $i = count($list['projects']) - 1;
  }
  $p = &$list['projects'][$i];
  if (isset($in_p['name']) && clean($in_p['name'], 60) !== '') $p['name'] = clean($in_p['name'], 60);
  foreach (['about' => 600, 'link' => 300, 'repo' => 100] as $field => $max) {
    if (isset($in_p[$field])) $p[$field] = clean($in_p[$field], $max);
  }
  if ($p['link'] !== '' && !preg_match('#^https?://#', $p['link'])) $p['link'] = 'https://' . $p['link'];
  if (isset($in_p['image'])) {
    $img = $in_p['image'];
    if ($img === '') @unlink("$data/img-$id.txt");
    elseif (is_string($img) && strlen($img) <= IMAGE_MAX && preg_match('#^data:image/(jpeg|png|webp);base64,[A-Za-z0-9+/=]+$#', $img)) {
      file_put_contents("$data/img-$id.txt", $img);
    } else fail('That picture is too big or not a JPEG/PNG.');
  }
  unset($p);
  write_json("$data/projects.json", $list);
  reply(['ok' => 1, 'id' => $id]);
}

$pid = project_id($in);

if ($a === 'board') {
  reply(with_board($pid, fn(&$b) => ['ok' => 1, 'role' => $me['role'], 'you' => $me['name'], 'project' => $pid,
                                     'stages' => STAGES, 'sections' => $b['sections'], 'cards' => ordered($b['cards'])]));
}

if ($a === 'queue') {
  reply(with_board($pid, fn(&$b) => ['ok' => 1, 'cards' => in_board_order($b, array_filter($b['cards'], fn($c) => $c['stage'] === 'ready'))]));
}

if ($a === 'inbox') {
  $spots = [];
  $extra = read_json("$data/spots-$pid.json") ?? [];
  foreach ($extra as $sid => $x) if (!empty($x['unread'])) $spots[] = ['id' => $sid] + $x;
  reply(with_board($pid, fn(&$b) => ['ok' => 1, 'cards' => in_board_order($b, array_filter($b['cards'], fn($c) => $c['unread'])),
                                     'spots' => $spots]));
}

if ($a === 'save') {
  $in_card = is_array($in['card'] ?? null) ? $in['card'] : fail('No card.');
  reply(with_board($pid, function (&$b) use ($in_card, $me) {
    $sections = array_map(fn($s) => $s['id'], $b['sections']);
    $id = is_string($in_card['id'] ?? null) ? $in_card['id'] : '';
    $i = $id !== '' ? find_card($b, $id) : -1;
    if ($id !== '' && $i < 0) fail('That card is gone.', 404);
    if ($i < 0) {
      $section = in_array($in_card['section'] ?? '', $sections, true) ? $in_card['section'] : $sections[0];
      $orders = array_map(fn($c) => $c['order'], $b['cards']);
      $b['cards'][] = ['id' => bin2hex(random_bytes(5)), 'section' => $section, 'title' => 'New card', 'stage' => 'idea',
                       'notes' => '', 'plan' => '', 'report' => '', 'question' => '', 'build' => '', 'thread' => [],
                       'unread' => false, 'order' => $orders ? max($orders) + 1 : 0, 'history' => [], 'updated' => time(),
                       'spot' => '', 'choices' => []];
      $i = count($b['cards']) - 1;
      note($b['cards'][$i], $me['name'], 'made this card');
    }
    $c = &$b['cards'][$i];
    $changes = [];
    if (isset($in_card['title'])) {
      $t = clean($in_card['title'], TITLE_MAX);
      if ($t !== '' && $t !== $c['title']) { $c['title'] = $t; $changes[] = 'title'; }
    }
    if (isset($in_card['section']) && in_array($in_card['section'], $sections, true) && $in_card['section'] !== $c['section']) {
      $c['section'] = $in_card['section'];
      $changes[] = 'moved to ' . $c['section'];
    }
    if (isset($in_card['stage']) && in_array($in_card['stage'], STAGES, true) && $in_card['stage'] !== $c['stage']) {
      $c['stage'] = $in_card['stage'];
      $changes[] = 'stage: ' . $c['stage'];
    }
    // The owner's own words. Claude may only fill them in when empty (moving old notes over).
    if (isset($in_card['notes'])) {
      $n = clean($in_card['notes'], TEXT_MAX);
      if ($n !== $c['notes'] && ($me['role'] === 'owner' || $c['notes'] === '')) {
        $c['notes'] = $n;
        $changes[] = 'what you want';
        if ($me['role'] === 'owner') $c['unread'] = true;
      }
    }
    if (isset($in_card['spot']) && is_string($in_card['spot']) && preg_match('/^[a-z0-9-]{0,24}$/', $in_card['spot'])
        && $in_card['spot'] !== $c['spot']) {
      $c['spot'] = $in_card['spot'];
      $changes[] = 'place: ' . ($c['spot'] ?: 'none');
    }
    if (isset($in_card['plan'])) {
      $p = clean($in_card['plan'], TEXT_MAX);
      if ($p !== $c['plan']) { $c['plan'] = $p; $changes[] = 'plan'; }
    }
    if (isset($in_card['answer'])) {  // the owner answers Claude's question
      $ans = clean($in_card['answer'], TEXT_MAX);
      if ($ans !== '') {
        say($c, $me, 'answer', $ans);
        $c['question'] = '';
        $c['choices'] = [];
        $changes[] = 'answered the question';
      }
    }
    if ($me['role'] === 'claude' && $changes) $c['unread'] = false;
    if ($changes) note($c, $me['name'], implode(', ', $changes));
    return ['ok' => 1, 'card' => $c, '_save' => true];
  }));
}

if ($a === 'say') {
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  $text = clean($in['text'] ?? '', TEXT_MAX);
  if ($text === '') fail('Nothing to send.');
  reply(with_board($pid, function (&$b) use ($id, $text, $me) {
    $i = find_card($b, $id);
    if ($i < 0) fail('That card is gone.', 404);
    $c = &$b['cards'][$i];
    if ($me['role'] === 'owner' && $c['question'] !== '') {  // a message while a question is open answers it
      say($c, $me, 'answer', $text);
      $c['question'] = '';
      $c['choices'] = [];
      note($c, $me['name'], 'answered the question');
    } else {
      say($c, $me, 'note', $text);
      note($c, $me['name'], 'message');
    }
    return ['ok' => 1, 'card' => $c, '_save' => true];
  }));
}

if ($a === 'report') {
  if ($me['role'] !== 'claude') fail('Only Claude reports on cards.', 403);
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  reply(with_board($pid, function (&$b) use ($in, $id, $me) {
    $i = find_card($b, $id);
    if ($i < 0) fail('That card is gone.', 404);
    $c = &$b['cards'][$i];
    $changes = [];
    if (isset($in['stage']) && in_array($in['stage'], STAGES, true) && $in['stage'] !== $c['stage']) {
      $c['stage'] = $in['stage'];
      $changes[] = 'stage: ' . $c['stage'];
    }
    foreach (['build' => 20, 'report' => TEXT_MAX, 'question' => TEXT_MAX] as $field => $max) {
      if (isset($in[$field])) {
        $c[$field] = clean($in[$field], $max);
        $changes[] = $field;
        if ($field !== 'build' && $c[$field] !== '') {
          say($c, $me, $field, $c[$field] . ($field === 'report' && $c['build'] !== '' ? "\n(build {$c['build']})" : ''));
        }
      }
    }
    if (isset($in['choices']) && is_array($in['choices'])) {
      $c['choices'] = array_values(array_filter(array_map(fn($x) => clean($x, 30), array_slice($in['choices'], 0, 4)),
                                                fn($x) => $x !== ''));
    }
    if (isset($in['question']) && $c['question'] === '') $c['choices'] = [];
    $c['unread'] = false;
    if ($changes) note($c, $me['name'], implode(', ', $changes));
    return ['ok' => 1, 'card' => $c, '_save' => true];
  }));
}

// ---- Places (the map) ----
function spots_seed($pid) {
  return read_json(__DIR__ . "/projects/$pid-spots.json") ?? ['spots' => [], 'rules' => []];
}
function with_spots($pid, $fn) {
  global $data;
  $f = fopen("$data/spots-$pid.json", 'c+');
  flock($f, LOCK_EX);
  $extra = json_decode(stream_get_contents($f), true);
  if (!is_array($extra)) $extra = [];
  $result = $fn($extra);
  if ($result['_save'] ?? false) {
    ftruncate($f, 0);
    rewind($f);
    fwrite($f, json_encode($extra, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));
    fflush($f);
  }
  unset($result['_save']);
  flock($f, LOCK_UN);
  fclose($f);
  return $result;
}
function spot_ids($pid) { return array_map(fn($s) => $s['id'], spots_seed($pid)['spots']); }
const SPOT_STATUS = ['live', 'building', 'planned', 'idea'];

if ($a === 'spots') {
  $seed = spots_seed($pid);
  reply(with_spots($pid, function (&$extra) use ($seed, $pid) {
    $out = [];
    foreach ($seed['spots'] as $sp) {
      $x = $extra[$sp['id']] ?? [];
      $sp['about'] = $x['about'] ?? ($sp['about'] ?? '');
      $sp['status'] = $x['status'] ?? ($sp['status'] ?? 'live');
      $sp['thread'] = $x['thread'] ?? [];
      $sp['unread'] = !empty($x['unread']);
      $out[] = $sp;
    }
    return ['ok' => 1, 'spots' => $out, 'rules' => $seed['rules'] ?? [],
            'map' => read_json(__DIR__ . "/projects/$pid-map.json") ?? (object)[]];
  }));
}

if ($a === 'spot' || $a === 'spotsay') {
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  if (!in_array($id, spot_ids($pid), true)) fail('No such place.', 404);
  reply(with_spots($pid, function (&$extra) use ($a, $id, $in, $me) {
    $x = $extra[$id] ?? ['thread' => [], 'unread' => false];
    if ($a === 'spot') {
      if (isset($in['about'])) $x['about'] = clean($in['about'], 300);
      if (isset($in['status']) && in_array($in['status'], SPOT_STATUS, true)) $x['status'] = $in['status'];
    } else {
      $text = clean($in['text'] ?? '', TEXT_MAX);
      if ($text === '') fail('Nothing to send.');
      $x['thread'][] = ['at' => time(), 'who' => $me['name'], 'role' => $me['role'], 'kind' => 'note', 'text' => $text];
      if (count($x['thread']) > THREAD_MAX) $x['thread'] = array_slice($x['thread'], -THREAD_MAX);
      $x['unread'] = $me['role'] === 'owner';
    }
    $x['updated'] = time();
    $extra[$id] = $x;
    return ['ok' => 1, 'spot' => ['id' => $id] + $x, '_save' => true];
  }));
}

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

// A section: the owner adds or renames one; Claude writes its "what goes here" line (docs/HUB_MAP.md).
if ($a === 'section') {
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  reply(with_board($pid, function (&$b) use ($id, $in, $me) {
    if ($id === '') {
      if ($me['role'] !== 'owner') fail('Only the owner adds sections.', 403);
      $name = clean($in['name'] ?? '', 40);
      if ($name === '') fail('Give the section a name.');
      $base = trim(preg_replace('/[^a-z0-9]+/', '-', strtolower($name)), '-') ?: 'section';
      $ids = array_map(fn($s) => $s['id'], $b['sections']);
      $sid = $base;
      for ($n = 2; in_array($sid, $ids, true); $n++) $sid = "$base-$n";
      $b['sections'][] = ['id' => $sid, 'name' => $name, 'about' => clean($in['about'] ?? '', 300), 'advanced' => !empty($in['advanced'])];
      return ['ok' => 1, 'section' => end($b['sections']), '_save' => true];
    }
    foreach ($b['sections'] as &$s) {
      if ($s['id'] !== $id) continue;
      if (isset($in['about'])) {
        $about = clean($in['about'], 300);
        if ($about === '') fail('No description.');
        $s['about'] = $about;
      }
      if ($me['role'] === 'owner' && isset($in['name']) && clean($in['name'], 40) !== '') $s['name'] = clean($in['name'], 40);
      if ($me['role'] === 'owner' && isset($in['advanced'])) $s['advanced'] = !empty($in['advanced']);
      return ['ok' => 1, 'section' => $s, '_save' => true];
    }
    fail('No such section.', 404);
  }));
}

if ($a === 'move') {
  if ($me['role'] !== 'owner') fail('Only the owner orders the board.', 403);
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  $dir = ($in['dir'] ?? 0) < 0 ? -1 : 1;
  reply(with_board($pid, function (&$b) use ($id, $dir) {
    $i = find_card($b, $id);
    if ($i < 0) fail('That card is gone.', 404);
    $same = array_values(array_filter(array_keys($b['cards']), fn($k) => $b['cards'][$k]['section'] === $b['cards'][$i]['section']));
    usort($same, fn($x, $y) => $b['cards'][$x]['order'] <=> $b['cards'][$y]['order']);
    $pos = array_search($i, $same, true);
    $other = $same[$pos + $dir] ?? null;
    if ($other === null) return ['ok' => 1];
    [$b['cards'][$i]['order'], $b['cards'][$other]['order']] = [$b['cards'][$other]['order'], $b['cards'][$i]['order']];
    return ['ok' => 1, '_save' => true];
  }));
}

// Drag to reorder: put a card right before (or after) another card of its section.
if ($a === 'place') {
  if ($me['role'] !== 'owner') fail('Only the owner orders the board.', 403);
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  $before = is_string($in['before'] ?? null) ? $in['before'] : '';
  $after = is_string($in['after'] ?? null) ? $in['after'] : '';
  reply(with_board($pid, function (&$b) use ($id, $before, $after) {
    $i = find_card($b, $id);
    if ($i < 0) fail('That card is gone.', 404);
    $sec = $b['cards'][$i]['section'];
    $same = array_values(array_filter(array_keys($b['cards']), fn($k) => $b['cards'][$k]['section'] === $sec && $k !== $i));
    usort($same, fn($x, $y) => $b['cards'][$x]['order'] <=> $b['cards'][$y]['order']);
    $anchor = find_card($b, $before !== '' ? $before : $after);
    $pos = $anchor >= 0 ? array_search($anchor, $same, true) : false;
    if ($pos === false) $pos = count($same);
    elseif ($before === '') $pos++;
    array_splice($same, $pos, 0, [$i]);
    foreach ($same as $n => $k) $b['cards'][$k]['order'] = $n;
    return ['ok' => 1, '_save' => true];
  }));
}

if ($a === 'delete') {
  if ($me['role'] !== 'owner') fail('Only the owner deletes cards.', 403);
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  reply(with_board($pid, function (&$b) use ($id) {
    $i = find_card($b, $id);
    if ($i < 0) fail('That card is gone.', 404);
    array_splice($b['cards'], $i, 1);
    return ['ok' => 1, '_save' => true];
  }));
}

if ($a === 'key') {
  if ($me['role'] !== 'owner') fail('Only the owner makes Claude keys.', 403);
  $key = bin2hex(random_bytes(24));
  write_json("$data/key.json", ['hash' => hash('sha256', $key), 'made' => time()]);
  reply(['ok' => 1, 'key' => $key]);
}

fail('Unknown action.');
