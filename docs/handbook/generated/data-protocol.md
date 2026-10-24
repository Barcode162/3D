<!-- generated: node tools/handbook.mjs write (from cloud/proto.js); never edit by hand -->
# Data: the room protocol

The messages between a phone and the room, read from the codec itself (`cloud/proto.js`; the game's twin is `looped/netproto.*`, both checked on `proto/vectors.json`). The full spec with every byte: `docs/PROTOCOL.md` ([the protocol](../systems/engine/net-codec-and-protocol.md)).

- Protocol version: **1** (the room also accepts the one before)
- Slots per room: 16
- Largest text to the room: 4096 bytes (a voice signal: 16384); largest to a phone: 65536

## Binary messages

| Type | Byte |
|---|---|
| `STATE` | `0x01` |
| `PROP` | `0x02` |
| `TICK` | `0x81` |

## Text messages: phone → room

One type letter + a JSON object; `?` marks an optional field.

| Letter | Message | Fields |
|---|---|---|
| `H` | hello | proto, build, name, look, resume?, ticket? |
| `I` | intent | i, k |
| `E` | events | e |
| `K` | kv | w |
| `G` | signal | to, data |
| `P` | ping | c |
| `R` | reauth | ticket |
| `A` | away | on |

## Text messages: room → phone

| Letter | Message | Fields |
|---|---|---|
| `W` | welcome | proto, slot, resume, serverMs, tick, tickHz, room, roster, claims, sessions, kv? |
| `D` | delta | d |
| `R` | result | r, ok, seq?, why?, by? |
| `E` | events | e |
| `K` | kv | w |
| `J` | join | slot, name, look, acct?, av?, away? |
| `L` | leave | slot, why? |
| `F` | profile | slot, name, acct?, av?, badges?, role? |
| `G` | signal | from, data |
| `P` | pong | c, s |
