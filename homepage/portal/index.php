<?php
// joshuavelasco.com/portal: the staff portal. Staff are the bar's owner and
// admins (the Studio's own rule, studio/api.php). Anyone else, signed in or
// not, gets the site's ordinary 404: guests never learn it exists.
// Each tool still checks the login itself; this page only gathers them.
require __DIR__ . '/../site/layout.php';
header('X-Robots-Tag: noindex, nofollow');
if (!site_is_staff()) site_404();
// The portal became Dev (PLAN.md R4.7): staff are sent there.
header('Location: /dev/', true, 302);
exit;
$v = site_viewer();
$owner = $v['role'] === 'owner';
$hour = (int)date('G');
$hello = $hour < 5 ? 'Up late' : ($hour < 12 ? 'Morning' : ($hour < 18 ? 'Afternoon' : 'Evening'));
$first = trim($v['name']) !== '' ? trim($v['name']) : $v['user'];

// [href, icon, title, text, owner only]
$tools = [
  ['/bar/', 'door', 'The live bar', 'Walk the floor as staff. What players see, right now.', false],
  ['/bar/gate.php?people', 'people', 'People', 'Who\'s signed up, who\'s waiting. Invite players, choose admins.', false],
  ['/hub/', 'pulse', 'The hub', 'Needs you, builds, the board and the news desk.', true],
  ['/hub/dials.html', 'sliders', 'Dials', 'Live tuning for the bar: light, sound, feel. Baked into the world.', true],
  ['/news/', 'news', 'Public news', 'The posts the world can see, with their pictures.', false],
  ['/classic/', 'cube', 'Bar3D classic', 'The old public build, kept as it was.', false],
];
site_head('portal', 'Staff portal', 'Staff only.');
?>
<section class="wrap portal-hero">
  <span class="badge-role reveal"><?= site_icon('key') ?> <?= $owner ? 'Owner' : 'Staff' ?> access</span>
  <h1 class="h-lg reveal" style="--d:.04s"><?= site_h($hello) ?>, <?= site_h($first) ?>.</h1>
  <p class="lede reveal" style="--d:.08s">This is the back room. Everything that builds, tunes and runs the bar is here. Guests can't see this page, or the tab that led you to it.</p>
  <p class="live reveal" style="--d:.12s"><span class="pulse" aria-hidden="true"></span><span data-live-text>Checking the live build…</span></p>
</section>

<section class="wrap" style="padding-bottom:clamp(48px,7vw,90px)">
  <div class="tiles">
    <a class="tile hero-tile reveal" href="/studio/">
      <span class="go"><?= site_icon('out') ?></span>
      <span class="ic"><?= site_icon('cursor') ?></span>
      <b>Open the Studio</b>
      <p style="max-width:46ch">Move, size and light anything in the world. Check it in the Monitor, then publish to the live bar in one tap. Every version is kept, so nothing is ever lost.</p>
      <span class="lock">Owner and admins · works on your phone</span>
    </a>
<?php foreach ($tools as $i => [$href, $icon, $title, $text, $ownerOnly]):
  if ($ownerOnly && !$owner) continue; ?>
    <a class="tile reveal" style="--d:<?= ($i % 3) * 0.05 ?>s" href="<?= site_h($href) ?>">
      <span class="go"><?= site_icon('out') ?></span>
      <span class="ic"><?= site_icon($icon) ?></span>
      <b><?= site_h($title) ?></b>
      <p><?= site_h($text) ?></p>
<?php if ($ownerOnly): ?>
      <span class="lock">Owner only</span>
<?php endif; ?>
    </a>
<?php endforeach; ?>
  </div>
</section>

<section class="wrap" style="padding-bottom:clamp(60px,9vw,120px)">
  <div class="section-head" style="margin-bottom:24px"><span class="eyebrow">Quick tools</span></div>
  <div class="tool-row">
    <div class="tool reveal">
      <b class="h-sm">Performance report</b>
      <p class="dim" style="font-size:15px">Someone sent a report from the bar (<span class="mono">?perf=1</span> → Send report)? Type its six-letter code to open it.</p>
      <form id="reportForm"><input id="reportCode" maxlength="6" placeholder="K7Q2XD" aria-label="Report code" autocomplete="off" autocapitalize="characters" spellcheck="false"><button class="btn btn-go" type="submit">Open</button></form>
    </div>
    <div class="tool reveal" style="--d:.06s">
      <b class="h-sm">You</b>
      <dl class="kv">
        <dt>Name</dt><dd><?= site_h($v['name']) ?></dd>
        <dt>Username</dt><dd class="mono">@<?= site_h($v['user']) ?></dd>
        <dt>Access</dt><dd><?= $owner ? 'Owner: everything, including the hub and choosing admins' : 'Admin: the Studio, people and invites' ?></dd>
        <dt>Live build</dt><dd class="mono" data-build>…</dd>
      </dl>
      <button class="btn btn-ghost btn-sm" type="button" data-signout style="width:fit-content">Sign out</button>
    </div>
  </div>
</section>
<script>
document.getElementById('reportForm').addEventListener('submit', function (e) {
  e.preventDefault();
  var c = document.getElementById('reportCode').value.trim().toUpperCase();
  if (/^[A-Z0-9]{6}$/.test(c)) location.href = '/bar/report.php?code=' + encodeURIComponent(c);
});
</script>
<?php site_foot();
