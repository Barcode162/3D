<?php
// The door of the private bar (joshuavelasco.com/bar, docs/LOOPED.md A1-A3).
// .htaccess sends every request here except account.php, report.php,
// relay.php, version.txt and this file. Without a login that may play, the
// page is the sign-in screen; with one, the file asked for is streamed
// (Range for Safari's <audio>, ETag/304 so a reload is cheap).
//
// Who may play: the owner and the admins (accounts/admins.json; while it
// doesn't exist, the first account) and the invited (accounts/players.json).
// The login is the bar's own (account.php sets the cookie bar_t = the token).
// Claude's checks: the hub's Claude key (private/hub/key.json) as the header
// X-Hub-Key or ?key= once (then the cookie bar_k).
//
// gate.php?people is the People screen (admins): invite, remove, make admin.

$base = __DIR__ . '/accounts';
$hubData = getenv('HUB_DATA') ?: dirname(__DIR__, 2) . '/private/hub';
$site = __DIR__;

function gate_json($path) {
  $s = @file_get_contents($path);
  $v = $s === false ? null : json_decode($s, true);
  return is_array($v) ? $v : null;
}

// The login on this request (cookie), or null.
function gate_user() {
  global $base;
  $t = $_COOKIE['bar_t'] ?? '';
  if (!is_string($t) || !preg_match('/^[0-9a-f]{48}$/', $t)) return null;
  $rec = gate_json("$base/tokens/" . hash('sha256', $t) . '.json');
  return $rec ? gate_json("$base/users/{$rec['id']}.json") : null;
}
function gate_admins() {
  global $base;
  $admins = gate_json("$base/admins.json");
  if (is_array($admins)) return array_values($admins);
  $names = gate_json("$base/names.json");
  return is_array($names) && count($names) ? [array_key_first($names)] : [];
}
// The bar is open (user, 2026-10-21): any account, or a guest (the cookie
// bar_guest the sign-in page sets with "Come in as a guest"). Only the hub
// stays private (hub/hub.php: the owner).
function gate_may_play($u) {
  return $u !== null || ($_COOKIE['bar_guest'] ?? '') === '1';
}
function gate_claude() {
  global $hubData;
  $key = $_SERVER['HTTP_X_HUB_KEY'] ?? ($_GET['key'] ?? ($_COOKIE['bar_k'] ?? ''));
  if (!is_string($key) || $key === '' || strlen($key) > 200) return false;
  $k = gate_json("$hubData/key.json");
  if (!$k || !hash_equals($k['hash'], hash('sha256', $key))) return false;
  if (($_COOKIE['bar_k'] ?? '') !== $key) {
    setcookie('bar_k', $key, ['expires' => time() + 86400, 'path' => '/', 'secure' => gate_https(),
                              'httponly' => true, 'samesite' => 'Lax']);
  }
  return true;
}
function gate_https() { return ($_SERVER['HTTPS'] ?? '') !== '' && $_SERVER['HTTPS'] !== 'off'; }

// The file this request asks for, relative to the bar's folder.
function gate_path() {
  $uri = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?: '/';
  $uri = rawurldecode($uri);
  $root = rtrim(str_replace('\\', '/', dirname($_SERVER['SCRIPT_NAME'] ?? '/')), '/');
  if ($root !== '' && strpos($uri, $root . '/') === 0) $uri = substr($uri, strlen($root));
  elseif ($root !== '' && $uri === $root) $uri = '/';
  if (substr($uri, -1) === '/') $uri .= 'index.html';
  return ltrim($uri, '/');
}

const GATE_TYPES = [
  'html' => 'text/html; charset=utf-8', 'js' => 'text/javascript; charset=utf-8', 'mjs' => 'text/javascript; charset=utf-8',
  'css' => 'text/css; charset=utf-8', 'json' => 'application/json', 'wasm' => 'application/wasm',
  'txt' => 'text/plain; charset=utf-8', 'svg' => 'image/svg+xml', 'png' => 'image/png', 'jpg' => 'image/jpeg',
  'jpeg' => 'image/jpeg', 'webp' => 'image/webp', 'gif' => 'image/gif', 'ico' => 'image/x-icon',
  'mp3' => 'audio/mpeg', 'm4a' => 'audio/mp4', 'aac' => 'audio/aac', 'ogg' => 'audio/ogg', 'wav' => 'audio/wav',
  'mid' => 'audio/midi', 'midi' => 'audio/midi', 'glb' => 'model/gltf-binary', 'gltf' => 'model/gltf+json',
  'bin' => 'application/octet-stream', 'ktx2' => 'image/ktx2', 'astc' => 'application/octet-stream',
  'woff' => 'font/woff', 'woff2' => 'font/woff2', 'ttf' => 'font/ttf', 'otf' => 'font/otf', 'osz' => 'application/zip',
  'zip' => 'application/zip', 'webm' => 'video/webm', 'mp4' => 'video/mp4',
];
const GATE_SQUEEZE = ['html', 'js', 'mjs', 'css', 'json', 'wasm', 'txt', 'svg', 'gltf', 'mid', 'midi'];

