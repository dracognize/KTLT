# Tutorial: Làm quen với Project KTLT

Bài hướng dẫn này dành cho **fresher** (người mới học lập trình). Bạn sẽ được dẫn dắt từng bước: từ chưa biết gì → build được project → chạy thử → hiểu luồng hoạt động → tự tay sửa code.

---

## 1. Giới thiệu project KTLT

### 1.1. KTLT là gì?

KTLT là một **project client-server quản lý tài khoản ngân hàng** hoàn chỉnh, viết bằng C++. Nó có:

- **Server** (thư mục `server/`): chương trình chạy nền, quản lý tài khoản người dùng, số dư, xử lý các yêu cầu từ client.
- **Client** (thư mục `app/`): chương trình giao diện terminal cho người dùng đăng nhập, xem số dư, chuyển tiền.
- **Thư viện mạng** (`libs/network/`): xử lý đóng gói dữ liệu gửi/nhận qua TCP.
- **Thư viện cấu trúc dữ liệu** (`libs/base/`): tự xây dựng lại các container giống STL (Vector, String, Map, Set...) như một bài tập học thuật.

### 1.2. Công nghệ sử dụng

| Công nghệ | Vai trò |
|-----------|---------|
| **C++26** | Ngôn ngữ lập trình |
| **CMake 4.2+** | Hệ thống build |
| **Ninja** | Trình build (nhanh hơn Make) |
| **Asio** | Thư viện mạng bất đồng bộ (socket TCP) |
| **FTXUI** | Thư viện giao diện terminal (gõ phím, ô input, nút bấm) |

### 1.3. Cấu trúc thư mục

```
KTLT/
├── CMakeLists.txt          # Cấu hình build
├── data.db                 # File database (nhị phân)
├── build/                  # Thư mục build
├── docs/                   # Tài liệu
├── libs/
│   ├── base/               # Thư viện cấu trúc dữ liệu tự xây
│   └── network/            # Thư viện giao thức mạng
├── server/                 # Mã nguồn server
├── app/                    # Mã nguồn client (giao diện)
└── tests/                  # Tiện ích tạo dữ liệu mẫu
```

---

## 2. Chuẩn bị môi trường

### 2.1. Cài đặt công cụ

Bạn cần:

```
- Trình biên dịch C++26 (g++-14+, clang-18+)
- CMake >= 4.2
- Ninja build system
- Git
```

Trên Ubuntu/Debian:

```bash
sudo apt update
sudo apt install g++-14 cmake ninja-build git
```

### 2.2. Cài đặt thư viện

**Asio** (thư viện mạng):

```bash
sudo apt install libasio-dev
```

**FTXUI** (thư viện giao diện terminal):

```bash
sudo apt install libftxui-dev
```

> Nếu apt không có FTXUI, bạn có thể tự build từ source: https://github.com/ArthurSonzogni/FTXUI

### 2.3. Clone project

```bash
git clone <đường-dẫn-repo> KTLT
cd KTLT
```

---

## 3. Build project

### 3.1. CMake configure + build

```bash
cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-14
cmake --build build
```

Giải thích từng bước:

- `-B build`: tạo thư mục build
- `-G Ninja`: dùng Ninja làm trình build
- `cmake --build build`: thực thi build

Sau khi build thành công, bạn sẽ thấy các file trong `build/`:

Ứng dụng client có **3 page** (trang) chính:
- `page = 0` — Login (đăng nhập)
- `page = 1` — Dashboard (bảng điều khiển)
- `page = 2` — Signup (tạo tài khoản)

### 3.2. Các executable tạo ra

| File | Mô tả |
|------|-------|
| `build/server` | Server (chạy trên cổng 8080) |
| `build/app` | Client terminal UI |
| `build/generate_test_data` | Công cụ tạo dữ liệu mẫu |

**Sơ đồ CMake targets:**

```
┌──────────────────────────────────────────────┐
│                 CMake Project                │
├──────────────────────────────────────────────┤
│  network (static library)                    │
│  └─ libs/network/Packet.cpp                  │
│     └─ phụ thuộc: Asio, Threads              │
│                                              │
│  server (executable) ──── link ──► network   │
│  ├─ server/main.cpp                          │
│  ├─ server/Server.cpp                        │
│  ├─ server/Database.cpp                      │
│  └─ server/ServerLog.cpp                     │
│                                              │
│  app (executable) ──── link ──► network      │
│  ├─ app/main.cpp               FTXUI         │
│  ├─ app/App.cpp                              │
│  ├─ app/client/Client.cpp                    │
│  ├─ app/pages/LoginPage.cpp                  │
│  └─ app/pages/DashboardPage.cpp              │
│                                              │
│  generate_test_data (executable)             │
│  └─ tests/generate_test_data.cpp             │
└──────────────────────────────────────────────┘
```

