# Cơ chế Server & Client

> Tài liệu này mô tả chi tiết cách server và client trong project KTLT vận hành — từ luồng chạy, threading model, đến từng bước xử lý request.

---

## Mục lục

1. [Tổng quan kiến trúc](#1-tổng-quan-kiến-trúc)
2. [Server Mechanics](#2-server-mechanics)
   - 2.1. [Vòng đời Server](#21-vòng-đời-server)
   - 2.2. [Vòng đời Session](#22-vòng-đời-session)
   - 2.3. [Cơ chế Database Worker](#23-cơ-chế-database-worker)
3. [Client Mechanics](#3-client-mechanics)
   - 3.1. [Vòng đời Client](#31-vòng-đời-client)
   - 3.2. [Cơ chế Send Request](#32-cơ-chế-send-request)
   - 3.3. [Cơ chế Recv Loop](#33-cơ-chế-recv-loop)
4. [Threading & An toàn luồng](#4-threading--an-toàn-luồng)
   - 4.1. [Bảng an toàn luồng](#41-bảng-an-toàn-luồng)
   - 4.2. [Work Guard](#42-work-guard)
5. [Luồng đăng nhập (từng bước)](#5-luồng-đăng-nhập-từng-bước)
6. [Luồng tạo tài khoản (Signup)](#6-luồng-tạo-tài-khoản-signup)

---

## 1. Tổng quan kiến trúc

```
┌──────────────────────────────────────────────────────────────────┐
│                         TCP / 8080                               │
│                                                                  │
│  ┌──────────────┐          Packet           ┌──────────────┐    │
│  │    Server    │ ◄══════════════════════►  │    Client    │    │
│  │  (server/)  │    5B header + payload    │    (app/)    │    │
│  └──────┬───────┘                          └──────┬───────┘    │
│         │                                        │             │
│         ├─ Asio io_context                       ├─ FTXUI UI   │
│         ├─ Session (per client)                  ├─ Asio thread │
│         └─ DbManager + worker thread             └─ recv loop   │
└──────────────────────────────────────────────────────────────────┘
```

- **Server**: đơn luồng Asio + worker thread cho database. Mỗi kết nối client là một `Session`.
- **Client**: hai luồng — main thread chạy giao diện FTXUI, background thread chạy Asio cho mạng.

---

## 2. Server Mechanics

### 2.1. Vòng đời Server

```cpp
// server/main.cpp
auto main() -> int {
    Server server(8080);   // Mở cổng, bắt đầu lắng nghe
    server.run();          // _io.run() — chạy mãi mãi
}
```

```
Server::Server(8080)
  │
  ├─ asio::ip::tcp::acceptor(_io, {tcp::v4(), 8080})
  ├─ DbManager(_io)
  │    ├─ load()              ← đọc data.db vào _data map
  │    └─ std::thread(dbLoop) ← worker thread khởi động
  │
  └─ doAccept()               ← async_accept, chờ kết nối

Server::run()
  │
  └─ _io.run()                ← event loop chính (blocking)
       │
       ├─ Khi có client mới: doAccept() callback
       │    └─ make_shared<Session>(socket, _db)
       │         └─ start()
       │              └─ doRead()
       │
       ├─ Khi session ghi/đọc: callback tương ứng
       │
       └─ Khi worker thread post callback:
            └─ asio::post(_io, ...) — chạy trong event loop
```

### 2.2. Vòng đời Session

```
Session::start()
  │
  ├─ ServerLog::info("connected")
  │
  └─ doRead()                          ╔═══════════════════╗
       │                                ║  VÒNG LẶP CHÍNH  ║
       ├─ packet::asyncRecv(_socket)    ║                   ║
       │    └─ callback(ec, type, data) ║  doRead()         ║
       │         │                      ║    │              ║
       │         ├─ ec?                 ║    ├─ parse type  ║
       │         │    └─ log disconnect ║    ├─ dispatch    ║
       │         │    └─ END            ║    └─ doWrite()   ║
       │         │                      ║         │         ║
       │         └─ switch(type)        ║         ├─ async  ║
       │              ├─ account_auth   ║         └─ doRead()║
       │              ├─ account_create ║                    ║
       │              ├─ balance_req    ║  doRead ←→ doWrite ║
       │              ├─ balance_change ║  luân phiên mãi    ║
       │              ├─ balance_transfer║                   ║
       │              └─ ...            ╚═══════════════════╝
       │
       └─ Gọi _db.someOp(params, callback)
            │
            └─ callback chạy khi worker thread xong
                 └─ doWrite(type, response)
                      └─ packet::asyncSend(socket, ...)
                           └─ callback → doRead() ← quay lại
```

**Chi tiết doRead() — dispatch theo PacketType:**

```cpp
// server/Server.cpp — Session::doRead()
using enum PacketType;

switch (type) {
case account_auth: {
    // Parse "username:password"
    auto delim = data.find(':');
    auto user  = data.substr(0, delim);
    auto pass  = data.substr(delim + 1);
    // Enqueue auth operation
    _db.authenticate(user, pass, [this, self](bool ok) {
        doWrite(std::to_underlying(account_auth), ok ? "OK" : "FAIL");
    });
    break;
}
case balance_req: {
    _db.getBalance(data, [this, self](u64 balance) {
        doWrite(std::to_underlying(balance_req), std::to_string(balance));
    });
    break;
}
case balance_transfer: {
    // Parse "sender:recipient:amount"
    auto delim1 = data.find(':');
    auto delim2 = data.find(':', delim1 + 1);
    auto sender    = data.substr(0, delim1);
    auto recipient = data.substr(delim1 + 1, delim2 - delim1 - 1);
    auto amount    = std::stoull(data.substr(delim2 + 1));
    _db.transferBalance(sender, recipient, amount, [this, self](bool ok) {
        doWrite(std::to_underlying(balance_transfer), ok ? "OK" : "FAIL");
    });
    break;
}
// ... các case khác tương tự
}
```

### 2.3. Cơ chế Database Worker

```
                 ┌──────────────────┐
                 │   Luồng chính    │
                 │  (io_context)    │
                 └────────┬─────────┘
                          │
                ┌─────────▼─────────┐
                │  db.authenticate() │
                │  db.getBalance()   │
                │  db.transfer()     │
                └─────────┬─────────┘
                          │ lock(mutex)
                          │ queue.push(WorkItem)
                          │ cv.notify_one()
                          │ unlock(mutex)
                          │ return ngay
                          │
                 ┌────────▼────────┐
                 │  Worker Thread  │
                 │  (dbLoop)       │
                 └────────┬────────┘
                          │
                ┌─────────▼─────────┐
                │  cv.wait(lock)    │
                │  queue.pop()      │
                │  processItem()    │
                └─────────┬─────────┘
                          │
                ┌─────────▼─────────┐
                │  std::visit(...)  │
                │                   │
                │  AuthOp:          │
                │    tìm user       │
                │    check lock     │
                │    so sánh pass   │
                │                   │
                │  TransferBalanceOp:│
                │    tìm sender     │
                │    tìm recipient  │
                │    check balance  │
                │    trừ + cộng     │
                │    ghi log cả 2   │
                │                   │
                │  ChangeBalanceOp: │
                │    check đủ tiền  │
                │    cập nhật       │
                │    ghi log        │
                └─────────┬─────────┘
                          │
                ┌─────────▼─────────┐
                │  asio::post(_io,  │
                │    callback)      │
                └─────────┬─────────┘
                          │
                 ┌────────▼────────┐
                 │   Luồng chính   │
                 │  callback chạy  │
                 │  → doWrite()    │
                 └─────────────────┘
```

**Cấu trúc WorkItem (std::variant):**

```cpp
using WorkItem = std::variant<
    AuthOp,             // authenticate
    ChangePasswordOp,   // changePassword
    CreateAccountOp,    // createAccount
    ToggleAccountOp,    // toggleAccount
    GetBalanceOp,       // getBalance
    ChangeBalanceOp,    // changeBalance (nạp/rút)
    TransferBalanceOp   // transferBalance
>;
```

**processItem() dùng std::visit để dispatch type-safe:**

```cpp
// server/Database.cpp
auto DbManager::processItem(WorkItem&& item) -> void {
    auto post = [this](auto cb) {
        asio::post(_io, std::move(cb));
    };

    std::visit([this, &post](auto&& op) {
        using T = std::decay_t<decltype(op)>;

        if constexpr (std::is_same_v<T, AuthOp>) {
            auto it = _data.find(op.username);
            if (it == _data.end() || it->second.isLocked) {
                post([cb = std::move(op.callback)] { if (cb) cb(false); });
                return;
            }
            auto ok = asString(it->second.password) == op.password;
            post([cb = std::move(op.callback), ok] { if (cb) cb(ok); });
        }
        else if constexpr (std::is_same_v<T, TransferBalanceOp>) {
            auto src = _data.find(op.sender);
            auto dst = _data.find(op.recipient);
            if (src == _data.end() || dst == _data.end()
                || src->second.balance < op.amount) {
                post([cb = std::move(op.callback)] { if (cb) cb(false); });
                return;
            }
            src->second.balance -= op.amount;
            dst->second.balance += op.amount;
            appendLog(asString(src->second.logFile), "...");
            appendLog(asString(dst->second.logFile), "...");
            post([cb = std::move(op.callback)] { if (cb) cb(true); });
        }
        // ... các case khác ...
    }, std::move(item));
}
```

---

## 3. Client Mechanics

### 3.1. Vòng đời Client

Client quản lý 3 page qua `Container::Tab` — chuyển đổi bằng biến `page` (0=Login, 1=Dashboard, 2=Signup).

```cpp
// app/App.cpp — trong App::run()
Client client("127.0.0.1", 8080);
std::thread clientThread([&] { client.run(); });  // Asio background

client.connect([&](bool ok) {                      // async resolve + connect
    if (!ok) screen.Exit();
});

screen.Loop(container);  // Main thread: FTXUI
```

```
App::run()  (main thread)
  │
  ├─ Client("127.0.0.1", 8080)
  │    ├─ _work(io_context)        ← work guard giữ io_context sống
  │    ├─ _socket(_io)             ← socket chưa kết nối
  │    └─ _connected = false
  │
  ├─ std::thread ────► Client::run()
  │                        └─ _io.run()    ← event loop Asio
  │                             │
  │                             ├─ connect: resolve + connect
  │                             ├─ recvLoop: asyncRecv vĩnh viễn
  │                             └─ send: asyncSend
  │
  ├─ LoginPage(client, ...)      ← giao diện (page=0)
  ├─ DashboardPage(client, ...)   ← giao diện (page=1)
  ├─ SignupPage(client, ...)      ← giao diện (page=2)
  │
  ├─ loginPage.setDashboard(dashboard)
  ├─ signupPage.setDashboard(dashboard)
  │
  └─ screen.Loop(container)
       │
       ├─ Render UI
       ├─ Xử lý sự kiện bàn phím
       │    ├─ Bấm Login → client.authenticate()
       │    └─ Bấm Sign Up → page=2 (hiện SignupPage)
       │
       └─ Escape → screen.Exit()
            └─ client.stop() → _io.stop()
            └─ clientThread.join()
```

### 3.2. Cơ chế Send Request

```
Main thread                          Client thread (Asio)
────────────                         ────────────────────

client.authenticate("admin", "admin123", callback)
  │
  └─ Client::sendRequest(
       PacketType::account_auth,
       "admin:admin123",
       handler)
    │
    └─ asio::post(_io, lambda) ────►  _io.run() nhận lambda
                                        │
                                        ├─ Kiểm tra _connected
                                        │    ├─ false → handler(false, {}) ngay
                                        │    └─ true  → tiếp tục
                                        │
                                        ├─ _pending.push(handler)
                                        │    ← xếp handler vào queue
                                        │
                                        └─ packet::asyncSend(
                                             _socket,
                                             account_auth,
                                             "admin:admin123",
                                             onSendDone)
                                           │
                                           └─ async_write header + body
                                                └─ callback → bỏ qua
```

**Payload format từng method:**

| Method | Payload | Ví dụ |
|--------|---------|-------|
| `authenticate` | `"username:password"` | `"admin:admin123"` |
| `createAccount` | `"username:password"` | `"newuser:pass123"` |
| `getBalance` | `"username"` | `"admin"` |
| `changeBalance` | `"username:change"` | `"admin:-5000"` |
| `transferBalance` | `"sender:recipient:amount"` | `"admin:user1:10000"` |
| `changePassword` | `"username:newpassword"` | `"admin:newpass456"` |
| `toggleAccount` | `"username"` | `"admin"` |

### 3.3. Cơ chế Recv Loop

Đây là cơ chế quan trọng nhất của client — thay vì gửi xong đợi nhận (dễ bị treo), client duy trì một **vòng lặp nhận vĩnh viễn**.

```
connect() thành công
  │
  └─ recvLoop()
       │
       └─ packet::asyncRecv(_socket, callback)
            │
            ├─ [1] Đọc header 5 byte
            │    ├─ 4 byte length (big-endian)
            │    └─ 1 byte type
            │
            ├─ [2] Đọc body (length byte)
            │
            └─ callback(ec, type, body)
                 │
                 ├─ ec? (lỗi mạng)
                 │    ├─ _connected = false
                 │    ├─ DRAIN _pending:
                 │    │    while (!_pending.empty())
                 │    │        pop → handler(false, {})
                 │    └─ END (không gọi lại recvLoop)
                 │
                 └─ OK
                      ├─ pop handler từ _pending.front()
                      ├─ handler(true, body)
                      └─ recvLoop()  ← lặp lại!
```

**Tại sao Recv Loop lại tốt hơn blocking?**

```
❌ Cũ (blocking):                    ✅ Mới (recv loop):
                                   
send("admin:admin123")               sendRequest("admin:admin123", handler)
recv() ← treo đến khi có response     _pending.push(handler)
...                                    asyncSend()
(response về)                          │
                                       └─ trả về ngay, không đợi
                                     
                                     recvLoop() chạy song song
                                       ├─ asyncRecv → handler
                                       ├─ asyncRecv → handler
                                       └─ ...

Luồng UI bị treo nếu mạng chậm.       Luồng UI không bao giờ bị treo.
```

**Các handler được xếp hàng FIFO — đảm bảo thứ tự:**

```
sendRequest(auth, handlerA)      _pending queue:
sendRequest(balance, handlerB)   [handlerA] ← [handlerB] ← [handlerC]
sendRequest(transfer, handlerC)
                                      │
                    recvLoop nhận response ──── pop handlerA
                    recvLoop nhận response ──── pop handlerB
                    recvLoop nhận response ──── pop handlerC
```

**Xử lý response trong từng method:**

```cpp
// app/client/Client.cpp
void Client::authenticate(..., BoolHandler handler) {
    auto payload = username + ":" + password;
    sendRequest(PacketType::account_auth, payload,
        [handler = std::move(handler)](bool ok, const std::string& resp) {
            // resp là "OK" hoặc "FAIL"
            if (handler)
                handler(ok && resp == "OK");
        });
}

void Client::getBalance(..., U64Handler handler) {
    sendRequest(PacketType::balance_req, username,
        [handler = std::move(handler)](bool ok, const std::string& resp) {
            // resp là số dư dạng chữ: "999999"
            if (!ok || resp.empty()) {
                if (handler) handler(std::nullopt);
                return;
            }
            auto val = std::stoull(resp);
            if (handler) handler(val);
        });
}
```

---

## 4. Threading & An toàn luồng

### 4.1. Bảng an toàn luồng

| # | Tài nguyên | Thread đọc | Thread ghi | Cơ chế bảo vệ |
|---|------------|------------|------------|---------------|
| 1 | `Client::_socket` | — | Chỉ client thread (Asio) | `asio::post()` — mọi op vào `_io` |
| 2 | `Client::_connected` | Main + Client | Chỉ client thread | `std::atomic<bool>` |
| 3 | `Client::_pending` queue | — | Chỉ client thread | Chỉ truy cập trong `_io` context |
| 4 | `Client::_io` | — | Client + Main | Asio thread-safe cho `post()` |
| 5 | UI state (LoginPage, etc.) | Chỉ main thread | Chỉ main thread | FTXUI đơn luồng |
| 6 | Network callback | Client thread | — | `screen.Post()` → main thread |
| 7 | `DbManager::_queue` | Worker | Main (enqueue) | `std::mutex` + `condition_variable` |
| 8 | `DbManager::_data` map | Worker | Worker | Chỉ worker thread xử lý tuần tự |

**Sơ đồ vượt luồng:**

```
Main thread                    Client thread (Asio)          Server thread
────────────                   ────────────────────         ─────────────

client.authenticate()
  │
  └─ asio::post() ─────────►  _pending.push(handler)
                                asyncSend() ───────────────► Session::doRead()
                                                               └─ DbManager
                                                                   └─ worker
                                                                   └─ asio::post()
                                 ◄── asyncRecv() ─────────────── doWrite()
                                      │
                                      pop handler
                                      handler(true, "OK")
                                        │
                                        └─ screen.Post() ──► UI update
                                                                _loading = false
                                                                _page = 1
```

### 4.2. Work Guard

```cpp
// Client.hpp
asio::executor_work_guard<asio::io_context::executor_type> _work;
```

`io_context::run()` trả về ngay khi không còn việc. Work guard giống như "cọc giữ thuyền" — nó giữ `io_context` ở lại dù chưa có việc gì.

```
Không work guard:
  _io.run()  → không có handler → return ngay
  → thread kết thúc
  → connect() không bao giờ gọi được

Có work guard:
  _io.run()  → có work guard → chờ
  → connect() resolve async → chạy
  → xử lý...
  → _io.stop() → work guard giải phóng → run() kết thúc
```

---

## 5. Luồng đăng nhập (từng bước)

Dưới đây là luồng chi tiết từ lúc user bấm **Login** đến khi thấy **Dashboard**.

```
Thời điểm   Main Thread (FTXUI)        Client Thread (Asio)          Server Thread
────────    ─────────────────────       ─────────────────────         ─────────────

T0          App::run()
              ├─ Client("127.0.0.1", 8080)
              ├─ thread: client.run()
              │                         _io.run() (với work guard)
              ├─ client.connect()
              │                         └─ async_resolve("127.0.0.1")
              │                              └─ async_connect(socket)
              │                                   ├─ _connected = true
              │                                   ├─ recvLoop()
              │                                   │    └─ asyncRecv header...
              │                                   └─ handler(true)
              ├─ screen.Loop() ← rảnh

T1          User nhập "admin", "admin123"
              └─ Bấm Login
                   │
T2             LoginPage::doLogin()
                 ├─ _loading = true
                 └─ client.authenticate("admin", "admin123", cb)
                      └─ sendRequest(account_auth, "admin:admin123", handler)
                           └─ asio::post([...]) ──►

T3                                                  _pending.push(handler)
                                                    asyncSend(socket, auth,
                                                      "admin:admin123")
                                                      │
                                                      └─ async_write ─────►
                                                                           │
T4                                                                     Session::doRead()
                                                                           │
                                                                           └─ parse: type=account_auth
                                                                              user="admin"
                                                                              pass="admin123"
                                                                           └─ _db.authenticate("admin",
                                                                                "admin123", cb)
                                                                                ├─ lock
                                                                                ├─ queue.push(AuthOp{...})
                                                                                ├─ notify
                                                                                └─ unlock
                                                                           │
T5                                                                     Worker thread:
                                                                           processItem(AuthOp)
                                                                             ├─ find("admin")
                                                                             ├─ !isLocked?
                                                                             ├─ password == "admin123"?
                                                                             └─ asio::post(io, [cb]{
                                                                                   cb(true)
                                                                                 })

T6                                  ◄──── asyncRecv() ─────────────────── doWrite(account_auth, "OK")
                                        │
T7                                  recvLoop() callback:
                                      ec=0, type=account_auth, data="OK"
                                        │
                                        ├─ pop handler
                                        ├─ handler(true, "OK")
                                        └─ recvLoop() ← chờ tiếp
                                        │
T8                                  handler:
                                      ok=true, resp="OK"
                                        │
                                        └─ screen.Post([&]{
                                             _loading = false
                                             _appUsername = "admin"
                                             onLogin()
                                               _dashboard.doRefresh()
                                                 client.getBalance("admin",...)
                                               _page = 1
                                           })
                                           │
T9              screen.Post chạy:
                  ├─ _loading = false
                  ├─ _appUsername = "admin"
                  ├─ onLogin()
                  │    ├─ doRefresh() → getBalance("admin", cb)
                  │    │    └─ sendRequest(balance_req, "admin", handler)
                  │    │         └─ asio::post ──► ...
                  │    └─ _page = 1
                  │
                  ├─ Container::Tab render
                  │    page=1 → DashboardPage hiện ra
                  │
                  └─ User thấy:
                       "Welcome, admin!"
                       "Balance: 999999"
```

**Dữ liệu trên wire cho đăng nhập:**

```
Client → Server:   [0x00 0x00 0x00 0x0E] [0x00] "admin:admin123"
                     └── length = 14 BE ─┘   └type┘ └── payload ──┘

                   Giải thích:
                   0x0E = 14 byte (độ dài của "admin:admin123")
                   0x00 = PacketType::account_auth

Server → Client:   [0x00 0x00 0x00 0x02] [0x00] "OK"
                     └── length = 2 BE ──┘   └type┘

                   0x02 = 2 byte ("O" + "K")
```

---

## 6. Luồng tạo tài khoản (Signup)

Từ màn hình Login, user bấm **Sign Up** → `_page = 2` → Container::Tab hiển thị SignupPage.

```
Thời điểm   Main Thread (FTXUI)        Client Thread (Asio)          Server Thread
────────    ─────────────────────       ─────────────────────         ─────────────

T0          User nhập "newuser", "pass123", "pass123"
              └─ Bấm Create Account
                   │
T1             SignupPage::doCreateAccount()
                  ├─ validate: _password == _confirmPassword?
                  ├─ (skip nếu không match — không gửi request)
                  └─ _client.createAccount("newuser", "pass123", cb)
                       └─ sendRequest(account_create, payload, handler)
                            └─ asio::post([...]) ──►

T2                                                    _pending.push(handler)
                                                      asyncSend(socket,
                                                        account_create,
                                                        "newuser:pass123")
                                                        │
                                                        └─ async_write ──────►
                                                                              │
T3                                                                       Session::doRead()
                                                                              │
                                                                              └─ parse: type=account_create
                                                                                 user="newuser", pass="pass123"
                                                                              └─ _db.createAccount("newuser",
                                                                                   "pass123", cb)
                                                                                   ├─ lock
                                                                                   ├─ queue.push(CreateAccountOp)
                                                                                   ├─ notify
                                                                                   └─ unlock
                                                                              │
T4                                                                       Worker thread:
                                                                             processItem(CreateAccountOp)
                                                                               ├─ _data["newuser"] chưa tồn tại
                                                                               ├─ Tạo record mới:
                                                                               │    balance = 100000
                                                                               │    logFile = "newuser.log"
                                                                               │    isLocked = false
                                                                               └─ asio::post(io, [cb]{ cb(true) })

T5                                     ◄──── asyncRecv() ───────────────── doWrite(account_create, "OK" / "FAIL")
                                           │
T6                                     recvLoop() callback:
                                         ec=0, type=account_create, data="OK" / "FAIL"
                                           │
                                           ├─ pop handler
                                           ├─ handler(true, "OK")
                                           └─ recvLoop() ← chờ tiếp
                                           │
T7                                     handler:
                                         ok=true, resp="OK"
                                           │
                                           └─ screen.Post([&]{
                                                _loading = false
                                                _appUsername = "newuser"
                                                onSuccess()
                                                  _dashboard.doRefresh()
                                                    client.getBalance("newuser",...)
                                                  _page = 1
                                              })
                                              │
T8                  screen.Post chạy:
                     ├─ _loading = false
                     ├─ _appUsername = "newuser"
                     ├─ onSuccess()
                     │    ├─ doRefresh() → getBalance → sendRequest...
                     │    └─ _page = 1
                     ├─ Container::Tab render
                     │    page=1 → DashboardPage hiện ra
                     └─ User thấy:
                          "Welcome, newuser!"
                          "Balance: 100000"
```

**Dữ liệu trên wire cho tạo tài khoản:**

```
Client → Server:   [0x00 0x00 0x00 0x0E] [0x01] "newuser:pass123"
                     └── length = 14 BE ─┘   └type┘ └── payload ────┘

                   Giải thích:
                   0x0E = 14 byte (độ dài của "newuser:pass123")
                   0x01 = PacketType::account_create

Server → Client:   [0x00 0x00 0x00 0x02] [0x01] "OK"
                     └── length = 2 BE ──┘   └type┘
```

**Điểm khác biệt so với luồng đăng nhập:**

| Bước | Login (account_auth) | Signup (account_create) |
|------|---------------------|------------------------|
| Validation | Client: empty check | Client: empty + password match |
| Packet type | `account_auth` (0) | `account_create` (1) |
| Server logic | Kiểm tra tồn tại + password | Tạo record mới (reject nếu đã tồn tại) |
| Response | `"OK"` / `"FAIL"` | `"OK"` / `"FAIL"` |
| Post-signup | Vào Dashboard | Vào Dashboard (tự động) |

---

## Tóm tắt các điểm chính

| # | Nguyên tắc | Mô tả |
|---|------------|-------|
| 1 | **Socket chỉ chạy trên client thread** | Mọi op socket đều qua `asio::post()` — main thread không bao giờ chạm `_socket` |
| 2 | **Callback vượt luồng qua screen.Post()** | Response mạng đến trên client thread, UI cập nhật trên main thread |
| 3 | **Work guard là thiết yếu** | Giữ `io_context` sống khi chưa có handler — nếu không, thread chết ngay |
| 4 | **Recv loop vĩnh viễn** | Client luôn sẵn sàng nhận, không blocking |
| 5 | **FIFO handler queue** | Request và response khớp nhau theo thứ tự — không cần request ID |
| 6 | **Server đơn luồng + worker** | Session chia sẻ một thread, database xử lý trên worker riêng |
| 7 | **std::variant cho work item** | Type-safe dispatch tại compile time, không cần virtual |
