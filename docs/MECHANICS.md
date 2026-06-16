# Server & Client Mechanics

## Architecture Overview

```
┌──────────┐     TCP/8080      ┌──────────┐
│  Server  │ ◄──────────────►  │  Client  │
│ (server/)│    packet proto   │ (app/)   │
└──────────┘                   └──────────┘
```

## Server

The server runs a single `io_context` event loop on the main thread. Each connected client gets a `Session` (`shared_ptr`) with its own socket.

```
Server::run()
  └─ _io.run()
       ├─ doAccept()          — waits for new connections
       │    └─ Session::start()
       │         └─ doRead()  — waits for packets
       │              ├─ account_auth  ─┐
       │              ├─ account_create │
       │              ├─ balance_req    ├─ dispatch via DbManager
       │              ├─ balance_change │
       │              └─ balance_transfer
       │              └─ doWrite() → doRead()  — reply, then read next
```

**Thread model**: single thread (`_io.run()` on main). All socket ops and DbManager work share this thread. `DbManager` spawns its own background worker thread for file I/O, but callbacks are posted back to the main `_io` via `asio::post`.

**Session lifecycle**:
1. `doAccept` creates `make_shared<Session>(socket, db)` and calls `start()`
2. `start()` logs "connected", begins `doRead()`
3. `doRead()` calls `packet::asyncRecv` — reads header + body asynchronously
4. On packet received, switches on `PacketType`, calls `_db` method, passes a callback that replies via `doWrite()`
5. `doWrite()` sends the response, then calls `doRead()` again
6. Disconnection is detected when `asyncRecv`/`asyncSend` returns an error — "disconnected" is logged, session ends

## Client

The client runs its own `io_context` on a background thread, keeping the main thread free for the ftxui UI loop.

```
App::run()  (main thread)
  ├─ std::thread clientThread  ──  Client::run() → _io.run()
  │     (blocked in asio, processing socket ops)
  │
  ├─ client.connect(...)       — async_resolve → async_connect
  ├─ LoginPage — ftxui UI
  ├─ DashboardPage — ftxui UI
  └─ screen.Loop(container)    — ftxui event loop
       └─ user clicks Login
            └─ client.authenticate(user, pass, callback)
                 └─ sendRequest(PacketType::account_auth, payload)
                      └─ asio::post(_io, ...)  — dispatch to client thread
                           └─ packet::asyncSend → packet::asyncRecv
                                └─ callback fires on client thread
                                     └─ screen.Post(...) → main thread callback
```

### Thread Safety

| Resource | Accessed by | Protection |
|----------|-------------|------------|
| `_socket` | client thread only | `asio::post` queues all ops onto `_io` |
| `_connected` | client thread (write), main thread (read) | `std::atomic<bool>` |
| `_io` | client thread (`_io.run()`), main thread (`asio::post`) | asio is thread-safe for posting |
| Ui state | main thread only | ftxui single-threaded |
| Network callbacks | client thread | `screen.Post` bridges to main thread |

### Work Guard

`Client` holds an `asio::executor_work_guard` that keeps `_io.run()` alive even when no handlers are queued. Without it, `_io.run()` would return immediately on thread start because no handlers exist yet. The work guard is only released when `_io.stop()` is called (in `~Client`).

## Login Flow (Step by Step)

```
App::run()
  │  Client client("127.0.0.1", 8080)
  │  std::thread([&] { client.run(); })  // _io.run() with work_guard
  │
  ├── client.connect(handler)
  │     └─ async_resolve("127.0.0.1", "8080")
  │          └─ on client thread: async_connect(socket, results)
  │               └─ on client thread: _connected = true
  │                                    handler(true)  // discard, no action
  │
  ├── LoginPage built, screen.Loop starts
  │
  │  ┌─ User enters "admin" / "admin123" and presses Login ──┐
  │  │                                                       │
  │  │  LoginPage::doLogin()  (main thread)                  │
  │  │   _loading = true                                     │
  │  │   _client.authenticate("admin", "admin123", callback) │
  │  │     └─ sendRequest(PacketType::account_auth,          │
  │  │                     "admin:admin123", handler)        │
  │  │          └─ asio::post(_io, lambda)                   │
  │  │               └─ on client thread:                    │
  │  │                    check _connected  ✓                │
  │  │                    packet::asyncSend(socket,          │
  │  │                      account_auth, "admin:admin123")  │
  │  │                    └─ async_write 5-byte header + body│
  │  │                         └─ on complete:               │
  │  │                              packet::asyncRecv(socket)│
  │  │                              └─ async_read header     │
  │  │                                   └─ on complete:     │
  │  │                                        async_read body│
  │  │                                        └─ on complete:│
  │  │                handler(true, "OK")                    │
  │  │                └─ screen.Post([&] {                   │
  │  │                     _loading = false                  │
  │  │                     onLogin("admin")                  │
  │  │                     // sets page=1, triggers re-render│
  │  │                   })                                  │
  │  │                                                       │
  │  │  ┌─ screen.Post runs on main thread ─────────────────┐│
  │  │  │  LoginPage callback:                              ││
  │  │  │    _loading = false                               ││
  │  │  │    ok = true → _onLogin("admin")                  ││
  │  │  │      └─ loggedInUser = "admin"                    ││
  │  │  │         page = 1                                  ││
  │  │  │         screen.Post([&]{})  // trigger re-render  ││
  │  │  │                                                   ││
  │  │  │  Container::Tab renders: *page=1 → DashboardPage  ││
  │  │  └───────────────────────────────────────────────────┘│
  │  └───────────────────────────────────────────────────────┘
  │
  └── User sees Dashboard with "admin" and balance
```

### Wire Format for Login

```
Client → Server:   [0x05 0x00 0x00 0x00] [0x00] "admin:admin123"
                    └── length=5 BE ──┘   └type┘ └── payload ──┘
                        
                        length   = 5 (payload "admin" has 5 chars... wait)

Actually "admin:admin123" = 14 bytes:

Client → Server:   [0x00 0x00 0x00 0x0E] [0x00] "admin:admin123"
                    └── length=14 BE ─┘   └type┘ └── payload ──┘

Server → Client:   [0x00 0x00 0x00 0x02] [0x00] "OK"
```

### Callback Chain (Thread Boundaries)

```
main thread                    client thread                server thread
─────────────                  ─────────────                ─────────────
LoginPage::doLogin()
  └─ Client::authenticate()
       └─ Client::sendRequest()
            └─ asio::post() ──► lambda()
                                 └─ asyncSend() ──────────► Session::doRead()
                                                              └─ DbManager::authenticate()
                                                                   └─ callback()
                                                                        └─ doWrite("OK")
                                      ◄── asyncRecv() ────
                                           └─ handler() 
                                                └─ screen.Post() ──► login callback
                                                                      └─ set page=1
```

### Key Points

- **All socket I/O runs on the client's `io_context` thread** — the main thread never touches `_socket` directly thanks to `asio::post` in `sendRequest`.
- **Callbacks cross threads via `screen.Post`** — network responses arrive on the client thread, but UI state must be updated on the ftxui main thread.
- **The work guard is essential** — without it, `_io.run()` returns immediately before any handlers are queued, and the connection never establishes.
- **The server is single-threaded** — all sessions and DB operations share one thread, so there is no internal concurrency on the server side.