---

## 4. Chạy thử — lần đầu tiên

### 4.1. Tạo dữ liệu mẫu

```bash
./build/generate_test_data
```

Lệnh này tạo file `data.db` với 3 tài khoản:

| Username | Password | Số dư | Trạng thái |
|----------|----------|-------|------------|
| `admin` | `admin123` | 999,999 | Hoạt động |
| `user1` | `password1` | 50,000 | Hoạt động |
| `locked` | `lockedpw` | 1,000 | **Bị khoá** |

### 4.2. Chạy server

Mở **terminal thứ nhất**:

```bash
./build/server
```

Bạn sẽ thấy output:

```
[INFO] [2026-06-17 10:00:00] listening on port 8080
```

Server đang lắng nghe kết nối TCP ở cổng 8080. Nó sẽ chạy mãi cho đến khi bạn tắt bằng `Ctrl+C`.

### 4.3. Chạy client

Mở **terminal thứ hai**:

```bash
./build/app
```

Một giao diện terminal hiện ra với màn hình đăng nhập.

### 4.4. Đăng nhập với tài khoản mẫu

```
┌────────────────────────┐
│         Login          │
├────────────────────────┤
│  Username: admin       │
│  Password: *******     │
├────────────────────────┤
│        [Login]         │
│       [Sign Up]        │   ← ★ Nút chuyển sang trang tạo tài khoản
└────────────────────────┘
```

- Nhập `admin` / `admin123`
- Bấm `Login`

Sau khi đăng nhập thành công, bạn thấy Dashboard:

```
┌────────────────────────┐
│       Dashboard        │
├────────────────────────┤
│  Welcome, admin!       │
│  Balance: 999999       │
├────────────────────────┤
│  [Refresh] [Logout]    │
└────────────────────────┘
```

Bấm `Escape` để thoát chương trình.

### 4.5. Tạo tài khoản mới qua SignupPage

Từ màn hình Login, bấm **Sign Up** để đến trang tạo tài khoản:

```
┌────────────────────────────┐
│       Create Account       │
├────────────────────────────┤
│  Username: [newuser      ] │
│  Password: [*******      ] │
│  Confirm:  [*******      ] │
├────────────────────────────┤
│  [Create Account] [Back]   │
└────────────────────────────┘
```

- Nhập `newuser` / `pass123` / `pass123`
- Bấm `Create Account`
- Nếu thành công: tự động chuyển sang Dashboard

---

## 5. Khám phá code — từng bước một

Bây giờ chúng ta sẽ đọc code để hiểu **luồng đăng nhập** hoạt động thế nào.

### 5.1. Điểm vào: main()

**Server** (`server/main.cpp`):

```cpp
#include "server/Server.hpp"

auto main() -> int {
    Server server(8080);   // Tạo server, lắng nghe cổng 8080
    server.run();          // Chạy vòng lặp sự kiện
}
```

Chỉ 2 dòng! `Server` constructor bắt đầu lắng nghe, `run()` chạy vô tận.

**Client** (`app/main.cpp`):

```cpp
#include "app/App.hpp"

auto main() -> int {
    App app;
    app.run();
}
```

Ba page chính: `LoginPage` (đăng nhập), `DashboardPage` (bảng điều khiển), `SignupPage` (tạo tài khoản mới). Người dùng có thể chuyển qua lại bằng nút bấm — không cần login để vào SignupPage.

### 5.2. Luồng đăng nhập (từng bước)

Khi bạn gõ `admin` + `admin123` và bấm Login, đây là những gì xảy ra:

**Bước 1 — Trong App::run()** (`app/App.cpp`):

```cpp
Client client("127.0.0.1", 8080);  // Tạo client kết nối localhost
std::thread clientThread([&] { client.run(); });  // Chạy vòng lặp mạng trên luồng riêng
client.connect([&](bool ok) { ... });  // Kết nối đến server

LoginPage loginPage(client, screen, appUsername, page);
DashboardPage dashboard(client, screen, appUsername, page, loginPage);
SignupPage signupPage(client, screen, appUsername, page);   // ★ Trang tạo tài khoản

loginPage.setDashboard(dashboard);
signupPage.setDashboard(dashboard);                         // ★ Liên kết dashboard

auto container = ftxui::Container::Tab(
    {loginComp, dashComp, signupComp}, &page                 // ★ 3 page!
) | ftxui::border;

screen.Loop(container);  // Vòng lặp giao diện (main thread)
```

