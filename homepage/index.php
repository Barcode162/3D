<?php
// joshuavelasco.com, the front page (the IONOS "home/" folder). The frame
// (tabs, footer, who is signed in) is site/layout.php; the look is
// site/site.css. The other pages: play/, engine/, vision/, signin/, portal/.
require __DIR__ . '/site/layout.php';
site_head('home', 'Home', 'J Production builds its own engine, its own language and the worlds that run on them. First stop: a living 3D bar you walk into from your browser.');
$projects = site_projects();
// What Dev → Publish sets (PLAN.md R5.2/R5.3): the top words, and the one highlight pushed to the front page.
$devData = getenv('DEV_DATA') ?: dirname(__DIR__) . '/private/dev';
$siteWords = site_json("$devData/site.json") ?? [];
$headline = trim((string)($siteWords['headline'] ?? '')) ?: 'We build worlds. From nothing.';
$lede = trim((string)($siteWords['lede'] ?? '')) ?: 'J Production makes its own engine, its own language and the worlds that run on them. The first one is a bar you walk into from your phone.';
$highlight = null;
foreach (site_json("$devData/highlights.json") ?? [] as $h) if (!empty($h['pushed'])) $highlight = $h;
if ($highlight) $highlight['img'] = ($highlight['pic'] ?? '') !== '' ? $highlight['pic'] : (is_file("$devData/highlights/{$highlight['id']}.jpg") ? "/dev/api.php?hi={$highlight['id']}" : '/site/img/wide.jpg');
// The headline's last sentence glows, like before ("We built a world. <em>From nothing.</em>").
$parts = preg_split('/(?<=[.!?])\s+(?=\S)/', $headline, 2);
?>
<section class="hero">
  <div class="hero-media"><picture><source media="(max-width: 700px)" srcset="/site/img/hero-tall.jpg"><img src="/site/img/hero.jpg" alt="Inside the bar: string lights, neon signs, the back bar glowing behind the counter" fetchpriority="high"></picture></div>
  <div class="hero-in wrap">
    <span class="eyebrow plain reveal">J Production</span>
    <h1 class="h-xl reveal" style="--d:.05s"><?php if (count($parts) === 2): ?><?= site_h($parts[0]) ?> <em><?= site_h($parts[1]) ?></em><?php else: ?><?= site_h($headline) ?><?php endif; ?></h1>
    <p class="lede reveal" style="--d:.12s"><?= site_h($lede) ?></p>
    <div class="hero-cta reveal" style="--d:.18s">
      <a class="btn btn-go btn-lg" href="/bar/">Enter the bar <span class="arr">→</span></a>
      <a class="btn btn-ghost btn-lg" href="#bar3d">See the bar</a>
    </div>
    <a class="pill reveal" style="--d:.24s" href="/bar/"><b>Open</b><span data-live-text>Doors open · live now</span></a>
  </div>
</section>

<div class="marquee" aria-hidden="true">
  <div class="marquee-track">
<?php for ($r = 0; $r < 2; $r++): foreach (['Racing rigs', '4K Mania', 'Darts', 'Chess', 'Grand piano', 'Jukebox', 'Live voice', 'Mirrors', 'The arcade', 'Emotes', 'Bartender station', 'Badges'] as $w): ?>
    <span><?= site_h($w) ?></span>
<?php endforeach; endfor; ?>
  </div>
</div>

<?php if ($highlight): ?>
<section class="section tight highlight-sec">
  <div class="wrap">
    <a class="shot highlight reveal" href="<?= site_h($highlight['link'] ?: '/news/') ?>"><img src="<?= site_h($highlight['img']) ?>" alt="" loading="eager"><span class="tag">New</span><span class="cap"><b><?= site_h($highlight['title']) ?></b><?php if (($highlight['text'] ?? '') !== ''): ?><span><?= site_h($highlight['text']) ?></span><?php endif; ?></span></a>
  </div>
</section>
<?php endif; ?>

<section class="section" id="bar3d">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">Bar3D · our first world</span>
      <h2 class="h-lg">Not a video. Not a download. <span class="hl">A place.</span></h2>
      <p class="lede">Tap a link and you're standing inside. Other people are in there too. The lights are on, the jukebox is playing, and somebody's losing at darts.</p>
    </div>
    <div class="grid g3">
      <article class="card glow reveal"><span class="idx mono">01</span><span class="ic"><?= site_icon('door') ?></span><h3>Walk in</h3><p>One tap and you're in. No app store, no 40 GB patch, no waiting room. It opens in Safari like any web page, then it isn't one.</p></article>
      <article class="card glow reveal" style="--d:.08s"><span class="idx mono">02</span><span class="ic"><?= site_icon('people') ?></span><h3>Hang out</h3><p>See people walk in. Hear them from where they're standing. Grab a stool at the counter, wave across the room, put your song on.</p></article>
      <article class="card glow reveal" style="--d:.16s"><span class="idx mono">03</span><span class="ic"><?= site_icon('pad') ?></span><h3>Play</h3><p>Two racing rigs, a rhythm cabinet, a dartboard, a chessboard in the lounge and an arcade in the back. All inside. All live.</p></article>
    </div>
  </div>
</section>

