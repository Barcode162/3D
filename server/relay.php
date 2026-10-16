<?php
// Multiplayer relay for Bar3D, for plain PHP web hosting (IONOS Web Hosting
// Plus), which can't keep a live WebSocket server running. Each player's
// page POSTs its own state about 10 times a second and gets back everyone
// else's latest state. Rooms are small JSON files, locked while updated.
//
// Request (JSON, sent as text/plain so browsers skip the CORS preflight):
//   {"room":"main","id":"k3j9x2a1","name":"Josh","t":12345.6,"s":[x,y,z,yaw,pitch]}
//   {"room":"main","id":"k3j9x2a1","leave":1}       (closing the page)
// Reply: {"ok":1,"players":[{"id":..,"name":..,"t":..,"s":[..],"age":ms}, ...]}
//
// Shared world (M2): things everyone sees the same, e.g. a glass's position,
// the jukebox song, mirrors on/off. Each is a short key with a few numbers.
// A request may add "w":{"g3":[...],"jb":[...]} (changes by this player)
// and "since":rev (the newest world revision it has). The reply then adds
// "rev", "now" (server clock, seconds) and "world":[[key, byId, [..]], ...]
// with every entry changed after `since` by someone else (since = 0: all).
// The world is forgotten once the room has been empty for WORLD_KEEP_S.
// Accounts: a logged-in player adds "tok" (their login token, see
// account.php). The relay checks it once per player and then shows their
// account's display name, and "acct" (account id) and "av" (picture
// revision) to the others, so nobody can wear someone else's name.
// GET relay.php -> {"ok":1,"relay":"bar3d"} (health check)

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');
header('Cache-Control: no-store');
header('Content-Type: application/json');

const TIMEOUT_S = 5.0;   // a player who stops sending is gone after this
const MAX_PLAYERS = 16;  // per room
const MAX_VALUES = 16;   // numbers in a player's state or a world entry
const MAX_KEYS = 96;     // world entries per room
const WORLD_KEEP_S = 1800.0;

function reply($data, $code = 200) {
  http_response_code($code);
  echo json_encode($data);
  exit;
}

$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
if ($method === 'OPTIONS') exit;
if ($method !== 'POST') reply(['ok' => 1, 'relay' => 'bar3d']);

$in = json_decode(file_get_contents('php://input', false, null, 0, 16384), true);
if (!is_array($in)) reply(['ok' => 0, 'error' => 'bad json'], 400);
$room = $in['room'] ?? 'main';
$id = $in['id'] ?? '';
if (!is_string($room) || !preg_match('/^[a-z0-9-]{1,32}$/', $room)) reply(['ok' => 0, 'error' => 'bad room'], 400);
if (!is_string($id) || !preg_match('/^[a-z0-9]{4,16}$/', $id)) reply(['ok' => 0, 'error' => 'bad id'], 400);
$leaving = !empty($in['leave']);
$player = null;
if (!$leaving) {
  $s = $in['s'] ?? null;
  if (!is_array($s) || count($s) > MAX_VALUES) reply(['ok' => 0, 'error' => 'bad state'], 400);
  foreach ($s as $v) if (!is_int($v) && !is_float($v)) reply(['ok' => 0, 'error' => 'bad state'], 400);
  $name = is_string($in['name'] ?? null) ? $in['name'] : 'Guest';
  $name = substr(preg_replace('/[^\x20-\x7E]/', '', $name), 0, 20);
  $t = $in['t'] ?? 0;
  $player = ['name' => $name === '' ? 'Guest' : $name, 't' => is_numeric($t) ? (float)$t : 0.0,
             's' => array_values($s)];
}
$tok = $in['tok'] ?? null;
if (!is_string($tok) || !preg_match('/^[0-9a-f]{48}$/', $tok)) $tok = null;
$changes = [];
if (!$leaving && isset($in['w'])) {
  if (!is_array($in['w']) || count($in['w']) > MAX_KEYS) reply(['ok' => 0, 'error' => 'bad world'], 400);
  foreach ($in['w'] as $key => $values) {
    if (!preg_match('/^[a-z0-9]{1,8}$/', (string)$key) || !is_array($values) || count($values) > MAX_VALUES) {
      reply(['ok' => 0, 'error' => 'bad world'], 400);
    }
    foreach ($values as $v) if (!is_int($v) && !is_float($v)) reply(['ok' => 0, 'error' => 'bad world'], 400);
    $changes[(string)$key] = array_values($values);
  }
}
$since = isset($in['since']) && is_int($in['since']) ? $in['since'] : -1;

