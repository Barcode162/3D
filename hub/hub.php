<?php
// The project hub (joshuavelasco.com/hub): the owner's board of everything
// in the world, card by card, and how Claude reads and updates it.
//
// POST JSON (sent as text/plain, so browsers skip the CORS preflight):
//   {"a":"board","token":".."}                 -> {"ok":1,"role":"owner","sections":[..],"cards":[..]}
//   {"a":"save","token":"..","card":{id?,section,title,stage,plan}}  -> {"ok":1,"card":..}
//   {"a":"move","token":"..","id":"..","dir":-1|1}   (up / down within its section)
//   {"a":"delete","token":"..","id":".."}
//   {"a":"key","token":".."}                   -> {"ok":1,"key":".."} a new Claude key (shown once)
// Claude, with the key (header X-Hub-Key, or "key" in the body):
//   {"a":"board"}                              the whole board
//   {"a":"queue"}                              -> the "Ready for Claude" cards, in order
//   {"a":"report","id":"..","stage":..,"report":..,"question":..,"build":..}
//   {"a":"save","card":{..}}                   new cards (e.g. split a big one), or edit one (with its id)
// Owner or Claude:
//   {"a":"section","id":"..","about":".."}     a section's "what goes here" line (docs/HUB_MAP.md)
// Logins are the bar's accounts (bar/account.php): the owner is the bar's
// owner (accounts/admins.json, else the first account made). Members (v2)
// will be able to write cards that wait for the owner's review.
// Storage (outside the web root): private/hub/board.json, private/hub/key.json.
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-Hub-Key');

const STAGES = ['idea', 'review', 'ready', 'doing', 'check', 'done'];
const TITLE_MAX = 100;
const TEXT_MAX = 4000;
const HISTORY_MAX = 40;

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
  $owner = is_array($admins) ? in_array($u['user'], $admins, true)
                             : (count($names) && array_key_first($names) === $u['user']);
  if (!$owner) fail('This hub is private.', 403);
  return ['role' => 'owner', 'name' => $u['name'] ?: $u['user']];
}

