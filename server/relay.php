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
// GET relay.php -> {"ok":1,"relay":"bar3d"} (health check)

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');
header('Cache-Control: no-store');
header('Content-Type: application/json');

const TIMEOUT_S = 5.0;   // a player who stops sending is gone after this
const MAX_PLAYERS = 16;  // per room
const MAX_VALUES = 16;   // numbers in a player's state

function reply($data, $code = 200) {
  http_response_code($code);
  echo json_encode($data);
  exit;
}

$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
if ($method === 'OPTIONS') exit;
if ($method !== 'POST') reply(['ok' => 1, 'relay' => 'bar3d']);

$in = json_decode(file_get_contents('php://input', false, null, 0, 4096), true);
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

$dir = __DIR__ . '/rooms';
if (!is_dir($dir)) @mkdir($dir, 0755);
$file = fopen("$dir/$room.json", 'c+');
if (!$file) reply(['ok' => 0, 'error' => 'storage'], 500);
flock($file, LOCK_EX);
$players = json_decode(stream_get_contents($file), true);
if (!is_array($players)) $players = [];
$now = microtime(true);
foreach ($players as $pid => $p) {
  if ($now - ($p['seen'] ?? 0) > TIMEOUT_S) unset($players[$pid]);
}
if ($leaving) {
  unset($players[$id]);
} elseif (isset($players[$id]) || count($players) < MAX_PLAYERS) {
  $player['seen'] = $now;
  $players[$id] = $player;
}
ftruncate($file, 0);
rewind($file);
fwrite($file, json_encode($players));
fflush($file);
flock($file, LOCK_UN);
fclose($file);

$others = [];
foreach ($players as $pid => $p) {
  if ((string)$pid === $id) continue;
  $others[] = ['id' => (string)$pid, 'name' => $p['name'], 't' => $p['t'], 's' => $p['s'],
               'age' => (int)round(($now - $p['seen']) * 1000)];
}
reply(['ok' => 1, 'players' => $others]);
