<?php
// joshuavelasco.com/projects (PLAN.md R5.4): every J Production project, one
// card each. Bar3D's page is play/, Looped's is engine/, where it's all going
// is vision/. A new project folder with a project.json joins the list.
require __DIR__ . '/../site/layout.php';
site_head('projects', 'Projects', 'What J Production makes: Bar3D, a 3D bar you walk into from your browser; Looped, our own engine; and our own script language, coming.');
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
      <a class="shot reveal" href="/play/"><img src="/site/img/counter.jpg" alt="The bar counter under warm lamps" loading="lazy"><span class="tag">Live · play now</span><span class="cap"><b>Bar3D</b><span>A living 3D bar in your browser: games, music and real people. Our first world.</span></span></a>
      <a class="shot reveal" style="--d:.08s" href="/engine/"><img src="/site/img/backbar.jpg" alt="Bottles glowing on the back bar" loading="lazy"><span class="tag">In progress · admins and devs</span><span class="cap"><b>Looped, the engine</b><span>Ours from the first line: light, sound, touch and people, live on a phone.</span></span></a>
    </div>
    <div class="grid g2" style="margin-top:16px">
      <article class="card glow reveal"><span class="idx mono">Planned</span><span class="ic"><?= site_icon('cursor') ?></span><h3>The script language</h3><p>A small, safe language for the things that live in our worlds: a candle that flickers, a jukebox that knows the mood, a door that remembers who came in.</p></article>
      <a class="card glow reveal" style="--d:.08s" href="/vision/"><span class="idx mono">Where we're going</span><span class="ic"><?= site_icon('rocket') ?></span><h3>Don't fake the world. Simulate it.</h3><p>Light, sound, touch, heat, weather: each a real system, and every system makes the others stronger. The bar is just the first room.</p></a>
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
