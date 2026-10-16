<?php
// Accounts for Bar3D: sign up, log in, profiles (display name, bio, a few
// card details, a profile picture). Plain PHP with small JSON files, like
// relay.php, so it runs on the IONOS web hosting without a database.
//
// POST (JSON, sent as text/plain so browsers skip the CORS preflight):
//   {"a":"signup","user":"josh","pass":"...","name":"Josh"}  -> {"ok":1,"token":..,"me":profile}
//   {"a":"login","user":"josh","pass":"..."}                -> {"ok":1,"token":..,"me":profile}
//   {"a":"me","token":".."}                                 -> {"ok":1,"me":profile}
//   {"a":"update","token":"..","name":..,"bio":..,"drink":..,"song":..,"color":0..7}
//   {"a":"avatar","token":"..","image":"data:image/jpeg;base64,..."}  (square JPEG, <= 256 px)
//   {"a":"logout","token":".."}
//   {"a":"profile","id":".."}                               -> {"ok":1,"profile":profile}
// GET account.php?avatar=<id>   -> the profile picture (JPEG)
// Errors: {"ok":0,"error":"..."} with a message meant for people.
//
// Storage (private, see .htaccess): accounts/users/<id>.json,
// accounts/names.json (username -> id), accounts/tokens/<sha256>.json,
// accounts/avatars/<id>.jpg. Passwords are kept only as bcrypt hashes;
// tokens only as their SHA-256.

header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

const BIO_MAX = 240;
const FIELD_MAX = 40;
const AVATAR_MAX_BYTES = 120000;
const MAX_TOKENS_PER_USER = 10;

$base = __DIR__ . '/accounts';
foreach (['', '/users', '/tokens', '/avatars'] as $d) {
  if (!is_dir($base . $d)) @mkdir($base . $d, 0755, true);
}

function reply($data, $code = 200) {
  header('Content-Type: application/json');
  header('Cache-Control: no-store');
  http_response_code($code);
  echo json_encode($data);
  exit;
}
function fail($message, $code = 400) { reply(['ok' => 0, 'error' => $message], $code); }

function clean($s, $max) {
  if (!is_string($s)) return '';
  $s = preg_replace('/[\x00-\x1F\x7F]/u', ' ', $s);
  return trim(mb_substr($s, 0, $max, 'UTF-8'));
}

function read_json($path) {
  $s = @file_get_contents($path);
  $v = $s === false ? null : json_decode($s, true);
  return is_array($v) ? $v : null;
}
function write_json($path, $data) {
  $tmp = $path . '.tmp' . bin2hex(random_bytes(4));
  file_put_contents($tmp, json_encode($data));
  rename($tmp, $path);
}

function user_path($id) { global $base; return "$base/users/$id.json"; }

// What others may see.
function public_profile($u) {
  return ['id' => $u['id'], 'user' => $u['user'], 'name' => $u['name'], 'bio' => $u['bio'],
          'drink' => $u['drink'], 'song' => $u['song'], 'color' => $u['color'],
          'avatar' => $u['avatar'], 'joined' => $u['joined']];
}

function new_token($id) {
  global $base;
  $token = bin2hex(random_bytes(24));
  write_json("$base/tokens/" . hash('sha256', $token) . '.json', ['id' => $id, 'at' => time()]);
  return $token;
}

function user_for_token($token) {
  global $base;
  if (!is_string($token) || !preg_match('/^[0-9a-f]{48}$/', $token)) fail('Please log in again.', 401);
  $t = read_json("$base/tokens/" . hash('sha256', $token) . '.json');
  $u = $t ? read_json(user_path($t['id'])) : null;
  if (!$u) fail('Please log in again.', 401);
  return $u;
}

// ---- GET: profile pictures ----
$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
if ($method === 'OPTIONS') exit;
if ($method === 'GET') {
  $id = $_GET['avatar'] ?? '';
  if (is_string($id) && preg_match('/^[0-9a-f]{12}$/', $id) && is_file("$base/avatars/$id.jpg")) {
    header('Content-Type: image/jpeg');
    header('Cache-Control: public, max-age=86400');  // the URL carries ?v=<revision>
    readfile("$base/avatars/$id.jpg");
    exit;
  }
  if (isset($_GET['avatar'])) fail('No picture', 404);
  reply(['ok' => 1, 'accounts' => 'bar3d']);
}

$in = json_decode(file_get_contents('php://input', false, null, 0, 400000), true);
if (!is_array($in)) fail('Bad request');
$a = $in['a'] ?? '';

