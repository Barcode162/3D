<?php
// joshuavelasco.com/signin: sign in or make an account, with the bar's own
// accounts (bar/account.php, which also sets the login cookie the whole site
// reads). Staff go on to the portal; everyone else back where they came from.
require __DIR__ . '/../site/layout.php';
$v = site_viewer();
site_head('signin', 'Sign in', 'Sign in to Bar3D, or make an account in a few seconds.');
?>
<section class="auth">
  <div class="wrap" style="display:grid;place-items:center">
    <div class="auth-box">
<?php if ($v): ?>
      <div style="display:grid;gap:12px;text-align:center;justify-items:center">
        <span class="eyebrow plain">Signed in</span>
        <h1 class="h-md">Hey, <?= site_h($v['name']) ?>.</h1>
        <p class="dim">You're signed in as @<?= site_h($v['user']) ?>.</p>
      </div>
      <div class="panel">
        <a class="btn btn-go btn-lg" href="/bar/">Enter the bar <span class="arr">→</span></a>
<?php if ($v['role'] !== 'member'): ?>
        <a class="btn btn-ghost btn-lg" href="/portal/">Open the staff portal</a>
<?php endif; ?>
        <button class="btn btn-ghost" type="button" data-signout>Sign out</button>
      </div>
<?php else: ?>
      <div style="display:grid;gap:12px;text-align:center;justify-items:center">
        <a class="brand" href="/" aria-label="Home"><?= site_mark() ?></a>
        <h1 class="h-md" id="authTitle">Welcome back.</h1>
        <p class="dim" id="authSub">Sign in with your Bar3D account.</p>
      </div>
      <form class="panel" id="authForm" novalidate>
        <div class="seg" role="tablist" aria-label="Sign in or make an account">
          <button type="button" role="tab" aria-selected="true" data-mode="in">Sign in</button>
          <button type="button" role="tab" aria-selected="false" data-mode="new">New account</button>
        </div>
        <div class="fld"><label for="user">Username</label><input id="user" name="user" autocomplete="username" autocapitalize="none" spellcheck="false" required maxlength="20"></div>
        <div class="fld" data-new hidden><label for="name">Display name</label><input id="name" name="name" autocomplete="nickname" maxlength="20"><small>What people see over your head. You can change it later.</small></div>
        <div class="fld"><label for="pass">Password</label><input id="pass" name="pass" type="password" autocomplete="current-password" required minlength="6" maxlength="100"></div>
        <div class="fld" data-new hidden><label for="birth">Birth date</label><input id="birth" name="birth" type="date"><small>13 and up. It stays private: nobody sees it, not even staff.</small></div>
        <p class="form-msg" id="msg" role="status" aria-live="polite"></p>
        <button class="btn btn-go btn-lg" type="submit" id="go">Sign in</button>
      </form>
      <p class="fine">Just looking? <a href="/bar/">Come in as a guest</a>. Forgot your password? Use your recovery code at <a href="/bar/">the door</a>.</p>
<?php endif; ?>
    </div>
  </div>
</section>
<?php if (!$v): ?>
<script>
(function () {
  var form = document.getElementById('authForm');
  var mode = location.hash === '#new' ? 'new' : 'in';
  var tabs = form.querySelectorAll('[data-mode]');
  var go = document.getElementById('go'), msg = document.getElementById('msg');
  var next = new URLSearchParams(location.search).get('next') || '';
  if (!/^\/(?!\/)[\w\-\/.?=&#%]*$/.test(next)) next = '';

  function setMode(m) {
    mode = m;
    tabs.forEach(function (t) { t.setAttribute('aria-selected', t.getAttribute('data-mode') === m ? 'true' : 'false'); });
    form.querySelectorAll('[data-new]').forEach(function (el) { el.hidden = m !== 'new'; });
    document.getElementById('pass').autocomplete = m === 'new' ? 'new-password' : 'current-password';
    document.getElementById('authTitle').textContent = m === 'new' ? 'Pull up a stool.' : 'Welcome back.';
    document.getElementById('authSub').textContent = m === 'new' ? 'Make your account. Takes ten seconds.' : 'Sign in with your Bar3D account.';
    go.textContent = m === 'new' ? 'Make my account' : 'Sign in';
    msg.textContent = ''; msg.className = 'form-msg';
    history.replaceState(null, '', m === 'new' ? '#new' : location.pathname + location.search);
  }
  tabs.forEach(function (t) { t.addEventListener('click', function () { setMode(t.getAttribute('data-mode')); }); });
  setMode(mode);

  form.addEventListener('submit', function (e) {
    e.preventDefault();
    var body = { a: mode === 'new' ? 'signup' : 'login', user: form.user.value.trim().toLowerCase(), pass: form.pass.value };
    if (mode === 'new') { body.name = form.name.value.trim(); body.birth = form.birth.value; }
    if (!/^[a-z0-9_]{3,20}$/.test(body.user)) { msg.textContent = 'Usernames are 3-20 letters, numbers or _.'; return; }
    if (body.pass.length < 6) { msg.textContent = 'Passwords need at least 6 characters.'; return; }
    if (mode === 'new' && !body.birth) { msg.textContent = 'Please enter your birth date.'; return; }
    go.disabled = true; msg.className = 'form-msg'; msg.textContent = '';
    fetch('/bar/account.php', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify(body), credentials: 'same-origin' })
      .then(function (r) { return r.json(); })
      .then(function (j) {
        if (!j.ok) throw new Error(j.error || 'Something went wrong. Try again.');
        try { localStorage.setItem('bar3dToken', j.token); localStorage.setItem('bar3dProfile', JSON.stringify(j.me)); } catch (err) {}
        msg.className = 'form-msg ok';
        msg.textContent = j.me && j.me.admin ? 'Welcome in. Opening the portal…' : 'You\'re in.';
        setTimeout(function () { location.href = next || (j.me && j.me.admin ? '/portal/' : '/'); }, 450);
      })
      .catch(function (err) {
        go.disabled = false;
        msg.textContent = err && err.message && !/JSON|fetch/i.test(err.message) ? err.message : 'Couldn\'t reach the server. Try again.';
      });
  });
})();
</script>
<?php endif; ?>
<?php site_foot();
