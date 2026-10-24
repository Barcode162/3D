// The Ledger: the money and the things people own (master list item 3,
// docs/archive/DECISIONS.md → "The ledger"). A Durable Object with its own SQLite
// database, so every change is one transaction: a balance can never be half
// updated, counted twice or go below zero.
//
// Money is a whole number of cents of the world's currency (its name comes
// with the world's new name; for now "credits", shown like dollars).
// Earning has a daily cap per player (DAILY_CAP_CENTS) so bots and grinders
// can't flood the economy. Every change is a row in `ledger` with a unique
// `ref` (the game or purchase it came from): sending the same change twice
// does nothing the second time.
//
// Only the server changes balances: the Room (games) and, later, the mall and
// the VIP store. Players can only read their own wallet:
//   POST /bank  {"a":"me","token":"<login token>"}
//     -> {"ok":1,"user":"<id>","balance":<cents>,"earnedToday":<cents>,
//         "cap":<cents>,"items":[{"id","item","serial","tradeable","at"}]}
// Server-only calls come straight from other Durable Objects (the Room). The
// Worker marks everything it forwards from the internet with X-Public: 1, and
// those requests may only ask "me" (tests use /bank-internal, which exists
// only with DEV = "1"):
//   {"a":"credit","user","cents","reason","ref"}  earn (capped per day)
//   {"a":"debit","user","cents","reason","ref"}   spend (refused if short)
//   {"a":"grant","user","item","reason","ref","tradeable"}  give an item
//     (numbered items get the next serial: "#37")
//   {"a":"history","user","limit"}                 the last changes
// Errors: {"ok":0,"error":"..."} meant for people.

const DAY_MS = 24 * 60 * 60 * 1000;

const json = (body, status = 200) => new Response(JSON.stringify(body), {
  status,
  headers: { 'Content-Type': 'application/json', 'Cache-Control': 'no-store', 'Access-Control-Allow-Origin': '*' },
});

const fail = (error, status = 400) => json({ ok: 0, error }, status);

// Who a login token belongs to: asked from the accounts server (IONOS for
// now), remembered for 5 minutes.
const TOKEN_CACHE_MS = 5 * 60 * 1000;

export class Ledger {
  constructor(ctx, env) {
    this.ctx = ctx;
    this.env = env;
    this.sql = ctx.storage.sql;
    this.tokens = new Map();  // token -> {user, at}
    this.sql.exec(`CREATE TABLE IF NOT EXISTS wallets (
      user TEXT PRIMARY KEY, balance INTEGER NOT NULL DEFAULT 0 CHECK (balance >= 0), updated INTEGER NOT NULL)`);
    this.sql.exec(`CREATE TABLE IF NOT EXISTS ledger (
      id INTEGER PRIMARY KEY AUTOINCREMENT, user TEXT NOT NULL, delta INTEGER NOT NULL,
      reason TEXT NOT NULL, ref TEXT NOT NULL UNIQUE, at INTEGER NOT NULL)`);
    this.sql.exec(`CREATE INDEX IF NOT EXISTS ledger_user ON ledger (user, at)`);
    this.sql.exec(`CREATE TABLE IF NOT EXISTS items (
      id INTEGER PRIMARY KEY AUTOINCREMENT, user TEXT NOT NULL, item TEXT NOT NULL, serial INTEGER,
      tradeable INTEGER NOT NULL DEFAULT 0, ref TEXT NOT NULL UNIQUE, at INTEGER NOT NULL)`);
    this.sql.exec(`CREATE INDEX IF NOT EXISTS items_user ON items (user)`);
  }

  get cap() { return Number(this.env.DAILY_CAP_CENTS) || 50000; }  // $500 a day by default

  now() { return Number(this.env.TEST_NOW) || Date.now(); }

  async fetch(request) {
    if (request.method === 'OPTIONS') {
      return new Response(null, { headers: { 'Access-Control-Allow-Origin': '*', 'Access-Control-Allow-Headers': 'Content-Type' } });
    }
    if (request.method !== 'POST') return fail('POST only', 405);
    let body;
    try {
      body = JSON.parse(await request.text());
    } catch (e) {
      return fail('bad request');
    }
    const a = String(body.a || '');
    if (a === 'me') return this.me(body);
    // Everything else changes money: servers only.
    if (request.headers.get('X-Public') === '1') return fail('not allowed', 403);
    const user = String(body.user || '');
    if (!/^[A-Za-z0-9_-]{1,64}$/.test(user)) return fail('bad user');
    if (a === 'credit') return this.change(user, body, +1);
    if (a === 'debit') return this.change(user, body, -1);
    if (a === 'grant') return this.grant(user, body);
    if (a === 'history') return this.history(user, body);
    return fail('unknown action');
  }

