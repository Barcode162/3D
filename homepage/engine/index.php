<?php
// joshuavelasco.com/engine: Looped, shown off without giving it away. What it
// does in plain words, what's been built, and a locked door (the Studio).
require __DIR__ . '/../site/layout.php';
site_head('engine', 'Looped, the engine', 'Looped is the engine behind the bar: written from scratch, tuned on a phone, running live in your browser. Here is what it does. How it does it is ours.');
?>
<section class="hero hero-small">
  <div class="hero-media"><img src="/site/img/counter.jpg" alt="Close up of the bar counter, lit bottles and glassware" fetchpriority="high"></div>
  <div class="hero-in wrap">
    <span class="eyebrow reveal">The engine · Looped</span>
    <h1 class="h-xl reveal" style="--d:.05s">We didn't pick an engine. <em>We wrote one.</em></h1>
    <p class="lede reveal" style="--d:.1s">Every pixel of light, every echo, every chair you sit in runs on code made here, from the very first line. No Unity. No Unreal. No shortcuts. That's why it feels like nothing else in a browser.</p>
  </div>
</section>

<section class="section tight">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">From tap to inside</span>
      <h2 class="h-md">Four steps. About two seconds.</h2>
    </div>
    <div class="flow">
      <div class="flow-step reveal"><span class="num mono">01</span><b>You tap the link</b><p>No store, no install, no account required to look around.</p></div>
      <div class="flow-step reveal" style="--d:.06s"><span class="num mono">02</span><b>Looped wakes up</b><p>The whole engine arrives in your browser and starts at native speed.</p></div>
      <div class="flow-step reveal" style="--d:.12s"><span class="num mono">03</span><b>The room builds</b><p>Walls, bottles, people, light. Streamed in, sharpest things first.</p></div>
      <div class="flow-step reveal" style="--d:.18s"><span class="num mono">04</span><b>You're inside</b><p>Drawn straight on your phone's graphics chip, every frame, live.</p></div>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">What it does</span>
      <h2 class="h-lg">It handles the senses.</h2>
      <p class="lede">Most engines give you a camera and wish you luck. Looped was built to make a place feel real: what you see, what you hear and what you touch.</p>
    </div>
    <div class="grid g3">
      <article class="card glow reveal"><span class="ic"><?= site_icon('light') ?></span><h3>Light</h3><p>Real materials that shine, scuff and glow. Soft shadows from every lamp, live reflections in the mirror, neon that spills onto the walls.</p></article>
      <article class="card glow reveal" style="--d:.06s"><span class="ic"><?= site_icon('sound') ?></span><h3>Sound</h3><p>Every sound comes from somewhere. Footsteps that match the floor, music that fills the room from the jukebox, voices that fade with distance.</p></article>
      <article class="card glow reveal" style="--d:.12s"><span class="ic"><?= site_icon('hand') ?></span><h3>Touch</h3><p>Seats you can sit in, keys you can press, darts that fly. Anything in the world can be something you do, not just something you see.</p></article>
      <article class="card glow reveal"><span class="ic"><?= site_icon('people') ?></span><h3>Together</h3><p>A live room server keeps everyone in the same moment: who's sitting where, whose turn it is, which song is on.</p></article>
      <article class="card glow reveal" style="--d:.06s"><span class="ic"><?= site_icon('phone') ?></span><h3>Phone first</h3><p>Tuned on an iPhone, frame by frame. If it isn't smooth in your pocket, it doesn't ship. Big screens get it easy.</p></article>
      <article class="card glow reveal" style="--d:.12s"><span class="ic"><?= site_icon('layers') ?></span><h3>It stacks</h3><p>Every new system makes the others better. Add a material once and it looks, sounds and moves right everywhere.</p></article>
    </div>
  </div>
</section>

