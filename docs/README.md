# KTLT Banking Terminal

A high-performance client-server banking application built with **C++26**, featuring a terminal UI powered by **FTXUI** and an asynchronous networking layer using **ASIO**.

---

## Table of Contents

- [Architecture](#architecture)
- [Getting Started](#getting-started)
- [Network Protocol](#network-protocol)
- [Base Library](#base-library)
- [Server Reference](#server-reference)
- [Client Reference](#client-reference)
- [Testing](#testing)

---

## Architecture

```
┌─────────────────────┐         TCP/IP        ┌──────────────────────┐
│   Client (TUI)      │ ◄───────────────────► │   Server             │
│   app/              │      custom packet    │   server/            │
│   FTXUI + ASIO      │      protocol         │   ASIO + threaded DB │
└─────────────────────┘                       └──────────────────────┘
                                                         │
                                                ┌────────┴────────┐
                                                │  data.db        │
                                                │  data/*.txn     │
                                                │  data/*.log     │
                                                └─────────────────┘
```

### Key Design Decisions

| Decision | Rationale |
|---|---|
| **Custom base library** (`libs/base/`) | Educational project goal; reimplements STL containers (Vector, List, Map, String, SHA-256) with C++26 features |
| **Binary flat-file DB** | Simple, portable, no external DB engine needed; uses packed structs with network byte order |
| **ASIO for networking** | Header-only, cross-platform async I/O; used for both client and server |
| **FTXUI for TUI** | Modern terminal UI library with mouse support, keyboard navigation, and rich styling |
| **Dedicated DB worker thread** | Keeps the networking event loop responsive by isolating file I/O |

### Directory Structure

```
KTLT/
├── app/                    # Client TUI application
│   ├── main.cpp            # Entry point
│   ├── App.cpp/.hpp        # App shell, tab navigation, global shortcuts
│   ├── Theme.hpp           # Catppuccin Mocha color palette & button styles
│   ├── client/             # Async network client
│   └── pages/              # TUI page components
├── server/                 # Banking server
│   ├── main.cpp            # Entry point with signal handling
│   ├── Server.cpp/.hpp     # TCP acceptor & session management
│   ├── Database.cpp/.hpp   # Flat-file DB engine, async worker thread
│   └── ServerLog.cpp/.hpp  # Thread-safe formatted logging
├── libs/
│   ├── base/               # Custom data structures (Vector, String, SHA-256, etc.)
│   └── network/            # Binary packet protocol (send/recv, sync/async)
├── data/                   # Runtime data directory (DB + logs + transaction files)
├── tests/                  # Test data generator
├── docs/                   # This document
├── CMakeLists.txt          # Build configuration
└── .clang-format           # Code style
```

---

## Getting Started

### Prerequisites

- **CMake** 4.2+
- **C++26** compiler (e.g., GCC 15+, Clang 19+)
- **FTXUI** (https://github.com/ArthurSonzogni/FTXUI) — install system-wide or via vcpkg
- **ASIO** (header-only, included via `find_package` or fetch)

### Build

```bash
cd KTLT
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Run

```bash
# Terminal 1: start the server
./build/server

# Terminal 2: start the client app
./build/app

# (Optional) generate test data
./build/generate_test_data
```

### Default Accounts (test data)

| Username | Password  | Notes |
|---|---|---|
| `admin`  | `admin123` | Full transaction history |
| `user1`  | `password1` | Regular user with history |
| `locked` | `lockedpw`  | Account is locked (created by generator) |

The server listens on `0.0.0.0:8080`. The client connects to `127.0.0.1:8080`.

### Client Keyboard Shortcuts

| Key | Action |
|---|---|
| `Esc` | Exit application |
| `Ctrl+Left` / `[` | Previous tab |
| `Ctrl+Right` / `]` | Next tab |
| `R` | Refresh current data (Dashboard / History) |
| `Tab` | Cycle focus between inputs |
| `Enter` | Submit form / confirm |

---

## Network Protocol

### Packet Format

```
┌──────────┬──────────┬──────────────┐
│  Length  │   Type   │   Payload    │
│  (4 BEs) │  (1 B)   │  (variable)  │
└──────────┴──────────┴──────────────┘
```

- **Length**: 4 bytes, network byte order (big-endian) — size of the payload
- **Type**: 1 byte — packet type identifier
- **Payload**: `Length` bytes of data

### Packet Types

| Value | Name | Direction | Payload |
|---|------|-----------|---------|
| 0 | `account_auth` | C→S | `username:password` |
| 1 | `account_create` | C→S | `username:password` |
| 2 | `balance_req` | C→S | `username` |
| 3 | `balance_change` | C→S | `username:change` (change as signed integer string) |
| 4 | `balance_transfer` | C→S | `sender:recipient:amount` |
| 5 | `ping` | C→S | (empty) |
| 6 | `account_change_password` | C→S | `username:newPassword` |
| 7 | `account_toggle` | C→S | `username` |
| 8 | `log_req` | C→S | `username` |
| 9 | `txn_history_req` | C→S | `username:count` |
| 10 | `user_search_req` | C→S | `prefix` (empty = all users) |

All server responses have a payload of either `"OK"`, `"FAIL"`, or data content.

### API (`libs/network/Packet.{hpp,cpp}`)

```cpp
namespace packet {
    // Synchronous
    void send(socket, PacketType, const std::string& data);
    auto recv(socket) -> std::pair<PacketType, std::string>;

    // Asynchronous
    void asyncSend(socket, PacketType, const std::string& data,
                   std::function<void(std::error_code)> handler);
    void asyncRecv(socket,
                   std::function<void(std::error_code, PacketType, std::string)> handler);
}
```

---

## Base Library (`libs/base/`)

A header-only reimplementation of common data structures for educational purposes. All containers use C++26 features (deducing `this`, concepts, `constexpr`).

### Vector (`vector.hpp`)

Dynamic array with growth factor 2, exception-safe memory management, and full iterator support.

```cpp
base::Vector<int> v;
v.push_back(42);
v.emplace_back(1, 2, 3);
```

### String (`string.hpp`)

Small String Optimization (SSO) string with 23 chars on the stack before heap allocation.

```cpp
base::String s("hello");
s.starts_with("he");  // true
s += " world";
```

### List (`list.hpp`)

Doubly-linked list with sentinel node, merge sort, `O(1)` splice operations.

### HashTable, HashMap, HashSet (`hash_table.hpp`, `hash_map.hpp`, `hash_set.hpp`)

Separate-chaining hash map/set with dynamic rehashing.

### RedBlackTree, Map, Set (`rbtree.hpp`, `map.hpp`, `set.hpp`)

Ordered associative containers backed by a red-black tree (`O(log n)` operations).

### SHA-256 (`sha256.hpp`)

Standalone SHA-256 implementation used for password hashing with a random salt.

### Algorithm (`algorithm.hpp`)

Generic algorithms: `swap`, `find`, `find_if`, `sort` (quicksort with Hoare partition + insertion sort fallback for small ranges).

### Types (`types.hpp`)

Portable integer aliases: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`, `usize`, `isize`, `f32`, `f64`, `b1`/`b8`/etc., `byte`.

---

## Server Reference

### Entry Point (`server/main.cpp`)

Initializes the `Server` on port 8080, installs a `SIGINT`/`SIGTERM` handler for graceful shutdown.

### Server (`server/Server.{hpp,cpp}`)

- Listens for TCP connections on all interfaces
- Creates a `Session` (via `shared_from_this`) per connection
- `Session` reads packets in a loop: `doRead()` → process → `doWrite()` → `doRead()`
- Routes each `PacketType` to the corresponding `DbManager` method
- Validates usernames against path traversal (`/`, `\\`, `..`)

### Database (`server/Database.{hpp,cpp}`)

#### Data Structures

```
Record (packed, 113 bytes)
┌──────────┬────────────┬─────────┬──────────┬──────────┐
│ username │  password  │ balance │ logFile  │ isLocked │
│ (24 B)   │  (48 B)    │  (8 B)  │  (32 B)  │  (1 B)   │
└──────────┴────────────┴─────────┴──────────┴──────────┘

TransactionRecord (packed, 89 bytes)
┌──────────┬──────────────┬────────┬──────────────┬───────────┬──────┐
│ username │ counterparty │ amount │ balanceAfter │ timestamp │ type │
│  (24 B)  │    (24 B)    │ (8 B)  │    (8 B)     │   (8 B)   │(1 B) │
└──────────┴──────────────┴────────┴──────────────┴───────────┴──────┘
```

All multi-byte integers are stored in **network byte order** (big-endian).

#### Password Storage

- Each user gets a unique 16-byte random salt (alphanumeric)
- Stored as: `salt (16 chars) + SHA-256 binary hash (32 bytes)` = 48 bytes
- Hash computation: `SHA-256(salt + ":" + password)`

#### Transaction Flow

| Operation | Database effect |
|---|---|
| `changeBalance(+n)` | Debit user, append `TransactionRecord(deposit)` |
| `changeBalance(-n)` | Credit user (if sufficient balance), append `TransactionRecord(withdraw)` |
| `transferBalance` | Debit sender + credit recipient atomically, append `transfer_out` + `transfer_in` records |
| `createAccount` | Create record with `DefaultBalance` (100,000), record initial deposit transaction |

#### Persistence

- **`data.db`**: Binary file with all `Record` entries
- **`data/<user>.txn`**: Binary file with all `TransactionRecord` entries
- **`data/<user>.log`**: Human-readable text log (not used for state recovery)
- Auto-save every 30 seconds via `asio::steady_timer`

### Logging (`server/ServerLog.{hpp,cpp}`)

Thread-safe formatted logging to stdout with timestamps:

```
[08:30:01] [INFO] listening on port 8080
[08:30:05] [AUTH] 127.0.0.1:54321 user 'admin' OK
[08:30:10] [BALANCE] 127.0.0.1:54321 user 'admin' balance changed
```

---

## Client Reference

### App (`app/App.cpp`)

The main application shell manages:

- **Tab navigation**: Pre-auth (Login / Sign Up) and post-auth (Dashboard / History / Deposit / Withdraw / Transfer / Settings)
- **Global keyboard shortcuts**: Escape (exit), Ctrl+Arrow/`[]` (tab switch), `R` (refresh)
- **Connection lifecycle**: Auto-connects on startup, handles disconnection gracefully by returning to login screen
- **Disconnect handler**: Resets UI state and displays "Connection to server lost"

### Client (`app/client/Client.{hpp,cpp}`)

Asynchronous network client wrapping the packet protocol:

```cpp
Client client("127.0.0.1", 8080);
client.connect([](bool ok) { ... });
client.authenticate("user", "pass", [](bool ok) { ... });
client.getBalance("user", [](std::optional<u64> bal) { ... });
```

- Runs its own `asio::io_context` in a dedicated thread
- Request queue maps responses back to callbacks (FIFO order)
- `onDisconnect` hook notifies the UI when the connection drops

### Pages (`app/pages/`)

| Page | File(s) | Features |
|---|---|---|
| **Login** | `LoginPage` | Username/password inputs, loading spinner, error display, auto-focus |
| **Sign Up** | `SignupPage` | Password strength meter, confirm password match indicator |
| **Dashboard** | `DashboardPage` | Balance display, latency indicator, recent activity table, balance history graph |
| **History** | `HistoryPage` | Full transaction history with search filter and sortable columns |
| **Deposit** | `DepositPage` | Numeric input validation, success/error feedback |
| **Withdraw** | `WithdrawPage` | Confirmation dialog before processing |
| **Transfer** | `TransferPage` | Recipient autocomplete via server-side user search |
| **Settings** | `SettingPage` | Password change (requires current password verification), logout, account lock with confirmation |

### Theme (`app/Theme.hpp`)

Catppuccin Mocha color palette with a helper `Button()` function for consistent button styling across all pages.

---

## Testing

### Test Data Generator (`tests/generate_test_data.cpp`)

Generates `data.db`, `.txn`, and `.log` files with three accounts:

- **admin** (30+ transactions, high activity)
- **user1** (30+ transactions, moderate activity)
- **locked** (account creation only, `isLocked = true`)

```bash
# Regenerate test data (destroys existing data.db)
./build/generate_test_data
```

All passwords are hashed with the same SHA-256 + salt scheme as the server for compatibility.

---

## Build Configuration

Key CMake targets:

| Target | Type | Dependencies |
|---|---|---|
| `app` | Executable | network, ftxui (screen, dom, component), Threads |
| `server` | Executable | network, Threads |
| `network` | Library | Threads |
| `generate_test_data` | Executable | (standalone) |

C++26 standard required. ASIO (header-only) and FTXUI are external dependencies.

---

## Security Notes

- **Passwords**: SHA-256 hashed with a unique 16-byte salt per user. No plaintext stored.
- **Path Traversal**: Usernames are validated against `/`, `\\`, `..`, and null bytes before being used in file paths.
- **Network**: The protocol is not encrypted. Use only on trusted networks or add TLS.
- **Account Locking**: Locked accounts cannot authenticate, change balance, or perform transfers.
- **Input Validation**: Client-side enforces max 23 chars for username/password; server-side validates where security-relevant.
