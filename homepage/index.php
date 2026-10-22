<?php
// joshuavelasco.com, the front page (the IONOS "home/" folder). The frame
// (tabs, footer, who is signed in) is site/layout.php; the look is
// site/site.css. The other pages: play/, engine/, vision/, signin/, portal/.
require __DIR__ . '/site/layout.php';
site_head('home', 'Home', 'A living 3D bar you walk into from your browser, built on our own engine from the first line of code. The doors are open: come in and play.');
$projects = site_projects();
?>
<section class="hero">
  <div class="hero-media"><picture><source media="(max-width: 700px)" srcset="/site/img/hero-tall.jpg"><img src="/site/img/hero.jpg" alt="Inside the bar: string lights, neon signs, the back bar glowing behind the counter" fetchpriority="high"></picture></div>
  <div class="hero-in wrap">
    <a class="pill reveal" href="/bar/"><b>Open</b><span data-live-text>Doors open · live now</span></a>
    <h1 class="h-xl reveal" style="--d:.05s">We built a world. <em>From nothing.</em></h1>
    <p class="lede reveal" style="--d:.12s">A living 3D bar you walk into straight from your browser. Our own engine, our own tools, zero downloads. Pull up a stool. Everyone's welcome.</p>
    <div class="hero-cta reveal" style="--d:.18s">
      <a class="btn btn-go btn-lg" href="/bar/">Enter the bar <span class="arr">→</span></a>
      <a class="btn btn-ghost btn-lg" href="#inside">See what's inside</a>
    </div>
    <div class="hero-meta reveal" style="--d:.24s"><span>Runs on your iPhone</span><span>Nothing to install</span><span>Real people, live</span><span>Free</span></div>
  </div>
</section>

<div class="marquee" aria-hidden="true">
  <div class="marquee-track">
<?php for ($r = 0; $r < 2; $r++): foreach (['Racing rigs', '4K Mania', 'Darts', 'Chess', 'Grand piano', 'Jukebox', 'Live voice', 'Mirrors', 'The arcade', 'Emotes', 'Bartender station', 'Badges'] as $w): ?>
    <span><?= site_h($w) ?></span>
<?php endforeach; endfor; ?>
  </div>
</div>

<section class="section" id="inside">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">What this is</span>
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

<section class="section">
  <div class="wrap split flip">
    <div class="copy reveal">
      <span class="eyebrow">The engine</span>
      <h2 class="h-lg">Looped. <span class="hl">Ours.</span></h2>
      <p class="lede">Everyone else rents an engine. We wrote ours, from the first line, and tuned it until a phone could carry a whole bar. You'll see everything it does. How it does it stays in the vault.</p>
      <p><a class="arrow-link" href="/engine/">What it can do <span>→</span></a></p>
    </div>
    <div class="vault reveal" style="--d:.1s" aria-label="A redacted look at the engine's code">
      <div class="vault-top"><i></i><i></i><i></i><span>looped / classified</span></div>
      <div class="ln"><span class="n">1</span><span><span class="k">namespace</span> engine {</span></div>
      <div class="ln"><span class="n">2</span><span class="c">// the part everyone asks about</span></div>
      <div class="ln"><span class="n">3</span><span>&nbsp;&nbsp;<span class="k">void</span> <span class="redact" style="width:9ch"></span>(<span class="redact" style="width:13ch"></span>) {</span></div>
      <div class="ln"><span class="n">4</span><span>&nbsp;&nbsp;&nbsp;&nbsp;<span class="redact" style="width:22ch"></span></span></div>
      <div class="ln"><span class="n">5</span><span>&nbsp;&nbsp;&nbsp;&nbsp;<span class="redact" style="width:16ch"></span> <span class="s">"nice try"</span>;</span></div>
      <div class="ln"><span class="n">6</span><span>&nbsp;&nbsp;&nbsp;&nbsp;<span class="redact" style="width:26ch"></span></span></div>
      <div class="ln"><span class="n">7</span><span>&nbsp;&nbsp;&nbsp;&nbsp;<span class="redact" style="width:11ch"></span></span></div>
      <div class="ln"><span class="n">8</span><span>&nbsp;&nbsp;}</span></div>
      <div class="ln"><span class="n">9</span><span>}</span></div>
      <span class="stamp">Classified</span>
    </div>
  </div>
  <div class="wrap" style="margin-top:clamp(48px,7vw,90px)">
    <div class="stats reveal">
      <div class="stat"><b>0</b><span>borrowed game engines. Not Unity, not Unreal, not anything.</span></div>
      <div class="stat"><b data-count="129942">129,942</b><span>triangles in the race track alone, streamed into a phone.</span></div>
      <div class="stat"><b><span data-count="6">6</span><small>min</small></b><span>from a new idea to live, for everyone.</span></div>
      <div class="stat"><b>1</b><span>phone it all answers to. If it isn't smooth there, it isn't done.</span></div>
    </div>
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

<section class="section">
  <div class="wrap">
    <span class="eyebrow reveal">Where this is going</span>
    <p class="manifesto reveal" style="margin:22px 0 30px">Most games fake the world. <span class="soft">We're building one that</span> simulates it.</p>
    <p class="lede reveal">Light, sound, touch, heat, weather. Each one a real system, and every system makes the others stronger. The bar is just the first room.</p>
    <p class="reveal" style="margin-top:26px"><a class="arrow-link" href="/vision/">Read the vision <span>→</span></a></p>
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
