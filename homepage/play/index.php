<?php
// joshuavelasco.com/play: the bar for people who haven't been in yet. What's
// inside, the games, how to get in, the usual questions.
require __DIR__ . '/../site/layout.php';
site_head('play', 'The bar', 'Bar3D: a 3D hangout bar in your browser. Racing rigs, a rhythm cabinet, darts, chess, a grand piano, a jukebox and real people. Free, nothing to install.');

$games = [
  ['Racing', 'Two sim rigs side by side. Race the bots or the person in the next seat while the room watches.', 'The racing rigs', '#FF5A3D', 'pad'],
  ['4K Mania', 'A four-lane rhythm game on its own glowing cabinet. Hit the notes, chase the combo, earn credits.', 'The 4K MANIA cabinet', '#FF3D8B', 'music'],
  ['Darts', 'Step up to the line and throw. A proper game of 301, scored for you.', 'The darts corner', '#F2C744', 'spark'],
  ['Chess', 'A real board in the lounge. Play the house or take on whoever sits across from you.', 'The lounge', '#EDE7D9', 'cube'],
  ['Platformer', 'A side-scroller in the arcade cabinet out back. One more try. Then one more.', 'The arcade', '#45D4FF', 'pad'],
  ['Meridian', 'Open road from a racing rig. Pick a car, take the long way, leave the bar behind for a bit.', 'A racing rig\'s menu', '#56CF86', 'globe'],
];
?>
<section class="hero hero-small">
  <div class="hero-media"><img src="/site/img/wide.jpg" alt="The whole bar from the corner: tables, string lights, the counter and the games" fetchpriority="high"></div>
  <div class="hero-in wrap">
    <span class="eyebrow reveal">Bar3D · open now</span>
    <h1 class="h-xl reveal" style="--d:.05s">Pull up a stool.</h1>
    <p class="lede reveal" style="--d:.1s">A cozy bar with neon, string lights and a fire going, plus a room full of games. It lives on the web, so the only thing between you and a seat at the counter is one tap.</p>
    <div class="hero-cta reveal" style="--d:.16s">
      <a class="btn btn-go btn-lg" href="/bar/">Enter the bar <span class="arr">→</span></a>
      <a class="btn btn-ghost btn-lg" href="#join">How to get in</a>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">A walk through</span>
      <h2 class="h-lg">Every corner does something.</h2>
    </div>

    <div class="split" style="margin-bottom:clamp(60px,9vw,120px)">
      <div class="copy reveal">
        <span class="eyebrow plain mono dim">01 · The counter</span>
        <h3 class="h-md">Sit at the bar.</h3>
        <p class="lede">Lit shelves, polished wood, a brass rail for your feet. Take a stool and the view swings round to the room. Behind the counter is the bartender's station, for whoever has earned the badge.</p>
      </div>
      <div class="shot reveal" style="--d:.08s"><img src="/site/img/counter.jpg" alt="The bar counter with stools and the lit back bar" loading="lazy"></div>
    </div>

    <div class="split flip" style="margin-bottom:clamp(60px,9vw,120px)">
      <div class="copy reveal">
        <span class="eyebrow plain mono dim">02 · The mirror</span>
        <h3 class="h-md">See yourself in there.</h3>
        <p class="lede">A full wall mirror that reflects the whole room, you included. Switch it to show only the people, or turn it off. Then wave at yourself. Everyone does.</p>
      </div>
      <div class="shot reveal" style="--d:.08s"><img src="/site/img/mirror.jpg" alt="The big mirror reflecting the bar and a person standing in it" loading="lazy"></div>
    </div>

    <div class="split" style="margin-bottom:clamp(60px,9vw,120px)">
      <div class="copy reveal">
        <span class="eyebrow plain mono dim">03 · The stage</span>
        <h3 class="h-md">Play the grand piano.</h3>
        <p class="lede">Sit down on the bench and play with your fingers, or plug in a keyboard. The keys and hammers move with the music. It knows the great classical pieces if you'd rather just listen.</p>
      </div>
      <div class="shot reveal" style="--d:.08s"><img src="/site/img/stage.jpg" alt="The stage with a grand piano next to a fireplace and bookshelves" loading="lazy"></div>
    </div>

    <div class="split flip">
      <div class="copy reveal">
        <span class="eyebrow plain mono dim">04 · The jukebox</span>
        <h3 class="h-md">Pick the music.</h3>
        <p class="lede">Smooth jazz by default. Walk up, choose a song, and the whole room hears it from right where the jukebox stands.</p>
      </div>
      <div class="shot reveal" style="--d:.08s"><img src="/site/img/jukebox.jpg" alt="The jukebox glowing by the wall" loading="lazy"></div>
    </div>
  </div>
</section>

