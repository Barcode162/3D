var __defProp = Object.defineProperty;
var __name = (target, value) => __defProp(target, "name", { value, configurable: true });

// worker.js
var MAX_PLAYERS = 16;
var MAX_VALUES = 20;
var MAX_KEYS = 96;
var MAX_MSG = 12e3;
var WORLD_KEEP_MS = 30 * 60 * 1e3;
var cors = {
  "Access-Control-Allow-Origin": "*",
  "Cache-Control": "no-store",
  "Content-Type": "application/json"
};
var worker_default = {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === "/health" || url.pathname === "/") {
      return new Response(JSON.stringify({ ok: 1, server: "bar3d-cloud" }), { headers: cors });
    }
    const m = /^\/room\/([a-z0-9-]{1,32})$/.exec(url.pathname);
    if (!m) return new Response("not found", { status: 404, headers: cors });
    if (request.headers.get("Upgrade") !== "websocket") {
      return new Response("expected a websocket", { status: 426, headers: cors });
    }
    const room = env.ROOMS.get(env.ROOMS.idFromName(m[1]));
    return room.fetch(request);
  }
};
function cleanNumbers(list) {
  if (!Array.isArray(list) || list.length > MAX_VALUES) return null;
  const out = [];
  for (const v of list) {
    if (typeof v !== "number" || !isFinite(v)) return null;
    out.push(v);
  }
  return out;
}
__name(cleanNumbers, "cleanNumbers");
var Room = class {
  static {
    __name(this, "Room");
  }
  constructor(ctx, env) {
    this.ctx = ctx;
    this.env = env;
    this.players = /* @__PURE__ */ new Map();
    this.world = null;
    this.lastActive = 0;
    for (const ws of this.ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (a && a.id) this.players.set(a.id, { name: a.name, t: 0, s: [], acct: a.acct || "", av: a.av || 0 });
    }
  }
  async loadWorld() {
    if (this.world) return;
    const saved = await this.ctx.storage.get("world");
    const active = await this.ctx.storage.get("active") || 0;
    this.world = new Map(saved && Date.now() - active < WORLD_KEEP_MS ? saved : []);
  }
  now() {
    return Date.now() / 1e3;
  }
  socketsExcept(id) {
    return this.ctx.getWebSockets().filter((ws) => {
      const a = ws.deserializeAttachment();
      return a && a.id !== id;
    });
  }
  socketOf(id) {
    return this.ctx.getWebSockets().find((ws) => {
      const a = ws.deserializeAttachment();
      return a && a.id === id;
    });
  }
  send(ws, data) {
    try {
      ws.send(typeof data === "string" ? data : JSON.stringify(data));
    } catch (e) {
    }
  }
  broadcast(fromId, data) {
    const text = JSON.stringify(data);
    for (const ws of this.socketsExcept(fromId)) this.send(ws, text);
  }
  async fetch(request) {
    const url = new URL(request.url);
    const id = url.searchParams.get("id") || "";
    if (!/^[a-z0-9]{4,16}$/.test(id)) return new Response("bad id", { status: 400 });
    if (!this.players.has(id) && this.players.size >= MAX_PLAYERS) return new Response("room full", { status: 503 });
    const old = this.socketOf(id);
    if (old) {
      try {
        old.close(4e3, "replaced");
      } catch (e) {
      }
    }
    const pair = new WebSocketPair();
    this.ctx.acceptWebSocket(pair[1]);
    pair[1].serializeAttachment({ id, name: "Guest", acct: "", av: 0, hello: false });
    return new Response(null, { status: 101, webSocket: pair[0] });
  }
  async webSocketMessage(ws, raw) {
    if (typeof raw !== "string" || raw.length > 64e3) return;
    let msg;
    try {
      msg = JSON.parse(raw);
    } catch (e) {
      return;
    }
    if (!msg || typeof msg !== "object") return;
    const a = ws.deserializeAttachment();
    if (!a) return;
    const id = a.id;
    if ("ping" in msg) this.send(ws, { pong: msg.ping, now: this.now() });
    if (msg.hello && !a.hello) {
      a.hello = true;
      a.name = typeof msg.name === "string" ? msg.name.replace(/[^\x20-\x7E]/g, "").slice(0, 20) || "Guest" : "Guest";
      if (typeof msg.tok === "string" && /^[0-9a-f]{48}$/.test(msg.tok)) {
        const acct = await this.checkToken(msg.tok);
        if (acct) {
          a.acct = acct.id;
          a.av = acct.av;
          a.name = acct.name;
        }
      }
      ws.serializeAttachment(a);
      const p = this.players.get(id) || { t: 0, s: [] };
      Object.assign(p, { name: a.name, acct: a.acct, av: a.av });
      this.players.set(id, p);
      await this.loadWorld();
      const players = [];
      for (const [pid, q] of this.players) {
        if (pid !== id && q.s.length) players.push({ id: pid, name: q.name, t: q.t, s: q.s, acct: q.acct, av: q.av });
      }
      const world = [];
      for (const [key, e] of this.world) if (e[0] !== id) world.push([key, e[0], e[1]]);
      this.send(ws, { welcome: 1, now: this.now(), players, world, name: a.name });
    }
    if (!a.hello) return;
    if (Array.isArray(msg.s)) {
      const s = cleanNumbers(msg.s);
      if (s) {
        const p = this.players.get(id);
        p.t = typeof msg.t === "number" ? msg.t : 0;
        p.s = s;
        this.broadcast(id, { p: { id, name: p.name, t: p.t, s, acct: p.acct, av: p.av } });
      }
    }
    if (msg.w && typeof msg.w === "object") {
      await this.loadWorld();
      const changes = [];
      for (const [key, values] of Object.entries(msg.w)) {
        if (!/^[a-z0-9]{1,8}$/.test(key)) continue;
        const v = cleanNumbers(values);
        if (!v) continue;
        if (!this.world.has(key) && this.world.size >= MAX_KEYS) continue;
        this.world.set(key, [id, v]);
        changes.push([key, id, v]);
      }
      if (changes.length) {
        this.broadcast(id, { w: changes });
        this.lastActive = Date.now();
        this.ctx.storage.put({ world: [...this.world], active: this.lastActive });
      }
    }
    if (Array.isArray(msg.m)) {
      for (const m of msg.m.slice(0, 40)) {
        if (!Array.isArray(m) || m.length !== 2 || typeof m[0] !== "string" || typeof m[1] !== "string") continue;
        if (m[1].length > MAX_MSG) continue;
        const text = JSON.stringify({ m: [id, m[1]] });
        if (m[0] === "*") {
          for (const other of this.socketsExcept(id)) this.send(other, text);
        } else {
          const target = this.socketOf(m[0]);
          if (target) this.send(target, text);
        }
      }
    }
  }
  async webSocketClose(ws) {
    this.gone(ws);
  }
  async webSocketError(ws) {
    this.gone(ws);
  }
  gone(ws) {
    const a = ws.deserializeAttachment();
    if (!a) return;
    const still = this.ctx.getWebSockets().some((o) => o !== ws && (o.deserializeAttachment() || {}).id === a.id);
    if (still) return;
    this.players.delete(a.id);
    this.broadcast(a.id, { left: a.id });
    if (this.world) this.ctx.storage.put("active", Date.now());
  }
  // Asks the accounts server (IONOS) whose login this is.
  async checkToken(tok) {
    try {
      const r = await fetch(this.env.ACCOUNT_URL, {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: JSON.stringify({ a: "me", token: tok })
      });
      const j = await r.json();
      if (j && j.ok && j.me) return { id: String(j.me.id), name: String(j.me.name).slice(0, 40), av: j.me.avatar | 0 };
    } catch (e) {
    }
    return null;
  }
};

