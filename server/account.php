<?php
// Accounts for Bar3D: sign up, log in, profiles (display name, bio, a few
// card details, a profile picture). Plain PHP with small JSON files, like
// relay.php, so it runs on the IONOS web hosting without a database.
//
// POST (JSON, sent as text/plain so browsers skip the CORS preflight):
//   {"a":"signup","user":"josh","pass":"...","name":"Josh","birth":"2004-05-17"}
//                                                          -> {"ok":1,"token":..,"me":profile}
//     (13 and up only; the birth date stays private: your own replies carry
//      only "band": "teen" (13-17) or "adult", "" for older accounts without one)
//   {"a":"login","user":"josh","pass":"..."}                -> {"ok":1,"token":..,"me":profile}
//   {"a":"me","token":".."}                                 -> {"ok":1,"me":profile}
//   {"a":"update","token":"..","name":..,"bio":..,"drink":..,"song":..,"color":0..7}
//   {"a":"avatar","token":"..","image":"data:image/jpeg;base64,..."}  (square JPEG, <= 256 px)
//   {"a":"logout","token":".."}
//   {"a":"password","token":"..","old":"...","new":"..."}  change your password
//   {"a":"recovery","token":"..","pass":"..."} -> {"ok":1,"code":"XXXX-XXXX-XXXX"}
//     a new recovery code (shown once; the old one stops working)
//   {"a":"reset","user":"josh","code":"XXXX-XXXX-XXXX","new":"..."} -> like login
//     forgot your password: the recovery code sets a new one (and is used up)
//   {"a":"profile","id":".."}                               -> {"ok":1,"profile":profile}
//   {"a":"update", ..., "details":{"pronouns":..,"status":..,...}}  (DETAIL_FIELDS)
//   {"a":"update", ..., "slots":["owner","", ...]}  badge slots on the card
//     (up to SLOT_COUNT; each one of your badges, or "" for an empty slot)
//   {"a":"badge","token":"..","user":"josh","badge":"bartender","on":1}  (owners only)
//   {"a":"people","token":".."}  (admins) -> {"ok":1,"people":[{user,name,joined,id,avatar,status}]}
//     status: "owner" | "admin" | "player" (invited) | "waiting" (not invited yet)
//   {"a":"invite","token":"..","user":"mia","on":1}  (admins) invite / remove a player
//   {"a":"admin","token":"..","user":"mia","on":1}   (the owner only) make / remove an admin
//
// The private bar (docs/archive/LOOPED.md A1-A2): gate.php lets in the owner, the
// admins and the invited (accounts/players.json). Your own profile carries
// "play" (1 = you may play) and "admin". Logging in, signing up, a reset and
// "me" also set the cookie bar_t (HttpOnly) that gate.php reads; logout
// clears it. The owner is the first name in admins.json (or, without it, the
// first account made); every name in admins.json is an admin.
//
// Badges ("bartender", "dj", "vip", "staff") unlock things in the bar (the
// bartender's station). Owners give them; the owners are the usernames in
// accounts/admins.json, or, while that file doesn't exist, the very first
// account made (the bar's creator).
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
// Extra profile details: field => max length.
const DETAIL_FIELDS = ['pronouns' => 20, 'status' => 60, 'location' => 40, 'birthday' => 20, 'game' => 40,
                       'genre' => 40, 'instrument' => 40, 'cocktail' => 40, 'job' => 40, 'languages' => 60,
                       'social' => 60, 'lookingFor' => 60, 'funFact' => 120];
const BADGES = ['bartender', 'dj', 'vip', 'staff'];
const SLOT_COUNT = 6;

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

// The admins: admins.json, or, while it doesn't exist, the first account.
// The first of them is the owner.
function admin_list() {
  global $base;
  $admins = read_json("$base/admins.json");
  if (is_array($admins)) return array_values($admins);
  $names = read_json("$base/names.json");
  return is_array($names) && count($names) ? [(string)array_key_first($names)] : [];
}
// Owners (admins) can give badges (see the top of this file).
function is_owner($u) { return in_array($u['user'], admin_list(), true); }
function is_top_owner($u) { return (admin_list()[0] ?? null) === $u['user']; }
function player_list() { global $base; return array_values(read_json("$base/players.json") ?? []); }
function may_play($u) { return is_owner($u) || in_array($u['user'], player_list(), true); }

