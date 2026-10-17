<?php
// Performance and crash reports from the game (restructure step 0.3).
//
// POST (JSON, at most 256 KB)  -> {"code":"K7Q2XD"}; the report is saved as
//                                 <code>.json in a private folder outside
//                                 the web root.
// GET ?code=K7Q2XD             -> that report (Claude reads it with curl).
// GET ?env=1                   -> facts about this hosting (PHP version,
//                                 sodium, pdo_mysql, private folder), used to
//                                 plan the account move (restructure 5.1).
//
// Limits: 10 reports per hour per IP, at most 500 files (oldest deleted),
// files older than 30 days deleted on each POST. Anyone with a code can read
// the report, so the game strips names and tokens before sending, and a
// report containing a 48-hex-digit string (what a login token looks like) is
// refused.

const MAX_BYTES = 262144;
const MAX_FILES = 500;
const MAX_AGE = 30 * 86400;
const PER_IP_PER_HOUR = 10;
const ORIGINS = ['https://barcode162.github.io', 'https://joshuavelasco.com', 'https://www.joshuavelasco.com'];
const CODE_CHARS = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';  // no I, O, 0, 1

header('X-Content-Type-Options: nosniff');
header('Cache-Control: no-store');
header('Content-Type: application/json');
$origin = $_SERVER['HTTP_ORIGIN'] ?? '';
$local = (bool)preg_match('#^http://(localhost|127\.0\.0\.1)(:\d+)?$#', $origin);
if (in_array($origin, ORIGINS, true) || $local) {
  header('Access-Control-Allow-Origin: ' . $origin);
  header('Vary: Origin');
}
if (($_SERVER['REQUEST_METHOD'] ?? '') === 'OPTIONS') {
  header('Access-Control-Allow-Methods: GET, POST');
  header('Access-Control-Allow-Headers: Content-Type');
  exit;
}

function answer(int $status, array $body): void {
  http_response_code($status);
  echo json_encode($body);
  exit;
}

// The SFTP root is two folders above this file (<root>/home/bar/report.php)
// and the web root is <root>/home, so <root>/private is not reachable from
// the web. BAR_PRIVATE_DIR overrides it (tests).
function privateDir(): string {
  $env = getenv('BAR_PRIVATE_DIR');
  return $env ? rtrim($env, '/') : dirname(__DIR__, 2) . '/private/bar';
}

function reportDir(): ?string {
  $dir = privateDir() . '/reports';
  if (!is_dir($dir)) @mkdir($dir, 0700, true);
  if (!is_dir($dir) || !is_writable($dir)) return null;
  // In case a domain ever points at the SFTP root: deny web access anyway.
  $deny = privateDir() . '/.htaccess';
  if (!file_exists($deny)) @file_put_contents($deny, "Require all denied\nDeny from all\n");
  return $dir;
}

if (isset($_GET['env'])) {
  $dir = reportDir();
  $docroot = realpath($_SERVER['DOCUMENT_ROOT'] ?? '') ?: '';
  $real = $dir ? (realpath($dir) ?: '') : '';
  answer(200, [
    'php' => PHP_VERSION,
    'sodium' => function_exists('sodium_crypto_sign_detached'),
    'pdo_mysql' => class_exists('PDO') && in_array('mysql', PDO::getAvailableDrivers(), true),
    'pdo_sqlite' => class_exists('PDO') && in_array('sqlite', PDO::getAvailableDrivers(), true),
    'privateWritable' => $dir !== null,
    'privateOutsideDocroot' => $real !== '' && $docroot !== '' && strpos($real, $docroot . '/') !== 0,
    'openBasedir' => ini_get('open_basedir') !== '' && ini_get('open_basedir') !== false,
  ]);
}

if (isset($_GET['code'])) {
  $code = strtoupper((string)$_GET['code']);
  if (!preg_match('/^[' . CODE_CHARS . ']{6}$/', $code)) answer(400, ['error' => 'bad code']);
  $dir = reportDir();
  $file = $dir ? "$dir/$code.json" : '';
  if (!$file || !is_file($file)) answer(404, ['error' => 'no such report']);
  echo file_get_contents($file);
  exit;
}

if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') answer(405, ['error' => 'POST a report or GET ?code=']);

$body = file_get_contents('php://input', false, null, 0, MAX_BYTES + 1);
if ($body === false || $body === '') answer(400, ['error' => 'empty report']);
if (strlen($body) > MAX_BYTES) answer(413, ['error' => 'report too big']);
if (preg_match('/[0-9a-fA-F]{48}/', $body)) answer(400, ['error' => 'report contains a token']);
$data = json_decode($body, true);
if (!is_array($data)) answer(400, ['error' => 'not JSON']);

$dir = reportDir();
if (!$dir) answer(500, ['error' => 'reports folder not writable']);

// Per-IP limit (the IP is stored only as a hash, for an hour).
$limitFile = "$dir/limits.json";
$lock = fopen("$dir/limits.lock", 'c');
flock($lock, LOCK_EX);
$limits = json_decode(@file_get_contents($limitFile) ?: '{}', true) ?: [];
$now = time();
foreach ($limits as $k => $times) {
  $limits[$k] = array_values(array_filter($times, function ($t) use ($now) { return $t > $now - 3600; }));
  if (!$limits[$k]) unset($limits[$k]);
}
$ip = hash('sha256', ($_SERVER['REMOTE_ADDR'] ?? '') . '|bar3d-reports');
if (count($limits[$ip] ?? []) >= PER_IP_PER_HOUR) {
  flock($lock, LOCK_UN);
  answer(429, ['error' => 'too many reports, try again in an hour']);
}
$limits[$ip][] = $now;
file_put_contents($limitFile, json_encode($limits));

// Old files out, and never more than MAX_FILES.
$files = glob("$dir/[A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9][A-Z0-9].json") ?: [];
$ages = [];
foreach ($files as $f) {
  $m = @filemtime($f);
  if ($m !== false && $m < $now - MAX_AGE) @unlink($f);
  else $ages[$f] = $m;
}
asort($ages);
while (count($ages) >= MAX_FILES) {
  $oldest = array_key_first($ages);
  @unlink($oldest);
  unset($ages[$oldest]);
}

$data['received'] = gmdate('c');
do {
  $code = '';
  for ($i = 0; $i < 6; $i++) $code .= CODE_CHARS[random_int(0, strlen(CODE_CHARS) - 1)];
} while (file_exists("$dir/$code.json"));
file_put_contents("$dir/$code.json", json_encode($data, JSON_UNESCAPED_SLASHES));
flock($lock, LOCK_UN);
answer(200, ['code' => $code]);
