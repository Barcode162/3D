<?php
// joshuavelasco.com/projects (PLAN.md R5.4): every J Production project, one
// card each, from the one list (toolbelt/tools.json → projects, J5.5): a
// project with a picture is a big picture card, one without a card with its
// icon; where it's all going (vision/) closes the second row.
require __DIR__ . '/../site/layout.php';
site_head('projects', 'Projects', 'What J Production makes: Bar3D, a 3D bar you walk into from your browser; Looped, our own engine; and our own script language, coming.');
$all = site_project_list();
$shots = array_values(array_filter($all, fn($p) => $p['place'] === 'featured' && $p['img'] !== ''));
$cards = array_values(array_filter($all, fn($p) => $p['place'] === 'featured' && $p['img'] === ''));
$more = site_projects();
?>
<section class="hero hero-small">
  <div class="hero-media"><img src="/site/img/wide.jpg" alt="The whole bar from above the counter, lamps lit" fetchpriority="high"></div>
  <div class="hero-in wrap">
    <span class="eyebrow reveal">J Production</span>
    <h1 class="h-xl reveal" style="--d:.05s">What we make. <em>From scratch.</em></h1>
    <p class="lede reveal" style="--d:.1s">One world you can walk into today, and the engine and language we're building so there can be many more.</p>
  </div>
</section>

<section class="section tight">
  <div class="wrap">
    <div class="grid g2">
<?php foreach ($shots as $i => $p): ?>
      <a class="shot reveal"<?= $i ? ' style="--d:.' . sprintf('%02d', 8 * $i) . 's"' : '' ?> href="<?= site_h($p['url']) ?>"><img src="<?= site_h($p['img']) ?>" alt="<?= site_h($p['alt']) ?>" loading="lazy"><span class="tag"><?= site_h($p['tag']) ?></span><span class="cap"><b><?= site_h($p['name']) ?></b><span><?= site_h($p['description']) ?></span></span></a>
<?php endforeach; ?>
    </div>
    <div class="grid g2" style="margin-top:16px">
<?php foreach ($cards as $i => $p): ?>
      <article class="card glow reveal"<?= $i ? ' style="--d:.' . sprintf('%02d', 8 * $i) . 's"' : '' ?>><span class="idx mono"><?= site_h($p['tag']) ?></span><span class="ic"><?= site_icon($p['icon'] ?: 'rocket') ?></span><h3><?= site_h($p['name']) ?></h3><p><?= site_h($p['description']) ?></p></article>
<?php endforeach; ?>
      <a class="card glow reveal" style="--d:.<?= sprintf('%02d', 8 * count($cards)) ?>s" href="/vision/"><span class="idx mono">Where we're going</span><span class="ic"><?= site_icon('rocket') ?></span><h3>Don't fake the world. Simulate it.</h3><p>Light, sound, touch, heat, weather: each a real system, and every system makes the others stronger. The bar is just the first room.</p></a>
    </div>
<?php if ($more): ?>
    <div class="section-head" style="margin-top:clamp(48px,7vw,90px)"><span class="eyebrow">Also in the workshop</span><h2 class="h-md">More projects.</h2></div>
    <div class="projects">
<?php foreach ($more as $p): ?>
      <a href="<?= site_h($p['url']) ?>"><b><?= site_h($p['name']) ?></b><small><?= site_h($p['description']) ?></small></a>
<?php endforeach; ?>
    </div>
<?php endif; ?>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="cta reveal">
      <span class="eyebrow plain">The first world is open</span>
      <h2 class="h-xl"><span class="neon">Come in.</span></h2>
      <div class="hero-cta" style="justify-content:center"><a class="btn btn-go btn-lg" href="/bar/">Enter the bar <span class="arr">→</span></a></div>
    </div>
  </div>
</section>
<?php site_foot();