// The login cookie for gate.php (same site only; the classic page on other
// sites keeps using the token in localStorage).
function set_login_cookie($token) {
  $https = ($_SERVER['HTTPS'] ?? '') !== '' && $_SERVER['HTTPS'] !== 'off';
  setcookie('bar_t', $token, ['expires' => $token === '' ? time() - 3600 : time() + 400 * 86400, 'path' => '/',
                              'secure' => $https, 'httponly' => true, 'samesite' => 'Lax']);
}

// Age group from the private birth date: "teen" (13-17), "adult", or ""
// (accounts made before birth dates were asked). Only in your own replies.
function age_band($u) {
  $b = $u['birth'] ?? '';
  if (!is_string($b) || !preg_match('/^\d{4}-\d{2}-\d{2}$/', $b)) return '';
  $age = (new DateTime($b))->diff(new DateTime('today'))->y;
  return $age >= 18 ? 'adult' : 'teen';
}
function own_profile($u) {
  return public_profile($u) + ['band' => age_band($u), 'hasRecovery' => !empty($u['recovery']) ? 1 : 0,
                               'play' => may_play($u) ? 1 : 0, 'admin' => is_owner($u) ? 1 : 0];
}
// What others may see.
function public_profile($u) {
  $badges = $u['badges'] ?? [];
  if (is_owner($u)) $badges = array_values(array_unique(array_merge(['owner'], $badges)));
  return ['id' => $u['id'], 'user' => $u['user'], 'name' => $u['name'], 'bio' => $u['bio'],
          'drink' => $u['drink'], 'song' => $u['song'], 'color' => $u['color'],
          'avatar' => $u['avatar'], 'joined' => $u['joined'], 'details' => (object)($u['details'] ?? []),
          'badges' => $badges,
          // The card's badge slots: only badges still held stay in them.
          'slots' => array_map(fn($b) => in_array($b, $badges, true) ? $b : '',
                               array_slice(array_pad($u['slots'] ?? [], SLOT_COUNT, ''), 0, SLOT_COUNT))];
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
    // 13 and up: the birth date is required and kept private.
    $birth = is_string($in['birth'] ?? null) ? $in['birth'] : '';
    if (!preg_match('/^(\d{4})-(\d{2})-(\d{2})$/', $birth, $bm) || !checkdate((int)$bm[2], (int)$bm[3], (int)$bm[1])) {
      flock($namesFile, LOCK_UN);
      fail('Please enter your birth date.');
    }
    $age = (new DateTime($birth))->diff(new DateTime('today'))->y;
    if ((new DateTime($birth)) > new DateTime('today') || $age > 120) { flock($namesFile, LOCK_UN); fail('Please enter your real birth date.'); }
    if ($age < 13) { flock($namesFile, LOCK_UN); fail('Bar3D is for ages 13 and up.'); }
    if (isset($names[$user])) { flock($namesFile, LOCK_UN); fail('That username is taken.'); }
    $id = bin2hex(random_bytes(6));
    $display = clean($in['name'] ?? '', 20);
    $u = ['id' => $id, 'user' => $user, 'hash' => password_hash($pass, PASSWORD_DEFAULT),
          'name' => $display !== '' ? $display : $user, 'bio' => '', 'drink' => '', 'song' => '',
          'color' => random_int(0, 7), 'avatar' => 0, 'joined' => date('Y-m-d'), 'birth' => $birth, 'tokens' => []];
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
  set_login_cookie($token);
  reply(['ok' => 1, 'token' => $token, 'me' => own_profile($u)]);
}

if ($a === 'profile') {
  $id = $in['id'] ?? '';
  $u = is_string($id) && preg_match('/^[0-9a-f]{12}$/', $id) ? read_json(user_path($id)) : null;
  if (!$u) fail('No such person', 404);
  reply(['ok' => 1, 'profile' => public_profile($u)]);
}