**Bước 2 — LoginPage::doLogin()** (`app/pages/LoginPage.cpp`):

```cpp
void LoginPage::doLogin() {
    _loading = true;
    _client.authenticate(_username, _password, [this](bool ok) {
        // Callback này chạy trên client thread! 
        // screen.Post() đưa nó về main thread
        _screen.Post([this, ok] {
            _loading = false;
            if (ok) {
                _appUsername = _username;
                onLogin();  // Chuyển sang Dashboard
            }
        });
    });
}
```

**Bước 3 — Client::authenticate()** (`app/client/Client.cpp`):

```cpp
void Client::authenticate(const std::string &username,
                          const std::string &password,
                          BoolHandler handler) {
    auto payload = username + ":" + password;  // "admin:admin123"
    sendRequest(PacketType::account_auth, payload, ...);
}
```

**Bước 4 — Client::sendRequest()** (`app/client/Client.cpp`):

```cpp
void Client::sendRequest(PacketType type, const std::string &payload, Handler handler) {
    // Post công việc sang client thread
    asio::post(_io, [this, type, payload, handler = std::move(handler)]() mutable {
        _pending.push(std::move(handler));     // Xếp handler vào queue
        packet::asyncSend(_socket, type, payload, ...);  // Gửi packet
    });
}
```

**Bước 5 — packet::asyncSend()** (`libs/network/Packet.cpp`):

```cpp
auto asyncSend(asio::ip::tcp::socket& socket, PacketType type,
               const std::string& data, ...) -> void {
    // Header: 4 byte length (big-endian) + 1 byte type
    auto header = std::make_shared<std::array<char, HeaderSize>>();
    auto len = toNetwork(static_cast<u32>(data.size()));  // Chuyển sang big-endian
    std::memcpy(header->data(), &len, LengthSize);
    (*header)[LengthSize] = static_cast<char>(type);

    auto body = std::make_shared<std::string>(data);

    // Gửi header + body cùng lúc
    asio::async_write(socket, {asio::buffer(header), asio::buffer(*body)}, ...);
}
```

**Sơ đồ dữ liệu trên wire:**

```
Client gửi:   [0x00 0x00 0x00 0x0E] [0x00] "admin:admin123"
                └─ length = 14 (BE) ─┘  └type┘ └── payload ──┘

Server trả:   [0x00 0x00 0x00 0x02] [0x00] "OK"
```

**Bước 6 — Session::doRead() trên Server** (`server/Server.cpp`):

Server nhận packet, parse type, switch-case đến `account_auth`:

```cpp
case account_auth: {
    auto delim = data.find(':');                     // Tìm dấu ':'
    auto user = data.substr(0, delim);               // "admin"
    auto pass = data.substr(delim + 1);              // "admin123"
    _db.authenticate(user, pass, [this, self](bool ok) {
        doWrite(std::to_underlying(PacketType::account_auth),
                ok ? "OK" : "FAIL");                  // Gửi response
    });
    break;
}
```

**Bước 7 — DbManager xử lý** (`server/Database.cpp`):

```cpp
// Trong processItem, với AuthOp:
auto it = _data.find(op.username);
if (it == _data.end() || it->second.isLocked) {
    // Tài khoản không tồn tại hoặc bị khoá → fail
    post([cb = ...] { cb(false); });
    return;
}
auto ok = asString(it->second.password) == op.password;
post([cb = ..., ok] { cb(ok); });  // Post kết quả về io_context
```

**Bước 8 — Response về client** (`app/client/Client.cpp`):

```cpp
void Client::recvLoop() {
    packet::asyncRecv(_socket, [this](std::error_code ec, PacketType, std::string data) {
        auto handler = std::move(_pending.front());
        _pending.pop();
        if (handler) handler(true, std::move(data));
        recvLoop();  // Lặp lại: chờ packet tiếp theo
    });
}
```

Handler được pop khỏi queue, gọi `handler(true, "OK")`, kích hoạt callback trong LoginPage.

**Luồng tổng thể qua 3 thread:**

