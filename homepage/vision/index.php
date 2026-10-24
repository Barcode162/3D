<?php
// joshuavelasco.com/vision: where it's all heading (docs/archive/VISION.md, told for
// visitors). Inspiring, not technical.
require __DIR__ . '/../site/layout.php';
site_head('vision', 'Where we\'re going', 'Not a set of props: a world that simulates what you see, hear and feel. The bar is the first room. Here is where it goes next.');
?>
<section class="hero hero-small">
  <div class="hero-media"><img src="/site/img/arcade.jpg" alt="The arcade room glowing with neon lines" fetchpriority="high"></div>
  <div class="hero-in wrap">
    <span class="eyebrow reveal">J Production · where we're going</span>
    <h1 class="h-xl reveal" style="--d:.05s">Don't fake the world. <em>Simulate it.</em></h1>
    <p class="lede reveal" style="--d:.1s">Most virtual places are stage sets: painted walls, canned sounds, props that do one trick. We're building the opposite. Systems that behave like the real thing, so the world can surprise even us.</p>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <p class="manifesto reveal">One rule: <span class="soft">every new piece makes</span> every other piece better.</p>
    <p class="lede reveal" style="margin-top:28px;max-width:56ch">We call it "it stacks". Teach the world what glass is once, and every glass in every room looks like glass, rings like glass and breaks like glass. No one-off tricks. Just a world that gets richer each time something is added.</p>
  </div>
</section>

<section class="section tight">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">Built on the senses</span>
      <h2 class="h-lg">If you can sense it, we simulate it.</h2>
    </div>
    <div class="grid g2">
      <article class="card glow reveal"><span class="idx mono">SIGHT</span><span class="ic"><?= site_icon('eye') ?></span><h3>Light that tells the truth</h3><p>Materials that react to light like the real ones: brass that gleams, leather that's worn where hands go, glass that bends what's behind it. Shadows, reflections and haze, all live.</p></article>
      <article class="card glow reveal" style="--d:.06s"><span class="idx mono">SOUND</span><span class="ic"><?= site_icon('sound') ?></span><h3>Sound made by the world</h3><p>A glass clinks because two things hit, and it sounds like what they're made of. Rooms echo because of their shape. Walls muffle what's behind them.</p></article>
      <article class="card glow reveal"><span class="idx mono">TOUCH</span><span class="ic"><?= site_icon('hand') ?></span><h3>Weight you can feel</h3><p>Pick things up, throw them, stack them, knock them over. Then soft things: cushions that squash, cloth that drapes. Then liquids: pour a drink and watch it fill.</p></article>
      <article class="card glow reveal" style="--d:.06s"><span class="idx mono">BEYOND</span><span class="ic"><?= site_icon('flame') ?></span><h3>Heat, air, time</h3><p>Candles that warm the air, ice that melts in your drink, a smell that drifts across the room, day turning to night outside the window, wear building up where people gather.</p></article>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap split">
    <div class="shot tall reveal"><img src="/site/img/mirror.jpg" alt="The big mirror reflecting the room and someone standing in it" loading="lazy"></div>
    <div class="copy reveal" style="--d:.08s">
      <span class="eyebrow">You, for real</span>
      <h2 class="h-lg">An avatar that looks like you.</h2>
      <p class="lede">Not a cartoon from a menu. A body with real bones and real motion, built from a scan of your own face. Walk past the mirror and recognise yourself.</p>
      <ul class="ticks">
        <li><span><b>Mirrors like the best social worlds.</b> One shows everything, one just the people.</span></li>
        <li><span><b>A move for everything.</b> Sitting, dancing, playing, waving.</span></li>
        <li><span><b>Your voice, crystal clear.</b> As close to lossless as it gets.</span></li>
      </ul>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap split flip">
    <div class="shot tall reveal"><img src="/site/img/jukebox.jpg" alt="The jukebox glowing on the green wall" loading="lazy"></div>
    <div class="copy reveal" style="--d:.08s">
      <span class="eyebrow">Music</span>
      <h2 class="h-lg">A studio inside the bar.</h2>
      <p class="lede">Synths, a drum machine, a sequencer and a mixer, right there in the room. Make a beat at a table, then put it on the jukebox for everyone. Maybe one day it leaves the bar as a real music tool.</p>
    </div>
  </div>
</section>

<section class="section" style="background:linear-gradient(180deg,transparent,var(--bg-2) 20%,var(--bg-2) 80%,transparent)">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">Now · Next · Later</span>
      <h2 class="h-lg">The road ahead.</h2>
      <p class="lede">The plan bends as we learn. The direction doesn't.</p>
    </div>
    <div class="grid g3">
      <article class="card reveal"><span class="eyebrow plain" style="color:var(--green)">● Now</span><h3>The Studio</h3><p>Our own world editor, made for building on a phone: move, light and publish anything in the bar in one tap.</p></article>
      <article class="card reveal" style="--d:.06s"><span class="eyebrow plain">● Next</span><h3>Things with weight</h3><p>Real physics: glasses you can lift and throw, stools you can knock over, darts that bounce off the wire.</p></article>
      <article class="card reveal" style="--d:.12s"><span class="eyebrow plain" style="color:var(--text-3)">● Later</span><h3>More rooms, more worlds</h3><p>Weather outside the windows, new places to walk to, and your own avatar, scanned from you.</p></article>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap" style="max-width:900px">
    <span class="eyebrow reveal">Why</span>
    <p class="big-quote reveal" style="margin-top:22px">Because the best way to find out what's possible is to <b>build the whole thing yourself</b>, then keep going past where anyone said you'd stop.</p>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="cta reveal">
      <span class="eyebrow plain">See it for yourself</span>
      <h2 class="h-lg">The first room is open.</h2>
      <p>Everything on this page starts in there. Come see how far it's come.</p>
      <a class="btn btn-go btn-lg" href="/bar/">Enter the bar <span class="arr">→</span></a>
    </div>
  </div>
</section>
<?php site_foot();