// Forgot your password: your recovery code sets a new one. The code is used
// up; every old login is signed out.
if ($a === 'reset') {
  $user = strtolower(is_string($in['user'] ?? null) ? trim($in['user']) : '');
  $code = strtoupper(preg_replace('/[^A-Za-z0-9]/', '', is_string($in['code'] ?? null) ? $in['code'] : ''));
  $new = is_string($in['new'] ?? null) ? $in['new'] : '';
  if (strlen($new) < 6 || strlen($new) > 100) fail('Passwords need at least 6 characters.');
  $names = read_json("$base/names.json") ?? [];
  $id = $names[$user] ?? null;
  $u = $id ? read_json(user_path($id)) : null;
  if (!$u || empty($u['recovery']) || strlen($code) !== 12 || !password_verify($code, $u['recovery'])) {
    usleep(400000);  // slows down guessing
    fail('That username and recovery code don\'t match.', 401);
  }
  foreach ($u['tokens'] ?? [] as $old) @unlink("$base/tokens/$old.json");
  $u['tokens'] = [];
  $u['hash'] = password_hash($new, PASSWORD_DEFAULT);
  unset($u['recovery']);
  $token = new_token($u['id']);
  $u['tokens'][] = hash('sha256', $token);
  write_json(user_path($u['id']), $u);
  set_login_cookie($token);
  reply(['ok' => 1, 'token' => $token, 'me' => own_profile($u)]);
}

// The page lost its saved login (Safari clears site storage after a week
// away) but the gate's cookie still holds it: hand the same login back.
// Same site only: cookies aren't sent with other sites' requests.
if ($a === 'session') {
  $t = $_COOKIE['bar_t'] ?? '';
  if (!is_string($t) || !preg_match('/^[0-9a-f]{48}$/', $t)) fail('Please log in.', 401);
  $u = user_for_token($t);
  reply(['ok' => 1, 'token' => $t, 'me' => own_profile($u)]);
}

$u = user_for_token($in['token'] ?? '');

// Change your password (needs the current one).
if ($a === 'password') {
  $old = is_string($in['old'] ?? null) ? $in['old'] : '';
  $new = is_string($in['new'] ?? null) ? $in['new'] : '';
  if (!password_verify($old, $u['hash'])) { usleep(400000); fail('Your current password is wrong.', 401); }
  if (strlen($new) < 6 || strlen($new) > 100) fail('Passwords need at least 6 characters.');
  $u['hash'] = password_hash($new, PASSWORD_DEFAULT);
  write_json(user_path($u['id']), $u);
  reply(['ok' => 1, 'me' => own_profile($u)]);
}

// A new recovery code (needs your password). Shown once: keep it somewhere safe.
if ($a === 'recovery') {
  $pass = is_string($in['pass'] ?? null) ? $in['pass'] : '';
  if (!password_verify($pass, $u['hash'])) { usleep(400000); fail('Your password is wrong.', 401); }
  $alphabet = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';  // no 0/O or 1/I
  $code = '';
  for ($i = 0; $i < 12; $i++) $code .= $alphabet[random_int(0, strlen($alphabet) - 1)];
  $u['recovery'] = password_hash($code, PASSWORD_DEFAULT);
  write_json(user_path($u['id']), $u);
  reply(['ok' => 1, 'code' => substr($code, 0, 4) . '-' . substr($code, 4, 4) . '-' . substr($code, 8, 4)]);
}

if ($a === 'me') {
  // The page's own request (same site, not the room asking): refresh the cookie.
  if (!empty($in['cookie'])) set_login_cookie($in['token']);
  reply(['ok' => 1, 'me' => own_profile($u)]);
}