```
main thread (FTXUI)       client thread (Asio)          server thread (Asio)
─────────────────────     ──────────────────────        ─────────────────────
                                                                            
App::run()                                                                  
  │                                                                         
  ├─ Create Client                                                         
  ├─ Thread: client.run() ──► _io.run()                                    
  │                                                                         
  ├─ screen.Loop()                                                         
  │   └─ User bấm Login                                                    
  │        └─ doLogin()                                                    
  │             └─ authenticate()                                           
  │                  └─ sendRequest()                                       
  │                       └─ asio::post() ──► _pending.push(handler)       
  │                                             asyncSend() ──────────────► doRead()
  │                                                                          └─ DbManager::authenticate()
  │                                                                               └─ worker thread xử lý
  │                                                                                    └─ post callback
  │                                                                          doWrite("OK")
  │                                    ◄──── asyncRecv() ─────────────────
  │                                         recvLoop()
  │                                           └─ pop handler
  │                                              handler(true, "OK")
  │                                                └─ screen.Post() ──► onLogin()
  │                                                                        └─ page = 1
  │                                                                           (hiện Dashboard)
```

### 5.3. Luồng tạo tài khoản (Signup)

Quy trình tạo tài khoản qua SignupPage tương tự đăng nhập, nhưng thêm bước **xác nhận mật khẩu** ở phía client.

**Bước 1 — SignupPage::doCreateAccount()** (`app/pages/SignupPage.cpp`):

```cpp
void SignupPage::doCreateAccount() {
    // Validate: không để trống, password khớp confirm
    if (_username.empty() || _password.empty() || _confirmPassword.empty()) {
        _status = "Please fill in all fields";
        return;
    }
    if (_password != _confirmPassword) {
        _status = "Passwords do not match";
        return;
    }
    _loading = true;
    _client.createAccount(_username, _password, [this](bool ok) {
        _screen.Post([this, ok] {
            _loading = false;
            if (ok) {
                _appUsername = _username;
                onSuccess();   // → doRefresh() + page = 1
            } else {
                _status = "Account creation failed (username may exist)";
            }
        });
    });
}
```

**Bước 2 — Client::createAccount()** (`app/client/Client.cpp`):

```cpp
void Client::createAccount(const std::string &username,
                           const std::string &password,
                           BoolHandler handler) {
    auto payload = username + ":" + password;  // "newuser:pass123"
    sendRequest(PacketType::account_create, payload, ...);
}
```

**Bước 3 — Server xử lý account_create** (`server/Server.cpp`):

```cpp
case account_create: {
    auto delim = data.find(':');                     // Tìm dấu ':'
    auto user = data.substr(0, delim);               // "newuser"
    auto pass = data.substr(delim + 1);              // "pass123"
    _db.createAccount(user, pass, [this, self](bool created) {
        doWrite(std::to_underlying(PacketType::account_create), created ? "OK" : "FAIL");
    });
    break;
}
```

**Bước 4 — DbManager xử lý CreateAccountOp** (`server/Database.cpp`):

Server luôn trả `"OK"` (idempotent — nếu user đã tồn tại thì không làm gì). Tài khoản mới được tạo với:
- `balance` = `DEFAULT_BALANCE` (100,000)
- `logFile` = `username + ".log"`
- `isLocked` = `false`

**Sơ đồ dữ liệu trên wire:**

```
Client gửi:   [0x00 0x00 0x00 0x0E] [0x01] "newuser:pass123"
                └─ length = 14 (BE) ─┘  └type┘ └─── payload ────┘
                type=1 = account_create

Server trả:   [0x00 0x00 0x00 0x02] [0x01] "OK"
```

**Luồng qua 3 thread:**

```
main thread (FTXUI)       client thread (Asio)          server thread (Asio)
─────────────────────     ──────────────────────        ─────────────────────

screen.Loop()
  └─ User bấm Sign Up
       └─ page = 2 ──► hiện SignupPage
       └─ User nhập newuser/pass123
       └─ Bấm Create Account
            └─ doCreateAccount()
                 ├─ validate (client-side)
                 └─ client.createAccount()
                      └─ sendRequest()
                           └─ asio::post() ──► _pending.push(handler)
                                                  asyncSend(account_create)
                                                       ──────────────────► doRead()
                                                                           └─ createAccount()
                                                                                └─ enqueue → worker
                                                                                     └─ processItem()
                                                                                          └─ tạo record mới
                                                                                     └─ asio::post(cb)
                                                                           doWrite("OK")
                                             ◄──── asyncRecv() ────────────
                                                  recvLoop()
                                                    └─ pop handler
                                                       handler(true, "OK")
                                                         └─ screen.Post() ──► onSuccess()
                                                                                ├─ doRefresh()
                                                                                └─ page = 1
                                                                                   (hiện Dashboard)
```

### 5.4. Vòng lặp nhận (recv loop)