// Streams one file of the bar (only plain files inside its folder; never
// the private folders or the PHP scripts).
function gate_serve($rel) {
  global $site;
  if ($rel === '' || preg_match('#(^|/)\.|(^|/)(accounts|rooms)(/|$)|\.php$#i', $rel)) gate_missing();
  $file = realpath("$site/$rel");
  $rootReal = realpath($site);
  if ($file !== false && is_dir($file) && strpos($file . DIRECTORY_SEPARATOR, $rootReal . DIRECTORY_SEPARATOR) === 0) {
    // A folder without its slash (/bar/meridian): send it to /bar/meridian/.
    $uri = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?: '/';
    $q = $_SERVER['QUERY_STRING'] ?? '';
    header('Location: ' . rtrim($uri, '/') . '/' . ($q !== '' ? "?$q" : ''), true, 301);
    exit;
  }
  if ($file === false || strpos($file, $rootReal . DIRECTORY_SEPARATOR) !== 0 || !is_file($file)) gate_missing();
  $ext = strtolower(pathinfo($file, PATHINFO_EXTENSION));
  $size = filesize($file);
  $mtime = filemtime($file);
  $etag = '"' . dechex($mtime) . '-' . dechex($size) . '"';
  header('Content-Type: ' . (GATE_TYPES[$ext] ?? 'application/octet-stream'));
  header('Cache-Control: private, no-cache');  // always ask; a 304 is cheap
  header('ETag: ' . $etag);
  header('Last-Modified: ' . gmdate('D, d M Y H:i:s', $mtime) . ' GMT');
  header('Accept-Ranges: bytes');
  header('X-Content-Type-Options: nosniff');
  if (trim($_SERVER['HTTP_IF_NONE_MATCH'] ?? '') === $etag) { http_response_code(304); exit; }
  $method = $_SERVER['REQUEST_METHOD'] ?? 'GET';

  $range = $_SERVER['HTTP_RANGE'] ?? '';
  if ($range !== '' && preg_match('/^bytes=(\d*)-(\d*)$/', trim($range), $m) && ($m[1] !== '' || $m[2] !== '')) {
    if ($m[1] === '') { $start = max(0, $size - (int)$m[2]); $end = $size - 1; }
    else { $start = (int)$m[1]; $end = $m[2] === '' ? $size - 1 : min((int)$m[2], $size - 1); }
    if ($start > $end || $start >= $size) {
      http_response_code(416);
      header("Content-Range: bytes */$size");
      exit;
    }
    http_response_code(206);
    header("Content-Range: bytes $start-$end/$size");
    header('Content-Length: ' . ($end - $start + 1));
    if ($method === 'HEAD') exit;
    $f = fopen($file, 'rb');
    fseek($f, $start);
    $left = $end - $start + 1;
    while ($left > 0 && !feof($f)) {
      $chunk = fread($f, min(65536, $left));
      echo $chunk;
      $left -= strlen($chunk);
      flush();
    }
    fclose($f);
    exit;
  }

  // Text and the wasm are sent gzipped when the browser takes it.
  $gz = in_array($ext, GATE_SQUEEZE, true) && $size > 1024 && $size < 64 * 1024 * 1024
    && strpos($_SERVER['HTTP_ACCEPT_ENCODING'] ?? '', 'gzip') !== false && function_exists('gzencode');
  if ($gz) {
    $body = gzencode(file_get_contents($file), 6);
    header('Content-Encoding: gzip');
    header('Vary: Accept-Encoding');
    header('Content-Length: ' . strlen($body));
    if ($method !== 'HEAD') echo $body;
    exit;
  }
  header('Content-Length: ' . $size);
  if ($method !== 'HEAD') readfile($file);
  exit;
}
function gate_missing() {
  http_response_code(404);
  header('Content-Type: text/plain; charset=utf-8');
  header('Cache-Control: no-store');
  echo "Not found\n";
  exit;
}

// The sign-in / waiting / people screens: one small page, its own look.
function gate_page($mode, $u = null) {
  http_response_code($mode === 'people' ? 200 : 401);
  header('Content-Type: text/html; charset=utf-8');
  header('Cache-Control: no-store');
  header('X-Frame-Options: DENY');
  $name = $u ? htmlspecialchars($u['name'] ?: $u['user'], ENT_QUOTES) : '';
  $isOwner = $u && (gate_admins()[0] ?? '') === $u['user'];
  $boot = json_encode(['mode' => $mode, 'name' => $name, 'owner' => $isOwner]);
  $html = @file_get_contents(__DIR__ . '/gate.html');
  echo $html === false ? '<p>Sign-in page missing.</p>'
                       : str_replace('<!--GATE-->', "<script>window.GATE=$boot;</script>", $html);
  exit;
}

if (isset($_GET['people'])) {
  $u = gate_user();
  if (!$u) gate_page('signin');
  if (!in_array($u['user'], gate_admins(), true)) gate_page('waiting', $u);
  gate_page('people', $u);
}

$u = gate_user();
if (gate_may_play($u) || gate_claude()) gate_serve(gate_path());
gate_page($u ? 'waiting' : 'signin', $u);
