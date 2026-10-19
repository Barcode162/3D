// Accounts: sign up, log in, your profile (picture, display name, bio,
// favorite drink and song, card color) and person cards. The forms are
// plain HTML over the game (the phone's keyboard and photo picker only
// work with real form fields); the server is server/account.php next to
// the multiplayer relay. The login token is kept in localStorage.
//
// C++ uses: bar3dAccount.showCard(accountId, name) when you tap someone,
// and the avatar URL (avatarUrl) to draw pictures over heads.
window.bar3dAccount = (function () {
  'use strict';
  // Extra profile details (server/account.php DETAIL_FIELDS): key, label, placeholder, max length.
  var DETAILS = [
    ['status', 'Status', 'e.g. here for the piano', 60], ['pronouns', 'Pronouns', 'e.g. he/him', 20],
    ['location', 'Where from', 'e.g. Austin, TX', 40], ['birthday', 'Birthday', 'e.g. March 3', 20],
    ['job', 'What I do', 'e.g. student', 40], ['game', 'Favorite game', 'e.g. darts', 40],
    ['genre', 'Music I love', 'e.g. jazz, house', 40], ['instrument', 'I play', 'e.g. piano', 40],
    ['cocktail', 'Signature cocktail', 'e.g. Espresso Martini', 40], ['languages', 'Languages', 'e.g. English, Spanish', 60],
    ['social', 'Find me at', 'e.g. @josh on Instagram', 60], ['lookingFor', 'Looking for', 'e.g. a darts rival', 60],
    ['funFact', 'Fun fact', 'Something surprising about you', 120],
  ];
  // Badge names; each one's picture is badges/<id>.png (3D-rendered,
  // tools/assets/badges.py).
  var BADGE_NAMES = { owner: 'Owner', bartender: 'Bartender', dj: 'DJ', vip: 'VIP', staff: 'Staff', race: 'Race winner',
                      darts: 'Darts ace', mania: '4K Mania star' };
  function badgeImg(b) { return el('img', { src: 'badges/' + b + '.png', alt: BADGE_NAMES[b] || b, draggable: 'false' }); }
  var COLORS = ['#ff5ab4', '#ffb35a', '#5ad1ff', '#7dff8a', '#b38bff', '#ff6b5a', '#f5d9a8', '#5affd2'];
  var server = '';     // account.php URL ('' = no accounts on this page)
  var token = null, me = null;
  var root = null, panel = null;

  function store(key, value) {
    try {
      if (value === undefined) return localStorage.getItem(key);
      if (value === null) localStorage.removeItem(key); else localStorage.setItem(key, value);
    } catch (e) {}
    return null;
  }
  function info(text) { if (window.Module && Module.print) Module.print('[info] ' + text); }
  function el(tag, attrs, children) {
    var e = document.createElement(tag);
    for (var k in attrs || {}) {
      if (k === 'style') e.style.cssText = attrs[k];
      else if (k === 'text') e.textContent = attrs[k];
      else if (k.slice(0, 2) === 'on') e.addEventListener(k.slice(2), attrs[k]);
      else e.setAttribute(k, attrs[k]);
    }
    (children || []).forEach(function (c) { if (c) e.appendChild(c); });
    return e;
  }
  function call(body) {
    if (!server) return Promise.reject(new Error('Accounts are not available here.'));
    return fetch(server, { method: 'POST', body: JSON.stringify(body), headers: { 'Content-Type': 'text/plain' } })
      .then(function (r) { return r.json(); })
      .then(function (reply) {
        if (!reply || !reply.ok) throw new Error((reply && reply.error) || 'Something went wrong.');
        return reply;
      });
  }
  function avatarUrl(id, rev) { return rev ? server + '?avatar=' + id + '&v=' + rev : ''; }

  // ---- Styles (once) ----
  function addStyles() {
    var css = [
      '#acctRoot{position:fixed;inset:0;z-index:8;display:none;align-items:center;justify-content:center;',
      'background:#0b0710e0;padding:16px;',  // no blur: blurring the live game costs every frame
      'font:15px/1.4 -apple-system,system-ui,sans-serif;color:#f5ecf8;}',
      '#acctRoot.open{display:flex;}',
      '.acctPanel{width:min(92vw,380px);max-height:88vh;overflow:auto;background:#1a1220;border-radius:20px;',
      'box-shadow:0 10px 40px #000a;padding:22px;}',
      '.acctPanel h2{margin:0 0 14px;font-size:22px;}',
      '.acctPanel label{display:block;font-size:13px;color:#cbb8d6;margin:12px 0 4px;}',
      '.acctPanel input,.acctPanel textarea{width:100%;box-sizing:border-box;font:16px -apple-system,system-ui;',
      'color:#fff;background:#2a1f33;border:1px solid #4a3a55;border-radius:10px;padding:10px;}',
      '.acctPanel textarea{min-height:80px;resize:vertical;}',
      '.acctBtn{display:block;width:100%;margin-top:14px;padding:12px;border:0;border-radius:12px;',
      'font:600 16px -apple-system,system-ui;color:#1a1220;background:#ff5ab4;}',
      '.acctBtn.ghost{background:#2a1f33;color:#f5ecf8;}',
      '.acctErr{color:#ff7a7a;min-height:20px;margin-top:10px;font-size:14px;}',
      '.acctSmall{font-size:13px;color:#cbb8d6;text-align:center;margin-top:12px;}',
      '.acctSmall a{color:#ffb35a;}',
      '.acctSwatches{display:flex;gap:8px;flex-wrap:wrap;}',
      '.acctSwatch{width:32px;height:32px;border-radius:50%;border:3px solid transparent;}',
      '.acctSwatch.on{border-color:#fff;}',
      '.acctSlots{margin:12px 16px 0;}',
      '.acctSlotTitle{font:700 10px system-ui;letter-spacing:2px;color:#b9a7c8;text-align:center;margin-bottom:6px;}',
      '.acctSlotRow{display:grid;grid-template-columns:repeat(6,1fr);gap:6px;}',
      '.acctSlot{aspect-ratio:1;border-radius:12px;border:2px dashed #4a3a55;display:flex;flex-direction:column;align-items:center;justify-content:center;background:rgba(255,255,255,.03);}',
      '.acctSlot.on{border:2px solid #ff5ab4;background:linear-gradient(160deg,#3a2347,#1f1428);box-shadow:0 0 10px rgba(255,90,180,.35);}',
      '.acctSlot.mine{cursor:pointer;}',
      '.acctSlot .ic{font-size:20px;line-height:1;color:#7a6a88;}',
      '.acctSlot .lb{font:600 8px system-ui;color:#e8dcef;margin-top:3px;text-align:center;line-height:1.1;}',
      '.acctSlotHint{font:12px system-ui;color:#9a8aa8;text-align:center;margin-top:6px;}',
      '.acctPic{width:96px;height:96px;border-radius:50%;object-fit:cover;background:#2a1f33;display:block;',
      'margin:0 auto;border:3px solid #fff3;}',
      '.acctCard{width:min(88vw,340px);border-radius:22px;overflow:hidden;background:#1a1220;',
      'box-shadow:0 10px 40px #000a;text-align:center;}',
      '.acctCardTop{height:86px;}',
      '.acctCard .acctPic{width:110px;height:110px;margin:-58px auto 0;border:4px solid #1a1220;}',
      '.acctCard .nm{font-size:22px;font-weight:700;margin-top:8px;}',
      '.acctCard .un{color:#cbb8d6;font-size:14px;}',
      '.acctCard .bio{margin:12px 18px 0;white-space:pre-wrap;}',
      '.acctCard .facts{margin:14px 18px 0;text-align:left;font-size:14px;color:#e6d6ee;}',
      '.acctCard .facts div{margin:4px 0;}',
      '.acctCard .btns{padding:6px 18px 18px;}',
      '#acctMe{pointer-events:auto;font:inherit;color:inherit;background:#0008;border:1px solid #f5d9a855;',
      'border-radius:6px;padding:4px 8px;margin-right:6px;}',
      '#acctLoading{margin-top:6px;text-align:center;font:14px -apple-system,system-ui;color:#f5d9a8;}',
      '#acctLoading button{font:600 15px -apple-system,system-ui;margin:6px 4px 0;padding:9px 16px;border-radius:18px;',
      'border:1px solid #f5d9a888;background:#0006;color:#f5d9a8;}',
      '#acctLoading img{width:34px;height:34px;border-radius:50%;object-fit:cover;vertical-align:middle;margin-right:8px;}',
    ].join('');
    document.head.appendChild(el('style', { text: css }));
  }

  function open(content) {
    if (window.bar3dModal) bar3dModal.open('account', close);
    root.innerHTML = '';
    root.appendChild(content);
    root.classList.add('open');
  }
  function close() {
    root.classList.remove('open');
    root.innerHTML = '';
    if (window.bar3dModal) bar3dModal.closed('account');
  }

  function picture(rev, id, letter, color) {
    var img = el('img', { class: 'acctPic', alt: '' });
    if (rev) img.src = avatarUrl(id, rev);
    else {
      // No picture yet: a colored circle with the first letter.
      var c = document.createElement('canvas');
      c.width = c.height = 128;
      var g = c.getContext('2d');
      g.fillStyle = color || '#ff5ab4';
      g.fillRect(0, 0, 128, 128);
      g.fillStyle = '#1a1220';
      g.font = 'bold 64px -apple-system,system-ui,sans-serif';
      g.textAlign = 'center';
      g.textBaseline = 'middle';
      g.fillText((letter || '?').charAt(0).toUpperCase(), 64, 68);
      img.src = c.toDataURL();
    }
    return img;
  }

  // ---- Screens ----
  function loginScreen(signup) {
    var user = el('input', { autocapitalize: 'none', autocomplete: 'username', placeholder: 'username' });
    var name = el('input', { placeholder: 'how your name shows in the bar', maxlength: '20' });
    var pass = el('input', { type: 'password', autocomplete: signup ? 'new-password' : 'current-password',
                             placeholder: 'password (6+ characters)' });
    var err = el('div', { class: 'acctErr' });
    var go = el('button', { class: 'acctBtn', text: signup ? 'Create account' : 'Log in' });
    go.onclick = function () {
      err.textContent = '';
      go.disabled = true;
      call({ a: signup ? 'signup' : 'login', user: user.value, pass: pass.value, name: name.value })
        .then(function (r) {
          setSession(r.token, r.me);
          info('Account: logged in as ' + r.me.name + '.');
          if (signup) editScreen(true); else close();
        })
        .catch(function (e) { err.textContent = e.message; go.disabled = false; });
    };
    var children = [el('h2', { text: signup ? 'Sign up' : 'Log in' }), el('label', { text: 'Username' }), user];
    if (signup) children.push(el('label', { text: 'Display name' }), name);
    children.push(el('label', { text: 'Password' }), pass, err, go,
                  el('button', { class: 'acctBtn ghost', text: 'Cancel', onclick: close }),
                  el('div', { class: 'acctSmall' }, [el('a', {
                    href: '#', text: signup ? 'I already have an account' : 'New here? Sign up',
                    onclick: function (e) { e.preventDefault(); loginScreen(!signup); } })]));
    open(el('div', { class: 'acctPanel' }, children));
  }

  function editScreen(fresh) {
    var color = me.color;
    var pic = picture(me.avatar, me.id, me.name, COLORS[me.color]);
    var file = el('input', { type: 'file', accept: 'image/*', id: 'acctFile', style: 'display:none' });
    var picErr = el('div', { class: 'acctErr' });
    file.onchange = function () {
      var f = file.files[0];
      if (!f) return;
      picErr.textContent = 'Uploading...';
      // Crop the middle square, shrink to 256 px, JPEG.
      createImageBitmap(f).then(function (bmp) {
        var s = Math.min(bmp.width, bmp.height), c = document.createElement('canvas');
        c.width = c.height = 256;
        c.getContext('2d').drawImage(bmp, (bmp.width - s) / 2, (bmp.height - s) / 2, s, s, 0, 0, 256, 256);
        return call({ a: 'avatar', token: token, image: c.toDataURL('image/jpeg', 0.85) });
      }).then(function (r) {
        setSession(token, r.me);
        pic.src = avatarUrl(me.id, me.avatar);
        picErr.textContent = '';
      }).catch(function (e) { picErr.textContent = e.message || 'Could not use that picture.'; });
    };
    var name = el('input', { maxlength: '20', value: me.name });
    var bio = el('textarea', { maxlength: '240', placeholder: 'A few words about you' });
    bio.value = me.bio;
    var drink = el('input', { maxlength: '40', value: me.drink, placeholder: 'e.g. Old Fashioned' });
    var song = el('input', { maxlength: '40', value: me.song, placeholder: 'e.g. Bossa Antigua' });
    var detailInputs = {};
    var details = me.details || {};
    var detailEls = [];
    DETAILS.forEach(function (d) {
      var input = d[3] > 60 ? el('textarea', { maxlength: String(d[3]), placeholder: d[2] })
                            : el('input', { maxlength: String(d[3]), placeholder: d[2] });
      input.value = details[d[0]] || '';
      if (d[3] > 60) input.style.minHeight = '50px';
      detailInputs[d[0]] = input;
      detailEls.push(el('label', { text: d[1] }), input);
    });
    var swatches = el('div', { class: 'acctSwatches' });
    COLORS.forEach(function (c, i) {
      var s = el('div', { class: 'acctSwatch' + (i === color ? ' on' : ''), style: 'background:' + c });
      s.onclick = function () {
        color = i;
        Array.prototype.forEach.call(swatches.children, function (x, j) { x.classList.toggle('on', j === i); });
      };
      swatches.appendChild(s);
    });
    var err = el('div', { class: 'acctErr' });
    var save = el('button', { class: 'acctBtn', text: 'Save' });
    save.onclick = function () {
      save.disabled = true;
      var det = {};
      for (var k in detailInputs) det[k] = detailInputs[k].value;
      call({ a: 'update', token: token, name: name.value, bio: bio.value, drink: drink.value, song: song.value,
             color: color, details: det })
        .then(function (r) { setSession(token, r.me); cardScreen(me, true); })
        .catch(function (e) { err.textContent = e.message; save.disabled = false; });
    };
    open(el('div', { class: 'acctPanel' }, [
      el('h2', { text: fresh ? 'Welcome! Make your card' : 'Edit your card' }),
      pic,
      el('label', { for: 'acctFile', class: 'acctBtn ghost', style: 'text-align:center;margin-top:10px;',
                    text: 'Choose profile picture' }),
      file, picErr,
      el('label', { text: 'Display name' }), name,
      el('label', { text: 'Bio' }), bio,
      el('label', { text: 'Favorite drink' }), drink,
      el('label', { text: 'Favorite song' }), song,
      el('label', { text: 'Card color' }), swatches,
      el('h2', { text: 'More about you', style: 'margin-top:18px;font-size:18px;' }),
    ].concat(detailEls).concat([
      err, save,
      el('button', { class: 'acctBtn ghost', text: 'Close', onclick: close }),
    ])));
  }

  // A person's card. `own` adds Edit / Log out.
  function cardScreen(p, own) {
    var facts = el('div', { class: 'facts' });
    var fact = function (k, v) { facts.appendChild(el('div', {}, [el('span', { class: 'k', text: k }), el('span', { text: v })])); };
    if (p.drink) fact('Favorite drink', p.drink);
    if (p.song) fact('Favorite song', p.song);
    if (p.joined) fact('Regular since', p.joined);
    var d = p.details || {};
    DETAILS.forEach(function (x) { if (d[x[0]]) fact(x[1], d[x[0]]); });
    var badges = badgeSlots(p, own);
    var btns = el('div', { class: 'btns' });
    if (own) {
      btns.appendChild(el('button', { class: 'acctBtn', text: 'Edit card', onclick: function () { editScreen(false); } }));
      if ((p.badges || []).indexOf('owner') >= 0) {
        btns.appendChild(el('button', { class: 'acctBtn ghost', text: 'Give badges', onclick: badgeScreen }));
      }
      btns.appendChild(el('button', { class: 'acctBtn ghost', text: 'Log out', onclick: logout }));
    }
    btns.appendChild(el('button', { class: 'acctBtn ghost', text: 'Close', onclick: close }));
    var color = COLORS[p.color || 0];
    open(el('div', { class: 'acctCard' }, [
      el('div', { class: 'acctCardTop', style: 'background:linear-gradient(135deg,' + color + ',#1a1220)' }),
      picture(p.avatar, p.id, p.name, color),
      el('div', { class: 'nm', text: p.name }),
      el('div', { class: 'un', text: p.user ? '@' + p.user : 'Guest' }),
      p.bio ? el('div', { class: 'bio', text: p.bio }) : null,
      badges, facts, btns,
    ]));
  }

  // The card's badge slots: SLOT_COUNT places for badges, like a showcase.
  // Filled ones show the badge; on your own card a tap picks which of your
  // badges goes there (or empties it). Badges not in a slot are listed small.
  var SLOT_COUNT = 6;
  function slotsOf(p) {
    var held = p.badges || [];
    var s = (p.slots || []).slice(0, SLOT_COUNT);
    while (s.length < SLOT_COUNT) s.push('');
    // Nothing chosen yet: the badges you have, in order.
    if (s.every(function (b) { return !b; })) held.slice(0, SLOT_COUNT).forEach(function (b, i) { s[i] = b; });
    return s.map(function (b) { return held.indexOf(b) >= 0 ? b : ''; });
  }
  function badgeSlots(p, own) {
    var wrap = el('div', { class: 'acctSlots' });
    var slots = slotsOf(p);
    var row = el('div', { class: 'acctSlotRow' });
    slots.forEach(function (b, i) {
      var cell = el('div', { class: 'acctSlot' + (b ? ' on' : '') + (own ? ' mine' : ''), title: b ? (BADGE_NAMES[b] || b) : 'Empty slot' },
        b ? [badgeImg(b)] : [el('div', { class: 'ic', text: own ? '+' : '' })]);
      if (own && p.user) cell.onclick = function () { pickSlot(i, slots); };
      row.appendChild(cell);
    });
    wrap.appendChild(el('div', { class: 'acctSlotTitle', text: 'BADGES' }));
    wrap.appendChild(row);
    if (!p.user) wrap.appendChild(el('div', { class: 'acctSlotHint', text: own ? 'Make an account to collect badges.' : '' }));
    else if (own && !(p.badges || []).length) wrap.appendChild(el('div', { class: 'acctSlotHint', text: 'Badges you earn go here.' }));
    var extra = (p.badges || []).filter(function (b) { return slots.indexOf(b) < 0; });
    if (extra.length) wrap.appendChild(el('div', { class: 'acctSlotHint', text: 'Also: ' + extra.map(function (b) { return BADGE_NAMES[b] || b; }).join('  ') }));
    return wrap;
  }
  function pickSlot(i, slots) {
    var choices = [''].concat((me.badges || []));
    open(el('div', { class: 'acctPanel' }, [el('h2', { text: 'Badge slot ' + (i + 1) })].concat(choices.map(function (b) {
      var btn = el('button', { class: 'acctBtn acctBadgeChoice' + (b === slots[i] ? '' : ' ghost'), onclick: function () {
        var next = slots.slice();
        var at = next.indexOf(b);
        if (b && at >= 0) next[at] = '';  // a badge sits in one slot only
        next[i] = b;
        call({ a: 'update', token: token, slots: next })
          .then(function (r) { setSession(token, r.me); cardScreen(me, true); })
          .catch(function () { cardScreen(me, true); });
      } }, b ? [badgeImg(b), el('span', { text: BADGE_NAMES[b] || b })] : [el('span', { text: 'Empty' })]);
      return btn;
    })).concat([el('button', { class: 'acctBtn ghost', text: 'Back', onclick: function () { cardScreen(me, true); } })])));
  }

  // Owners: give or take badges (bartender...) by username.
  function badgeScreen() {
    var user = el('input', { autocapitalize: 'none', placeholder: 'their username' });
    var badge = el('select', { style: 'width:100%;font:16px -apple-system,system-ui;padding:10px;border-radius:10px;background:#2a1f33;color:#fff;border:1px solid #4a3a55;' },
                   ['bartender', 'dj', 'vip', 'staff'].map(function (b) { return el('option', { value: b, text: BADGE_NAMES[b] }); }));
    var msg = el('div', { class: 'acctErr' });
    function go(on) {
      msg.textContent = '';
      call({ a: 'badge', token: token, user: user.value, badge: badge.value, on: on ? 1 : 0 })
        .then(function (r) {
          if (r.me) setSession(token, r.me);
          msg.style.color = '#7dff8a';
          msg.textContent = (on ? 'Gave ' : 'Took ') + badge.value + (on ? ' to ' : ' from ') + r.profile.name + '.';
        })
        .catch(function (e) { msg.style.color = ''; msg.textContent = e.message; });
    }
    open(el('div', { class: 'acctPanel' }, [
      el('h2', { text: 'Give badges' }),
      el('div', { class: 'acctSmall', style: 'text-align:left', text: 'Bartenders can work the bar and make drinks. You can give yourself one too.' }),
      el('label', { text: 'Username' }), user, el('label', { text: 'Badge' }), badge, msg,
      el('button', { class: 'acctBtn', text: 'Give badge', onclick: function () { go(true); } }),
      el('button', { class: 'acctBtn ghost', text: 'Take badge away', onclick: function () { go(false); } }),
      el('button', { class: 'acctBtn ghost', text: 'Back', onclick: function () { cardScreen(me, true); } }),
    ]));
  }

  function logout() {
    if (token) call({ a: 'logout', token: token }).catch(function () {});
    setSession(null, null);
    close();
    info('Account: logged out.');
  }

  function setSession(t, profile) {
    token = t;
    me = profile;
    store('bar3dToken', t);
    store('bar3dProfile', profile ? JSON.stringify(profile) : null);
    refreshBadges();
  }

  // The account line on the loading screen and the HUD "me" button.
  function refreshBadges() {
    var meBtn = document.getElementById('acctMe');
    if (meBtn) meBtn.textContent = me ? me.name : 'log in';
    var box = document.getElementById('acctLoading');
    if (!box) return;
    box.innerHTML = '';
    if (!server) return;
    if (me) {
      var line = el('div', {}, [picture(me.avatar, me.id, me.name, COLORS[me.color]),
                                el('span', { text: 'Signed in as ' + me.name })]);
      line.firstChild.className = '';
      box.appendChild(line);
      box.appendChild(el('button', { text: 'My card', onclick: function () { cardScreen(me, true); } }));
    } else {
      box.appendChild(el('div', { text: 'Playing as a guest' }));
      box.appendChild(el('button', { text: 'Log in', onclick: function () { loginScreen(false); } }));
      box.appendChild(el('button', { text: 'Sign up', onclick: function () { loginScreen(true); } }));
    }
  }

  function init() {
    var relay = window.bar3dNet && bar3dNet.serverUrl ? bar3dNet.serverUrl() : '';
    server = relay ? relay.replace(/relay\.php(\?.*)?$/, 'account.php') : '';
    if (server && !bar3dNet.tokenOk(server)) server = '';  // never send a password elsewhere
    addStyles();
    root = el('div', { id: 'acctRoot' });
    root.addEventListener('click', function (e) { if (e.target === root) close(); });
    document.body.appendChild(root);
    var loading = document.getElementById('loading');
    if (loading) loading.appendChild(el('div', { id: 'acctLoading' }));
    var hudRight = document.getElementById('logToggle');
    if (hudRight && server) {
      hudRight.parentNode.insertBefore(el('button', { id: 'acctMe', onclick: function () {
        if (me) cardScreen(me, true); else loginScreen(false);
      } }), hudRight);
    }
    token = store('bar3dToken');
    try { me = JSON.parse(store('bar3dProfile') || 'null'); } catch (e) { me = null; }
    refreshBadges();
    if (token) {  // check the saved login is still good, refresh the profile
      call({ a: 'me', token: token })
        .then(function (r) { setSession(token, r.me); info('Account: signed in as ' + r.me.name + '.'); })
        .catch(function (e) { if (/log in/i.test(e.message)) setSession(null, null); });
    }
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init); else init();

  return {
    token: function () { return token; },
    name: function () { return me ? me.name : ''; },
    loggedIn: function () { return !!me; },
    hasBadge: function (b) { return !!me && !!me.badges && (me.badges.indexOf(b) >= 0 || me.badges.indexOf('owner') >= 0); },
    // The menu's Profile button: your card (or log in).
    openProfile: function () { if (!server) { return; } if (me) cardScreen(me, true); else loginScreen(false); },
    avatarUrl: avatarUrl,
    // Shows a person's card (C++ calls this when you tap someone).
    showCard: function (id, name) {
      if (!id) { cardScreen({ name: name, user: '', bio: 'Just visiting (no account yet).', color: 6 }, false); return; }
      if (me && id === me.id) { cardScreen(me, true); return; }
      call({ a: 'profile', id: id }).then(function (r) { cardScreen(r.profile, false); })
        .catch(function () { cardScreen({ name: name, user: '', color: 6 }, false); });
    },
    isOpen: function () { return !!root && root.classList.contains('open'); },
  };
})();