// node_modules/wrangler/templates/middleware/middleware-ensure-req-body-drained.ts
var drainBody = /* @__PURE__ */ __name(async (request, env, _ctx, middlewareCtx) => {
  try {
    return await middlewareCtx.next(request, env);
  } finally {
    try {
      if (request.body !== null && !request.bodyUsed) {
        const reader = request.body.getReader();
        while (!(await reader.read()).done) {
        }
      }
    } catch (e) {
      console.error("Failed to drain the unused request body.", e);
    }
  }
}, "drainBody");
var middleware_ensure_req_body_drained_default = drainBody;

// node_modules/wrangler/templates/middleware/middleware-miniflare3-json-error.ts
function reduceError(e) {
  return {
    name: e?.name,
    message: e?.message ?? String(e),
    stack: e?.stack,
    cause: e?.cause === void 0 ? void 0 : reduceError(e.cause)
  };
}
__name(reduceError, "reduceError");
var jsonError = /* @__PURE__ */ __name(async (request, env, _ctx, middlewareCtx) => {
  try {
    return await middlewareCtx.next(request, env);
  } catch (e) {
    const error = reduceError(e);
    return Response.json(error, {
      status: 500,
      headers: { "MF-Experimental-Error-Stack": "true" }
    });
  }
}, "jsonError");
var middleware_miniflare3_json_error_default = jsonError;

// .wrangler/tmp/bundle-ecgA40/middleware-insertion-facade.js
var __INTERNAL_WRANGLER_MIDDLEWARE__ = [
  middleware_ensure_req_body_drained_default,
  middleware_miniflare3_json_error_default
];
var middleware_insertion_facade_default = worker_default;