  async userOf(token) {
    const devToken = this.env.DEV === '1' && /^dev[0-9a-f]{45}$/.test(token);  // tests
    if (!devToken && !/^[0-9a-f]{48}$/.test(token)) return null;
    const hit = this.tokens.get(token);
    if (hit && this.now() - hit.at < TOKEN_CACHE_MS) return hit.user;
    let user = null;
    if (devToken) {
      user = 'dev-' + token.slice(3, 11);  // tests: "dev" + 45 hex digits
    } else if (this.env.ACCOUNT_URL) {
      try {
        const r = await fetch(this.env.ACCOUNT_URL, {
          method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: JSON.stringify({ a: 'me', token }),
        });
        const j = await r.json();
        if (j && j.ok && j.me && j.me.id) user = String(j.me.id);
      } catch (e) {
        user = null;
      }
    }
    if (user) this.tokens.set(token, { user, at: this.now() });
    return user;
  }

  balanceOf(user) {
    const row = this.sql.exec('SELECT balance FROM wallets WHERE user = ?', user).toArray()[0];
    return row ? row.balance : 0;
  }

  earnedToday(user) {
    const since = Math.floor(this.now() / DAY_MS) * DAY_MS;
    const row = this.sql.exec('SELECT COALESCE(SUM(delta), 0) AS s FROM ledger WHERE user = ? AND delta > 0 AND at >= ?',
                              user, since).toArray()[0];
    return row ? row.s : 0;
  }

  async me(body) {
    const user = await this.userOf(String(body.token || ''));
    if (!user) return fail('log in first', 401);
    const items = this.sql.exec('SELECT id, item, serial, tradeable, at FROM items WHERE user = ? ORDER BY id', user).toArray();
    return json({ ok: 1, user, balance: this.balanceOf(user), earnedToday: this.earnedToday(user), cap: this.cap, items });
  }

  // One change of money, in one transaction. `sign` +1 earns, -1 spends.
  change(user, body, sign) {
    const cents = Math.floor(Number(body.cents));
    const reason = String(body.reason || '').slice(0, 60);
    const ref = String(body.ref || '').slice(0, 120);
    if (!(cents > 0) || cents > 100000000) return fail('bad amount');
    if (!reason || !ref) return fail('reason and ref needed');
    const now = this.now();
    let result;
    this.ctx.storage.transactionSync(() => {
      if (this.sql.exec('SELECT 1 FROM ledger WHERE ref = ?', ref).toArray().length) {
        result = { ok: 1, duplicate: 1, balance: this.balanceOf(user) };
        return;
      }
      let delta = sign * cents;
      if (sign > 0) {
        const room = Math.max(0, this.cap - this.earnedToday(user));
        delta = Math.min(delta, room);
        if (delta === 0) {
          result = { ok: 1, capped: 1, credited: 0, balance: this.balanceOf(user) };
          return;
        }
      } else if (this.balanceOf(user) < cents) {
        result = { ok: 0, error: 'not enough credits' };
        return;
      }
      // (An update or an insert, not an upsert: the row's own CHECK would
      // reject a negative delta before the conflict turned it into an update.)
      if (this.sql.exec('SELECT 1 FROM wallets WHERE user = ?', user).toArray().length) {
        this.sql.exec('UPDATE wallets SET balance = balance + ?, updated = ? WHERE user = ?', delta, now, user);
      } else {
        this.sql.exec('INSERT INTO wallets (user, balance, updated) VALUES (?, ?, ?)', user, delta, now);
      }
      this.sql.exec('INSERT INTO ledger (user, delta, reason, ref, at) VALUES (?, ?, ?, ?, ?)', user, delta, reason, ref, now);
      result = { ok: 1, credited: sign > 0 ? delta : undefined, capped: sign > 0 && delta < cents ? 1 : undefined,
                 balance: this.balanceOf(user) };
    });
    return json(result, result.ok ? 200 : 409);
  }

  grant(user, body) {
    const item = String(body.item || '');
    const ref = String(body.ref || '').slice(0, 120);
    if (!/^[a-z0-9_.-]{1,64}$/.test(item) || !ref) return fail('bad item');
    const numbered = !!body.numbered;
    const tradeable = body.tradeable ? 1 : 0;
    let result;
    this.ctx.storage.transactionSync(() => {
      const had = this.sql.exec('SELECT id, serial FROM items WHERE ref = ?', ref).toArray()[0];
      if (had) {
        result = { ok: 1, duplicate: 1, id: had.id, serial: had.serial };
        return;
      }
      let serial = null;
      if (numbered) {
        const row = this.sql.exec('SELECT COALESCE(MAX(serial), 0) AS m FROM items WHERE item = ?', item).toArray()[0];
        serial = row.m + 1;
      }
      this.sql.exec('INSERT INTO items (user, item, serial, tradeable, ref, at) VALUES (?, ?, ?, ?, ?, ?)',
                    user, item, serial, tradeable, ref, this.now());
      const id = this.sql.exec('SELECT last_insert_rowid() AS id').toArray()[0].id;
      result = { ok: 1, id, serial };
    });
    return json(result);
  }

  history(user, body) {
    const limit = Math.max(1, Math.min(100, Math.floor(Number(body.limit) || 20)));
    const rows = this.sql.exec('SELECT delta, reason, ref, at FROM ledger WHERE user = ? ORDER BY id DESC LIMIT ?',
                               user, limit).toArray();
    return json({ ok: 1, balance: this.balanceOf(user), rows });
  }
}
