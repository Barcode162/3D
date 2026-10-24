// Dev → People (PLAN.md R4.6): everyone with an account (the bar's
// account.php: people, invite, admin). Admins invite or remove players; only
// the owner chooses admins. Moved here from the bar's gate page.
(function () {
  'use strict';
  var D = window.Dev, ACCOUNT = '/bar/account.php';
  var LABEL = { owner: 'Owner', admin: 'Admin', player: 'Player', waiting: 'Waiting' };
  var ORDER = { owner: 0, admin: 1, waiting: 2, player: 3 };
  function act(a, user, on) {
    return D.post(ACCOUNT, { a: a, user: user, on: on ? 1 : 0 }).then(function (j) {
      if (!j.ok) { D.toast(j.error || 'That didn\'t work.'); return false; }
      return true;
    });
  }
  function draw() {
    var slot = D.slot('people'); if (!slot) return;
    (D.demo ? Promise.resolve({ ok: 1, people: [{ user: 'demo', name: 'Demo', joined: Date.now() / 1000, status: 'owner' }] }) : D.post(ACCOUNT, { a: 'people' })).then(function (j) {
      slot = D.slot('people'); if (!slot) return;
      if (!j.ok) { slot.appendChild(D.part({ type: 'listrow', text: 'People did not load', detail: j.error || '' })); return; }
      var meOwner = j.people.some(function (p) { return p.user === D.me.user && p.status === 'owner'; });
      j.people.sort(function (a, b) { return ORDER[a.status] - ORDER[b.status] || a.user.localeCompare(b.user); }).forEach(function (p) {
        var row = D.part({ type: 'listrow', icon: p.status === 'owner' || p.status === 'admin' ? 'lock' : 'people', text: p.name || p.user,
          detail: '@' + p.user + ' · joined ' + new Date(p.joined * 1000).toLocaleDateString('en', { month: 'short', year: 'numeric' }),
          value: LABEL[p.status] || p.status, tone: p.status === 'waiting' ? 'warn' : p.status === 'player' ? 'normal' : 'ok' });
        row.dataset.user = p.user;
        var acts = document.createElement('div'); acts.className = 'dev-ask';
        function key(text, style, fn) { var k = D.part({ type: 'key', text: text, style: style }); k.addEventListener('click', function () { k.disabled = true; fn().then(function (ok) { if (ok) draw(); else k.disabled = false; }); }); acts.appendChild(k); }
        if (p.status === 'waiting') key('Let in', 'go', function () { return act('invite', p.user, true); });
        if (p.status === 'player') key('Remove', 'small', function () { return act('invite', p.user, false); });
        if (meOwner && (p.status === 'player' || p.status === 'waiting')) key('Make admin', 'small', function () { return act('admin', p.user, true); });
        if (meOwner && p.status === 'admin') key('Remove admin', 'small', function () { return act('admin', p.user, false); });
        if (acts.children.length) row.querySelector('.ls-lr-text').appendChild(acts);
        slot.appendChild(row);
      });
    });
  }
  D.tabs.people.fill = function () {
    var field = D.slot('invite-user', true), go = D.slot('invite', true);
    if (go) go.addEventListener('click', function () {
      var user = (field.value || '').trim().replace(/^@/, '');
      if (!user) { D.toast('Type their username.'); return; }
      act('invite', user, true).then(function (ok) { if (ok) { field.value = ''; D.toast('@' + user + ' can come in now.'); D.slot('people'); draw(); } });
    });
    draw();
  };
})();