// node_modules/wrangler/templates/middleware/common.ts
var __facade_middleware__ = [];
function __facade_register__(...args) {
  __facade_middleware__.push(...args.flat());
}
__name(__facade_register__, "__facade_register__");
function __facade_invokeChain__(request, env, ctx, dispatch, middlewareChain) {
  const [head, ...tail] = middlewareChain;
  const middlewareCtx = {
    dispatch,
    next(newRequest, newEnv) {
      return __facade_invokeChain__(newRequest, newEnv, ctx, dispatch, tail);
    }
  };
  return head(request, env, ctx, middlewareCtx);
}
__name(__facade_invokeChain__, "__facade_invokeChain__");
function __facade_invoke__(request, env, ctx, dispatch, finalMiddleware) {
  return __facade_invokeChain__(request, env, ctx, dispatch, [
    ...__facade_middleware__,
    finalMiddleware
  ]);
}
__name(__facade_invoke__, "__facade_invoke__");

// .wrangler/tmp/bundle-ecgA40/middleware-loader.entry.ts
var __Facade_ScheduledController__ = class ___Facade_ScheduledController__ {
  constructor(scheduledTime, cron, noRetry) {
    this.scheduledTime = scheduledTime;
    this.cron = cron;
    this.#noRetry = noRetry;
  }
  static {
    __name(this, "__Facade_ScheduledController__");
  }
  #noRetry;
  noRetry() {
    if (!(this instanceof ___Facade_ScheduledController__)) {
      throw new TypeError("Illegal invocation");
    }
    this.#noRetry();
  }
};
function wrapExportedHandler(worker) {
  if (__INTERNAL_WRANGLER_MIDDLEWARE__ === void 0 || __INTERNAL_WRANGLER_MIDDLEWARE__.length === 0) {
    return worker;
  }
  for (const middleware of __INTERNAL_WRANGLER_MIDDLEWARE__) {
    __facade_register__(middleware);
  }
  const fetchDispatcher = /* @__PURE__ */ __name(function(request, env, ctx) {
    if (worker.fetch === void 0) {
      throw new Error("Handler does not export a fetch() function.");
    }
    return worker.fetch(request, env, ctx);
  }, "fetchDispatcher");
  return {
    ...worker,
    fetch(request, env, ctx) {
      const dispatcher = /* @__PURE__ */ __name(function(type, init) {
        if (type === "scheduled" && worker.scheduled !== void 0) {
          const controller = new __Facade_ScheduledController__(
            Date.now(),
            init.cron ?? "",
            () => {
            }
          );
          return worker.scheduled(controller, env, ctx);
        }
      }, "dispatcher");
      return __facade_invoke__(request, env, ctx, dispatcher, fetchDispatcher);
    }
  };
}
__name(wrapExportedHandler, "wrapExportedHandler");
function wrapWorkerEntrypoint(klass) {
  if (__INTERNAL_WRANGLER_MIDDLEWARE__ === void 0 || __INTERNAL_WRANGLER_MIDDLEWARE__.length === 0) {
    return klass;
  }
  for (const middleware of __INTERNAL_WRANGLER_MIDDLEWARE__) {
    __facade_register__(middleware);
  }
  return class extends klass {
    #fetchDispatcher = /* @__PURE__ */ __name((request, env, ctx) => {
      this.env = env;
      this.ctx = ctx;
      if (super.fetch === void 0) {
        throw new Error("Entrypoint class does not define a fetch() function.");
      }
      return super.fetch(request);
    }, "#fetchDispatcher");
    #dispatcher = /* @__PURE__ */ __name((type, init) => {
      if (type === "scheduled" && super.scheduled !== void 0) {
        const controller = new __Facade_ScheduledController__(
          Date.now(),
          init.cron ?? "",
          () => {
          }
        );
        return super.scheduled(controller);
      }
    }, "#dispatcher");
    fetch(request) {
      return __facade_invoke__(
        request,
        this.env,
        this.ctx,
        this.#dispatcher,
        this.#fetchDispatcher
      );
    }
  };
}
__name(wrapWorkerEntrypoint, "wrapWorkerEntrypoint");
var WRAPPED_ENTRY;
if (typeof middleware_insertion_facade_default === "object") {
  WRAPPED_ENTRY = wrapExportedHandler(middleware_insertion_facade_default);
} else if (typeof middleware_insertion_facade_default === "function") {
  WRAPPED_ENTRY = wrapWorkerEntrypoint(middleware_insertion_facade_default);
}
var middleware_loader_entry_default = WRAPPED_ENTRY;
export {
  Room,
  __INTERNAL_WRANGLER_MIDDLEWARE__,
  middleware_loader_entry_default as default
};
//# sourceMappingURL=worker.js.map