<section class="section tight">
  <div class="wrap split">
    <div class="copy reveal">
      <span class="eyebrow">Real time, on a phone</span>
      <h2 class="h-md">Every bottle. Every bulb. Every reflection.</h2>
      <p class="lede">This isn't a render or a trailer. It's what your phone draws, live, sixty times a second, while you walk around in it.</p>
      <ul class="ticks">
        <li><span><b>Light that behaves.</b> Lamps cast shadows, glass catches the neon, the floor shines where it's polished.</span></li>
        <li><span><b>Sound with a place.</b> The jukebox sounds like it's over there, because it is.</span></li>
        <li><span><b>Things you can touch.</b> Sit down, throw darts, press keys on the piano, flip the mirror on.</span></li>
      </ul>
    </div>
    <a class="shot reveal" style="--d:.1s" href="/play/"><img src="/site/img/backbar.jpg" alt="The back bar: shelves of lit bottles, pendant lamps and the bartender's station" loading="lazy"><span class="tag">Live capture</span><span class="cap"><b>The back bar</b><span>Captured in the browser. No touch-ups.</span></span></a>
  </div>
</section>

<section class="section" id="games">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">Inside the bar</span>
      <h2 class="h-lg">Something in every corner.</h2>
    </div>
    <div class="grid g2">
      <a class="shot reveal" href="/play/#games"><img src="/site/img/rigs.jpg" alt="Two racing simulator rigs with screens" loading="lazy"><span class="tag">Race</span><span class="cap"><b>Racing rigs</b><span>Sit in, grip the wheel, race a friend or the bots.</span></span></a>
      <a class="shot reveal" style="--d:.08s" href="/play/#games"><img src="/site/img/rhythm.jpg" alt="The 4K Mania rhythm cabinet glowing in the corner" loading="lazy"><span class="tag">Rhythm</span><span class="cap"><b>4K Mania</b><span>Four lanes, real charts, your thumbs against the beat.</span></span></a>
      <a class="shot reveal" href="/play/#games"><img src="/site/img/stage.jpg" alt="The stage with a grand piano, by the fire" loading="lazy"><span class="tag">Music</span><span class="cap"><b>The stage</b><span>A grand piano by the fire. The keys move when you play.</span></span></a>
      <a class="shot reveal" style="--d:.08s" href="/play/#games"><img src="/site/img/arcade.jpg" alt="The arcade room under neon" loading="lazy"><span class="tag">Arcade</span><span class="cap"><b>The arcade</b><span>Through the neon arch in the back. Bring coins.</span></span></a>
    </div>
    <p class="reveal" style="margin-top:28px"><a class="arrow-link" href="/play/">Everything you can do in there <span>→</span></a></p>
  </div>
</section>

<section class="section" id="made-here">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">Made here</span>
      <h2 class="h-lg">Our own engine. <span class="hl">Our own language.</span></h2>
      <p class="lede">Everything above runs on tools we build ourselves. They're works in progress, used by our admins and devs, and every world we make grows them.</p>
    </div>
    <div class="grid g2">
      <a class="card glow reveal" href="/engine/"><span class="idx mono">In progress</span><span class="ic"><?= site_icon('cube') ?></span><h3>Looped, the engine</h3><p>Written from the first line, tuned until a phone can carry a whole bar: light, sound, touch and people, live.</p></a>
      <a class="card glow reveal" style="--d:.08s" href="/vision/"><span class="idx mono">Planned</span><span class="ic"><?= site_icon('cursor') ?></span><h3>The script language</h3><p>A small, safe language for things that live in our worlds: a candle that flickers, a jukebox that knows the mood.</p></a>
    </div>
  </div>
</section>

<section class="section tight" data-news hidden>
  <div class="wrap">
    <div class="section-head">
      <span class="eyebrow">Fresh off the line</span>
      <h2 class="h-md">What changed lately.</h2>
    </div>
    <div class="news-list"></div>
    <p style="margin-top:24px"><a class="arrow-link" href="/news/">All the news <span>→</span></a></p>
  </div>
</section>

<?php if ($projects): ?>
<section class="section tight">
  <div class="wrap">
    <div class="section-head"><span class="eyebrow">Also from the workshop</span><h2 class="h-md">More projects.</h2></div>
    <div class="projects">
<?php foreach ($projects as $p): ?>
      <a href="<?= site_h($p['url']) ?>"><b><?= site_h($p['name']) ?></b><small><?= site_h($p['description']) ?></small></a>
<?php endforeach; ?>
    </div>
  </div>
</section>
<?php endif; ?>

<section class="section">
  <div class="wrap">
    <div class="cta reveal">
      <span class="eyebrow plain">Tonight · and every night</span>
      <h2 class="h-xl"><span class="neon">Open.</span></h2>
      <p>The doors don't lock and the first round is on us. Come in, bring your friends, stay as long as you like.</p>
      <div class="hero-cta" style="justify-content:center">
        <a class="btn btn-go btn-lg" href="/bar/">Come on in <span class="arr">→</span></a>
        <a class="btn btn-ghost btn-lg" href="/signin/#new">Make an account</a>
      </div>
    </div>
  </div>
</section>
<?php site_foot();