// The board, read and written under a lock so two saves can't mix.
function with_board($fn) {
  global $data;
  $f = fopen("$data/board.json", 'c+');
  flock($f, LOCK_EX);
  $board = json_decode(stream_get_contents($f), true);
  $seeded = false;
  if (!is_array($board) || !isset($board['cards'])) {
    $seeded = true;  // the first visit: the starting cards (seed.json), saved right away
    $board = read_json(__DIR__ . '/seed.json') ?? ['sections' => [], 'cards' => []];
    foreach ($board['cards'] as $i => &$c) {
      $c += ['id' => bin2hex(random_bytes(5)), 'plan' => '', 'report' => '', 'question' => '', 'build' => '',
             'order' => $i, 'history' => [], 'updated' => time()];
    }
    unset($c);
  }
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
function ordered($cards) {
  usort($cards, fn($a, $b) => $a['order'] <=> $b['order']);
  return $cards;
}

$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
if ($method === 'OPTIONS') exit;
if ($method !== 'POST') fail('POST only', 405);
$in = json_decode(file_get_contents('php://input'), true);
if (!is_array($in)) fail('Bad request.');
$a = $in['a'] ?? '';
$me = who($in);

if ($a === 'board') {
  reply(with_board(fn(&$b) => ['ok' => 1, 'role' => $me['role'], 'you' => $me['name'], 'stages' => STAGES,
                                'sections' => $b['sections'], 'cards' => ordered($b['cards'])]));
}

if ($a === 'queue') {
  reply(with_board(function (&$b) {
    $ready = array_values(array_filter($b['cards'], fn($c) => $c['stage'] === 'ready'));
    // In board order: section order first, then the cards' own order.
    $sectionIndex = array_flip(array_map(fn($s) => $s['id'], $b['sections']));
    usort($ready, fn($x, $y) => [$sectionIndex[$x['section']] ?? 99, $x['order']] <=> [$sectionIndex[$y['section']] ?? 99, $y['order']]);
    return ['ok' => 1, 'cards' => $ready];
  }));
}

if ($a === 'save') {
  $in_card = is_array($in['card'] ?? null) ? $in['card'] : fail('No card.');
  reply(with_board(function (&$b) use ($in_card, $me) {
    $sections = array_map(fn($s) => $s['id'], $b['sections']);
    $id = is_string($in_card['id'] ?? null) ? $in_card['id'] : '';
    $i = $id !== '' ? find_card($b, $id) : -1;
    if ($id !== '' && $i < 0) fail('That card is gone.', 404);
    if ($i < 0) {
      $section = in_array($in_card['section'] ?? '', $sections, true) ? $in_card['section'] : 'ideas';
      $orders = array_map(fn($c) => $c['order'], $b['cards']);
      $b['cards'][] = ['id' => bin2hex(random_bytes(5)), 'section' => $section, 'title' => 'New card', 'stage' => 'idea',
                       'plan' => '', 'report' => '', 'question' => '', 'build' => '',
                       'order' => $orders ? max($orders) + 1 : 0, 'history' => [], 'updated' => time()];
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
    if (isset($in_card['plan'])) {
      $p = clean($in_card['plan'], TEXT_MAX);
      if ($p !== $c['plan']) { $c['plan'] = $p; $changes[] = 'plan'; }
    }
    if (isset($in_card['answer'])) {  // the owner answers Claude's question
      $ans = clean($in_card['answer'], TEXT_MAX);
      if ($ans !== '') {
        $c['plan'] = trim($c['plan'] . "\n\nAnswer: " . $ans);
        $c['question'] = '';
        $changes[] = 'answered the question';
      }
    }
    if ($changes) note($c, $me['name'], implode(', ', $changes));
    return ['ok' => 1, 'card' => $c, '_save' => true];
  }));
}

if ($a === 'report') {
  if ($me['role'] !== 'claude') fail('Only Claude reports on cards.', 403);
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  reply(with_board(function (&$b) use ($in, $id, $me) {
    $i = find_card($b, $id);
    if ($i < 0) fail('That card is gone.', 404);
    $c = &$b['cards'][$i];
    $changes = [];
    if (isset($in['stage']) && in_array($in['stage'], STAGES, true) && $in['stage'] !== $c['stage']) {
      $c['stage'] = $in['stage'];
      $changes[] = 'stage: ' . $c['stage'];
    }
    foreach (['report' => TEXT_MAX, 'question' => TEXT_MAX, 'build' => 20] as $field => $max) {
      if (isset($in[$field])) {
        $c[$field] = clean($in[$field], $max);
        $changes[] = $field;
      }
    }
    if ($changes) note($c, $me['name'], implode(', ', $changes));
    return ['ok' => 1, 'card' => $c, '_save' => true];
  }));
}

// A section's description ("what goes here / not here", docs/HUB_MAP.md).
if ($a === 'section') {
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  $about = clean($in['about'] ?? '', 300);
  if ($about === '') fail('No description.');
  reply(with_board(function (&$b) use ($id, $about) {
    foreach ($b['sections'] as &$s) {
      if ($s['id'] === $id) { $s['about'] = $about; return ['ok' => 1, 'section' => $s, '_save' => true]; }
    }
    fail('No such section.', 404);
  }));
}

if ($a === 'move') {
  if ($me['role'] !== 'owner') fail('Only the owner orders the board.', 403);
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  $dir = ($in['dir'] ?? 0) < 0 ? -1 : 1;
  reply(with_board(function (&$b) use ($id, $dir) {
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

if ($a === 'delete') {
  if ($me['role'] !== 'owner') fail('Only the owner deletes cards.', 403);
  $id = is_string($in['id'] ?? null) ? $in['id'] : '';
  reply(with_board(function (&$b) use ($id) {
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