if ($a === 'signup' || $a === 'login') {
  $user = strtolower(is_string($in['user'] ?? null) ? trim($in['user']) : '');
  $pass = is_string($in['pass'] ?? null) ? $in['pass'] : '';
  if (!preg_match('/^[a-z0-9_]{3,20}$/', $user)) fail('Usernames are 3-20 letters, numbers or _.');
  if (strlen($pass) < 6 || strlen($pass) > 100) fail('Passwords need at least 6 characters.');
  $namesFile = fopen("$base/names.json", 'c+');
  flock($namesFile, LOCK_EX);
  $names = json_decode(stream_get_contents($namesFile), true);
  if (!is_array($names)) $names = [];
  if ($a === 'signup') {
    if (isset($names[$user])) { flock($namesFile, LOCK_UN); fail('That username is taken.'); }
    $id = bin2hex(random_bytes(6));
    $display = clean($in['name'] ?? '', 20);
    $u = ['id' => $id, 'user' => $user, 'hash' => password_hash($pass, PASSWORD_DEFAULT),
          'name' => $display !== '' ? $display : $user, 'bio' => '', 'drink' => '', 'song' => '',
          'color' => random_int(0, 7), 'avatar' => 0, 'joined' => date('Y-m-d'), 'tokens' => []];
    $names[$user] = $id;
    ftruncate($namesFile, 0);
    rewind($namesFile);
    fwrite($namesFile, json_encode($names));
    fflush($namesFile);
  } else {
    $id = $names[$user] ?? null;
    $u = $id ? read_json(user_path($id)) : null;
    if (!$u || !password_verify($pass, $u['hash'])) {
      flock($namesFile, LOCK_UN);
      usleep(400000);  // slows down password guessing
      fail('Wrong username or password.', 401);
    }
  }
  flock($namesFile, LOCK_UN);
  fclose($namesFile);
  $token = new_token($u['id']);
  // Keep the newest few sessions per user.
  $u['tokens'][] = hash('sha256', $token);
  while (count($u['tokens']) > MAX_TOKENS_PER_USER) @unlink("$base/tokens/" . array_shift($u['tokens']) . '.json');
  write_json(user_path($u['id']), $u);
  reply(['ok' => 1, 'token' => $token, 'me' => public_profile($u)]);
}

if ($a === 'profile') {
  $id = $in['id'] ?? '';
  $u = is_string($id) && preg_match('/^[0-9a-f]{12}$/', $id) ? read_json(user_path($id)) : null;
  if (!$u) fail('No such person', 404);
  reply(['ok' => 1, 'profile' => public_profile($u)]);
}

$u = user_for_token($in['token'] ?? '');

if ($a === 'me') reply(['ok' => 1, 'me' => public_profile($u)]);

if ($a === 'update') {
  if (isset($in['name'])) {
    $n = clean($in['name'], 20);
    if ($n !== '') $u['name'] = $n;
  }
  if (isset($in['bio'])) $u['bio'] = clean($in['bio'], BIO_MAX);
  if (isset($in['drink'])) $u['drink'] = clean($in['drink'], FIELD_MAX);
  if (isset($in['song'])) $u['song'] = clean($in['song'], FIELD_MAX);
  if (isset($in['color']) && is_int($in['color'])) $u['color'] = max(0, min(7, $in['color']));
  write_json(user_path($u['id']), $u);
  reply(['ok' => 1, 'me' => public_profile($u)]);
}

if ($a === 'avatar') {
  $img = $in['image'] ?? '';
  if (!is_string($img) || !preg_match('#^data:image/jpeg;base64,([A-Za-z0-9+/=]+)$#', $img, $m)) fail('Bad picture');
  $bytes = base64_decode($m[1], true);
  if ($bytes === false || strlen($bytes) > AVATAR_MAX_BYTES) fail('That picture is too big.');
  $info = @getimagesizefromstring($bytes);
  if (!$info || $info[2] !== IMAGETYPE_JPEG || $info[0] > 512 || $info[1] > 512) fail('Bad picture');
  file_put_contents("$base/avatars/{$u['id']}.jpg", $bytes);
  $u['avatar'] = ($u['avatar'] ?? 0) + 1;
  write_json(user_path($u['id']), $u);
  reply(['ok' => 1, 'me' => public_profile($u)]);
}

if ($a === 'logout') {
  $h = hash('sha256', $in['token']);
  @unlink("$base/tokens/$h.json");
  $u['tokens'] = array_values(array_filter($u['tokens'], function ($t) use ($h) { return $t !== $h; }));
  write_json(user_path($u['id']), $u);
  reply(['ok' => 1]);
}

fail('Unknown action');