<section class="section" id="games" style="background:linear-gradient(180deg,transparent,var(--bg-2) 20%,var(--bg-2) 80%,transparent)">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">Games inside</span>
      <h2 class="h-lg">The house plays too.</h2>
      <p class="lede">Every game lives in the room. Walk up to it, sit down, and you're playing. Your friends can stand behind you and watch.</p>
    </div>
    <div class="grid g3">
<?php foreach ($games as $i => [$name, $about, $where, $color, $icon]): ?>
      <article class="game reveal" style="--c:<?= $color ?>;--d:<?= ($i % 3) * 0.06 ?>s"><span class="glyph"><?= site_icon($icon) ?></span><span class="where"><?= site_h($where) ?></span><b><?= site_h($name) ?></b><span><?= site_h($about) ?></span></article>
<?php endforeach; ?>
    </div>
    <div class="grid g2" style="margin-top:16px">
      <div class="shot reveal"><img src="/site/img/rigs.jpg" alt="The two racing rigs with the race on their screens" loading="lazy"><span class="tag">Racing</span><span class="cap"><b>The rigs</b><span>Two seats, one track, no mercy.</span></span></div>
      <div class="shot reveal" style="--d:.08s"><img src="/site/img/chess.jpg" alt="The lounge: leather sofas, a chessboard and a fire" loading="lazy"><span class="tag">Chess</span><span class="cap"><b>The lounge</b><span>Leather sofas, a fire and a board set up.</span></span></div>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">You, in there</span>
      <h2 class="h-lg">Be someone.</h2>
    </div>
    <div class="grid g4">
      <article class="card reveal"><span class="ic"><?= site_icon('mic') ?></span><h3>Talk</h3><p>Voice chat that sounds like the room: people get quieter as you walk away.</p></article>
      <article class="card reveal" style="--d:.06s"><span class="ic"><?= site_icon('people') ?></span><h3>Your card</h3><p>A profile with your photo, your drink, your song and whatever else you want people to know.</p></article>
      <article class="card reveal" style="--d:.12s"><span class="ic"><?= site_icon('spark') ?></span><h3>Badges</h3><p>Bartender, DJ, VIP and more. Collect them, then pin your favorites to your card.</p></article>
      <article class="card reveal" style="--d:.18s"><span class="ic"><?= site_icon('hand') ?></span><h3>Emotes</h3><p>Wave, cheer, dance, bow, facepalm. Say it without saying it.</p></article>
    </div>
  </div>
</section>

<section class="section" id="join">
  <div class="wrap">
    <div class="section-head reveal">
      <span class="eyebrow">How to get in</span>
      <h2 class="h-lg">Three steps. One is "tap".</h2>
    </div>
    <ol class="steps">
      <li class="reveal"><b>Open the link</b><p>Go to <a href="/bar/">joshuavelasco.com/bar</a> on your phone or computer. Safari on iPhone works great.</p></li>
      <li class="reveal" style="--d:.06s"><b>Say who you are</b><p>Make an account in a few seconds so people know it's you, or come in as a guest and look around first.</p></li>
      <li class="reveal" style="--d:.12s"><b>Walk in</b><p>Drag to look around, tap the floor to walk, tap a stool to sit. That's the whole tutorial.</p></li>
    </ol>
  </div>
</section>

<section class="section tight">
  <div class="wrap" style="max-width:880px">
    <div class="section-head reveal"><span class="eyebrow">Questions</span><h2 class="h-md">Good questions.</h2></div>
    <div class="faq reveal">
      <details><summary>Do I need to download anything?</summary><p>No. It runs in your web browser. Open the link and you're in.</p></details>
      <details><summary>Does it work on my iPhone?</summary><p>That's where it's made. Every change is checked on an iPhone first. Recent Safari is all you need; computers with Chrome, Edge or Safari work too.</p></details>
      <details><summary>Is it free?</summary><p>Yes. Walk in, play, stay as long as you like.</p></details>
      <details><summary>Can I bring friends?</summary><p>Send them the link. Everyone in the bar sees and hears each other, and you can race, play chess or throw darts together.</p></details>
      <details><summary>Who can make an account?</summary><p>Anyone 13 or older. Your birth date stays private.</p></details>
      <details><summary>How often does it change?</summary><p>All the time. New things land most weeks, sometimes several times a day. The <a href="/news/">news</a> page shows what changed, with pictures.</p></details>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="cta reveal">
      <span class="eyebrow plain">Last call is never</span>
      <h2 class="h-lg">Your stool's waiting.</h2>
      <p>No line, no cover, no dress code.</p>
      <a class="btn btn-go btn-lg" href="/bar/">Enter the bar <span class="arr">→</span></a>
    </div>
  </div>
</section>
<?php site_foot();