// ---- The private bar's people (admins) ----
function find_user($who) {
  global $base;
  $who = strtolower(is_string($who) ? trim($who) : '');
  $names = read_json("$base/names.json") ?? [];
  $id = $names[$who] ?? null;
  return $id ? read_json(user_path($id)) : null;
}
if ($a === 'people') {
  if (!is_owner($u)) fail('Only admins can see this.', 403);
  $admins = admin_list();
  $players = player_list();
  $out = [];
  foreach (read_json("$base/names.json") ?? [] as $name => $id) {
    $name = (string)$name;  // "123" comes back from JSON as a number key
    $p = read_json(user_path($id));
    if (!$p) continue;
    $status = ($admins[0] ?? '') === $name ? 'owner' : (in_array($name, $admins, true) ? 'admin'
            : (in_array($name, $players, true) ? 'player' : 'waiting'));
    $out[] = ['user' => $name, 'name' => $p['name'], 'joined' => $p['joined'], 'id' => $p['id'],
              'avatar' => $p['avatar'] ?? 0, 'status' => $status];
  }
  reply(['ok' => 1, 'people' => $out]);
}
if ($a === 'invite' || $a === 'admin') {
  if ($a === 'invite' && !is_owner($u)) fail('Only admins can invite people.', 403);
  if ($a === 'admin' && !is_top_owner($u)) fail('Only the owner can choose admins.', 403);
  $t = find_user($in['user'] ?? '');
  if (!$t) fail('No player with that username.');
  $on = !empty($in['on']);
  if ($a === 'invite') {
    $list = player_list();
    $list = $on ? array_values(array_unique(array_merge($list, [$t['user']]))) : array_values(array_diff($list, [$t['user']]));
    write_json("$base/players.json", $list);
  } else {
    $list = admin_list();
    if ($t['user'] === $list[0]) fail('The owner stays an admin.');
    $list = $on ? array_values(array_unique(array_merge($list, [$t['user']]))) : array_values(array_diff($list, [$t['user']]));
    write_json("$base/admins.json", $list);
  }
  reply(['ok' => 1]);
}

if ($a === 'update') {
  if (isset($in['name'])) {
    $n = clean($in['name'], 20);
    if ($n !== '') $u['name'] = $n;
  }
  if (isset($in['bio'])) $u['bio'] = clean($in['bio'], BIO_MAX);
  if (isset($in['drink'])) $u['drink'] = clean($in['drink'], FIELD_MAX);
  if (isset($in['song'])) $u['song'] = clean($in['song'], FIELD_MAX);
  if (isset($in['color']) && is_int($in['color'])) $u['color'] = max(0, min(7, $in['color']));
  if (isset($in['details']) && is_array($in['details'])) {
    $d = $u['details'] ?? [];
    foreach (DETAIL_FIELDS as $k => $max) {
      if (array_key_exists($k, $in['details'])) {
        $v = clean($in['details'][$k], $max);
        if ($v === '') unset($d[$k]); else $d[$k] = $v;
      }
    }
    $u['details'] = $d;
  }
  if (isset($in['slots']) && is_array($in['slots'])) {
    $held = public_profile($u)['badges'];
    $slots = [];
    foreach (array_slice($in['slots'], 0, SLOT_COUNT) as $b) {
      $slots[] = is_string($b) && in_array($b, $held, true) ? $b : '';
    }
    $u['slots'] = $slots;
  }
  write_json(user_path($u['id']), $u);
  reply(['ok' => 1, 'me' => own_profile($u)]);
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
  reply(['ok' => 1, 'me' => own_profile($u)]);
}

if ($a === 'badge') {
  if (!is_owner($u)) fail('Only the bar\'s owner can give badges.', 403);
  $who = strtolower(is_string($in['user'] ?? null) ? trim($in['user']) : '');
  $badge = $in['badge'] ?? '';
  if (!in_array($badge, BADGES, true)) fail('Unknown badge.');
  $names = read_json("$base/names.json") ?? [];
  $id = $names[$who] ?? null;
  $t = $id ? read_json(user_path($id)) : null;
  if (!$t) fail('No player with that username.');
  $list = $t['badges'] ?? [];
  if (!empty($in['on'])) $list[] = $badge; else $list = array_diff($list, [$badge]);
  $t['badges'] = array_values(array_unique($list));
  write_json(user_path($t['id']), $t);
  if ($t['id'] === $u['id']) $u = $t;
  reply(['ok' => 1, 'profile' => public_profile($t), 'me' => own_profile($u)]);
}

if ($a === 'logout') {
  $h = hash('sha256', $in['token']);
  @unlink("$base/tokens/$h.json");
  $u['tokens'] = array_values(array_filter($u['tokens'], function ($t) use ($h) { return $t !== $h; }));
  write_json(user_path($u['id']), $u);
  set_login_cookie('');
  reply(['ok' => 1]);
}

fail('Unknown action');