Một điểm quan trọng: client không gửi request rồi chờ response. Thay vào đó, nó có một **vòng lặp nhận vĩnh viễn** (`recvLoop`).

```
sendRequest()
  ├─ push handler vào _pending queue
  └─ asyncSend() — gửi đi, KHÔNG đợi

recvLoop()  ← chạy song song, chờ packet từ server
  └─ có response
       ├─ pop handler từ queue
       └─ gọi handler(response)
```

Cơ chế này đảm bảo:
- Luôn sẵn sàng nhận bất kỳ lúc nào
- Handler được gọi đúng thứ tự (FIFO queue)
- Nếu mất kết nối, toàn bộ handler được drain với lỗi

### 5.5. Database worker thread

Server không xử lý database ngay trên luồng chính. Nó dùng một **worker thread riêng**:

```
Luồng chính                    Worker thread
────────────                   ─────────────
DbManager::authenticate()
  ├─ lock mutex
  ├─ queue.push(AuthOp)
  ├─ notify_one()
  └─ unlock                  cv.wait() thức dậy
                               ├─ pop queue
                               ├─ processItem(AuthOp)
                               │   └─ std::visit([...], item)
                               │       └─ tìm user, so sánh password
                               └─ asio::post(io, callback) → đưa kết quả về main thread
```

Worker thread giúp thao tác I/O file (đọc/ghi `data.db`, ghi log) không block luồng xử lý chính của server.

---

## 6. Thử tự tay sửa code

### 6.1. Thêm log khi đăng nhập thành công

Mở `app/pages/LoginPage.cpp`, tìm hàm `doLogin()`:

```cpp
void LoginPage::doLogin() {
    // ... code hiện tại ...
    _client.authenticate(_username, _password, [this](bool ok) {
        _screen.Post([this, ok] {
            _loading = false;
            if (ok) {
                // ★ THÊM DÒNG NÀY:
                _status = "Login successful! Welcome " + _username + "!";
                // ... phần còn lại ...
            }
        });
    });
}
```

Build lại (`cmake --build build`) và chạy thử.

### 6.2. Thay đổi số dư mặc định

Mở `server/Database.hpp`, tìm dòng:

```cpp
static constexpr u64 DEFAULT_BALANCE = 100'000;
```

Đổi thành:

```cpp
static constexpr u64 DEFAULT_BALANCE = 500'000;
```

Build lại server, tạo lại dữ liệu (`./build/generate_test_data`), và chạy thử. Tài khoản mới sẽ có số dư 500,000.

### 6.3. Thêm trạng thái "đang tải" khi Refresh

Mở `app/pages/DashboardPage.cpp`, tìm `doRefresh()`:

```cpp
void DashboardPage::doRefresh() {
    _balanceStr = "Loading...";  // ← Đã có sẵn!
    _status = "";                // ← Xoá status cũ
    _client.getBalance(_username, [this](std::optional<u64> balance) { ... });
}
```

Bạn thấy dòng `_balanceStr = "Loading..."` — đó là cách cho người dùng biết đang có request mạng.

### 6.4. Thêm kiểm tra độ dài username/password trong SignupPage

Database dùng `char[24]` cho username và password — tối đa 23 ký tự mỗi field. Mở `app/pages/SignupPage.cpp` và thêm validation:

```cpp
void SignupPage::doCreateAccount() {
    // ... code hiện tại ...
    if (_password != _confirmPassword) {
        _status = "Passwords do not match";
        return;
    }
    // ★ GIỚI HẠN ĐỘ DÀI (do Record dùng char[24] trong DB):
    if (_username.size() > 23) {
        _status = "Username must be at most 23 characters";
        return;
    }
    if (_password.size() > 23) {
        _status = "Password must be at most 23 characters";
        return;
    }
    // ... code hiện tại ...
}

Build lại và chạy thử: nhập username dài hơn 23 ký tự, bạn sẽ thấy thông báo lỗi ngay lập tức **mà không cần gửi request mạng**.

---

## Tổng kết

Qua bài tutorial này, bạn đã:

1. Biết cách build và chạy project KTLT
2. Hiểu luồng đăng nhập và **tạo tài khoản** từ UI → network → server → database → response
3. Biết cơ chế recv loop, worker thread, và giao thức packet
4. Tự tay sửa được code và thấy kết quả

Hãy tiếp tục với các tài liệu khác:
- **`how-to.md`**: Các "công thức" mở rộng project
- **`reference.md`**: Tra cứu API từng class/function
- **`explanation.md`**: Giải thích sâu về kiến trúc và thiết kế