<section class="section" style="background:linear-gradient(180deg,transparent,var(--bg-2) 25%,var(--bg-2) 75%,transparent)">
  <div class="wrap split">
    <div class="copy reveal">
      <span class="eyebrow">The formula</span>
      <h2 class="h-lg">You can look. <span class="hl">You can't have it.</span></h2>
      <p class="big-quote">People ask how a phone browser runs a whole bar with <b>real light</b>, <b>real sound</b> and <b>real people</b>. Good question.</p>
      <p class="dim">The answer is a lot of late nights and a recipe we're keeping. What we will tell you: it's ours, all of it, and it keeps getting better every week.</p>
    </div>
    <div class="vault reveal" style="--d:.1s" aria-label="A redacted look at the engine's code">
      <div class="vault-top"><i></i><i></i><i></i><span>looped / frame</span></div>
      <div class="ln"><span class="n">1</span><span class="c">// one frame, start to finish</span></div>
      <div class="ln"><span class="n">2</span><span><span class="k">void</span> Engine::frame() {</span></div>
      <div class="ln"><span class="n">3</span><span>&nbsp;&nbsp;<span class="redact" style="width:18ch"></span>;</span></div>
      <div class="ln"><span class="n">4</span><span>&nbsp;&nbsp;<span class="redact" style="width:12ch"></span>(<span class="redact" style="width:8ch"></span>);</span></div>
      <div class="ln"><span class="n">5</span><span>&nbsp;&nbsp;<span class="k">for</span> (<span class="redact" style="width:20ch"></span>)</span></div>
      <div class="ln"><span class="n">6</span><span>&nbsp;&nbsp;&nbsp;&nbsp;<span class="redact" style="width:24ch"></span>;</span></div>
      <div class="ln"><span class="n">7</span><span>&nbsp;&nbsp;light(<span class="redact" style="width:10ch"></span>);</span></div>
      <div class="ln"><span class="n">8</span><span>&nbsp;&nbsp;sound(<span class="redact" style="width:14ch"></span>);</span></div>
      <div class="ln"><span class="n">9</span><span>&nbsp;&nbsp;<span class="redact" style="width:16ch"></span> <span class="c">// the good part</span></span></div>
      <div class="ln"><span class="n">10</span><span>&nbsp;&nbsp;present();</span></div>
      <div class="ln"><span class="n">11</span><span>}</span></div>
      <span class="stamp">Ours</span>
    </div>
  </div>
</section>

<section class="section tight">
  <div class="wrap">
    <div class="stats reveal">
      <div class="stat"><b>0</b><span>outside engines or game libraries inside Looped.</span></div>
      <div class="stat"><b><span data-count="60">60</span><small>fps</small></b><span>the target on a phone, every frame, in a full room.</span></div>
      <div class="stat"><b data-count="129942">129,942</b><span>triangles in the race track, built in about 50 ms.</span></div>
      <div class="stat"><b><span data-count="6">6</span><small>min</small></b><span>from a change being made to it being live for everyone.</span></div>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap split">
    <div class="copy reveal">
      <span class="eyebrow">The road so far</span>
      <h2 class="h-lg">Built a piece at a time.</h2>
      <p class="lede">Each step is checked on a real phone before the next one starts. Here's what's stacked up.</p>
    </div>
    <ol class="timeline reveal" style="--d:.08s">
      <li><span class="when">Foundation</span><b>The core</b><p>Drawing, timing, loading, the bones everything else stands on.</p></li>
      <li><span class="when">The room</span><b>A bar made of real things</b><p>Every object became something alive: it knows what it is and what it does.</p></li>
      <li><span class="when">Hands on</span><b>Touch, seats, the jukebox, darts</b><p>Tap anything, sit anywhere, and the room agrees on what happened.</p></li>
      <li><span class="when">Looks</span><b>Shadows, smoke, reflections</b><p>Soft shadows, haze in the lamplight and a mirror that shows the whole room.</p></li>
      <li><span class="when">Life</span><b>Idle moves and real sound</b><p>People shift and breathe; the room has its own sounds.</p></li>
      <li><span class="when">Reach</span><b>Travel and a test world</b><p>Walk from place to place; build new rooms without touching the bar.</p></li>
      <li class="now"><span class="when">Now</span><b>The Studio</b><p>A full world editor, made in house, so the world can grow faster than ever.</p></li>
      <li class="later"><span class="when">Next</span><b>Physics, heat, weather</b><p>Pick things up and throw them. Candles that warm. Rain on the window.</p></li>
    </ol>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="cta reveal" style="text-align:left;justify-items:start">
      <span class="ic" style="width:56px;height:56px;border-radius:14px;display:grid;place-items:center;background:rgba(242,140,44,.12);box-shadow:inset 0 0 0 1px rgba(242,140,44,.35);color:var(--orange)"><?= site_icon('lock') ?></span>
      <span class="eyebrow">Behind a locked door</span>
      <h2 class="h-lg">There's a studio. It's not for you. <span class="hl">Yet.</span></h2>
      <p>The world is built in our own editor: move anything, light anything, publish it to the live bar in one tap. It's staff only. If you're staff, you already know where it is.</p>
      <div class="hero-cta">
        <a class="btn btn-go btn-lg" href="/bar/">See the result <span class="arr">→</span></a>
<?php if (site_is_staff()): ?>
        <a class="btn btn-ghost btn-lg" href="/dev/">Open Dev</a>
<?php endif; ?>
      </div>
    </div>
  </div>
</section>
<?php site_foot();