$dir = __DIR__ . '/rooms';
if (!is_dir($dir)) @mkdir($dir, 0755);
$file = fopen("$dir/$room.json", 'c+');
if (!$file) reply(['ok' => 0, 'error' => 'storage'], 500);
flock($file, LOCK_EX);
$data = json_decode(stream_get_contents($file), true);
if (!is_array($data) || !isset($data['players'])) $data = ['players' => [], 'world' => [], 'rev' => 0, 'active' => 0];
$players = $data['players'];
$world = $data['world'];
$rev = (int)$data['rev'];
$now = microtime(true);
foreach ($players as $pid => $p) {
  if ($now - ($p['seen'] ?? 0) > TIMEOUT_S) unset($players[$pid]);
}
if (count($players) === 0 && $now - (float)$data['active'] > WORLD_KEEP_S) $world = [];
foreach ($changes as $key => $values) {
  if (!isset($world[$key]) && count($world) >= MAX_KEYS) continue;
  $world[$key] = ['v' => $values, 'by' => $id, 'r' => ++$rev];
}
if ($leaving) {
  unset($players[$id]);
} elseif (isset($players[$id]) || count($players) < MAX_PLAYERS) {
  $player['seen'] = $now;
  $old = $players[$id] ?? [];
  // Account: verified when the token first shows up (or changes), then
  // refreshed now and then so a new name or picture reaches the others.
  if ($tok !== null) {
    $th = hash('sha256', $tok);
    if (($old['th'] ?? '') === $th && $now - ($old['checked'] ?? 0) < 20) {
      foreach (['th', 'checked', 'acct', 'av', 'acctName'] as $k) $player[$k] = $old[$k];
    } else {
      $t = json_decode((string)@file_get_contents(__DIR__ . "/accounts/tokens/$th.json"), true);
      $u = is_array($t) ? json_decode((string)@file_get_contents(__DIR__ . "/accounts/users/{$t['id']}.json"), true) : null;
      if (is_array($u)) {
        $player += ['th' => $th, 'checked' => $now, 'acct' => $u['id'], 'av' => (int)($u['avatar'] ?? 0),
                    'acctName' => substr((string)$u['name'], 0, 40)];
      }
    }
    if (isset($player['acctName'])) $player['name'] = $player['acctName'];
  }
  $players[$id] = $player;
}
ftruncate($file, 0);
rewind($file);
fwrite($file, json_encode(['players' => (object)$players, 'world' => (object)$world, 'rev' => $rev, 'active' => $now]));
fflush($file);
flock($file, LOCK_UN);
fclose($file);

$others = [];
foreach ($players as $pid => $p) {
  if ((string)$pid === $id) continue;
  $o = ['id' => (string)$pid, 'name' => $p['name'], 't' => $p['t'], 's' => $p['s'],
        'age' => (int)round(($now - $p['seen']) * 1000)];
  if (isset($p['acct'])) {
    $o['acct'] = $p['acct'];
    $o['av'] = $p['av'];
  }
  $others[] = $o;
}
$out = ['ok' => 1, 'players' => $others];
if ($since >= 0) {
  $entries = [];
  foreach ($world as $key => $e) {
    if ($e['r'] > $since && $e['by'] !== $id) $entries[] = [(string)$key, $e['by'], $e['v']];
  }
  $out['rev'] = $rev;
  $out['now'] = round($now, 3);
  $out['world'] = $entries;
}
reply($out);
