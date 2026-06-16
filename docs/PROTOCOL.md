# Socket Server Protocol

## Packet Format

Every message uses a 5-byte header followed by a variable-length body:

```
┌───────────┬──────────────┬──────────────────┐
│  4 bytes  │   1 byte     │  variable length │
│  length   │    type      │     payload      │
│ (BE u32)  │ (PacketType) │   (raw UTF-8)    │
└───────────┴──────────────┴──────────────────┘
```

- **length**: payload size in bytes, big-endian unsigned 32-bit
- **type**: `PacketType` enum value
- **payload**: raw UTF‑8 string (the server parses delimiters within)

## Packet Types

| ID | Name | Direction | Payload Format |
|----|------|-----------|----------------|
| 0 | `account_auth` | → `"OK"` / `"FAIL"` | `username:password` |
| 1 | `account_create` | → `"OK"` / `"FAIL"` | `username:password` |
| 2 | `balance_req` | → decimal string | `username` |
| 3 | `balance_resp` | ← server only | balance as decimal string |
| 4 | `balance_change` | → `"OK"` / `"FAIL"` | `username:change` |
| 5 | `balance_transfer` | → `"OK"` / `"FAIL"` | `sender:recipient:amount` |
| 6 | `ping` | (unused) | – |
| 7 | `account_change_password` | → `"OK"` / `"FAIL"` | `username:newpassword` |
| 8 | `account_toggle` | → `"OK"` / `"FAIL"` | `username` |

## Operations

### Authentication (`account_auth`)

**Request**: `"username:password"`
**Response**: `"OK"` on valid credentials and unlocked account, else `"FAIL"`.

A locked account always returns `"FAIL"` regardless of password correctness.

---

### Create Account (`account_create`)

**Request**: `"username:password"`
**Response**: `"OK"` (even if account already exists — idempotent).

New accounts are created with:
- balance = `100'000`
- `logFile` = `username + ".log"`
- unlocked

---

### Get Balance (`balance_req`)

**Request**: `"username"`
**Response**: balance as a decimal string (e.g. `"100000"`). Returns `"0"` if the user does not exist.

---

### Change Balance (`balance_change`)

**Request**: `"username:change"`
**Response**: `"OK"`.

`change` is a signed integer (`i64`). If the result would go below zero, it is clamped to `0`.

---

### Transfer Balance (`balance_transfer`)

**Request**: `"sender:recipient:amount"`
**Response**: `"OK"`.

The transfer succeeds only if the sender exists, the recipient exists, and the sender's balance is ≥ `amount`. Otherwise the request is silently ignored (still returns `"OK"`).

---

### Change Password (`account_change_password`)

**Request**: `"username:newpassword"`
**Response**: `"OK"`.

If the username does not exist the request is silently ignored.

---

### Toggle Account Lock (`account_toggle`)

**Request**: `"username"`
**Response**: `"OK"`.

Flips the locked/unlocked flag. A locked account cannot authenticate. If the username does not exist the request is silently ignored.

## Payload Delimiters

- Single field: the entire payload is the value (`balance_req`, `account_toggle`)
- Two fields: separated by the first `:` (`account_auth`, `account_create`, `balance_change`, `account_change_password`)
- Three fields: separated by two `:` delimiters (`balance_transfer`)

Malformed payloads (missing delimiter) reply with `"FAIL"`.

## Implementation Notes

- The database is backed by a binary file (`data.db`) containing packed `Record` structs, big-endian.
- All operations are serialized through a single worker thread — requests are queued and processed sequentially.
- Callbacks are invoked from the worker thread, not the `io_context` event loop.
- The server alternates between `doRead` → `doWrite` → `doRead` per session.
