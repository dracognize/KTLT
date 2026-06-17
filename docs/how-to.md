# How-to Guide: Cẩm nang giải quyết vấn đề

Tài liệu này chứa các "công thức" cụ thể cho những tác vụ thường gặp khi làm việc với project KTLT. Mỗi mục là một bài toán + giải pháp từng bước.

---

## 1. Cách thêm một loại Packet mới

Bạn muốn thêm một loại packet mới (ví dụ: `ping` để kiểm tra kết nối)? Làm theo các bước sau.

### Bước 1: Thêm enum vào PacketType.hpp

```cpp
// libs/network/PacketType.hpp
enum class PacketType : u8 {
    account_auth   = 0,
    account_create = 1,
    balance_req    = 2,
    balance_change = 3,
    balance_transfer = 4,
    ping           = 5,  // ← Đã có sẵn
    account_change_password = 6,
    account_toggle = 7,
};
```

### Bước 2: Thêm xử lý trong Server.cpp

```cpp
// server/Server.cpp — trong Session::doRead()
case ping: {
    ServerLog::info(_peer + " ping received");
    doWrite(std::to_underlying(PacketType::ping), "PONG");
    break;
}
```

### Bước 3: Thêm method trong Client

```cpp
// app/client/Client.hpp
void ping(BoolHandler handler);

// app/client/Client.cpp
void Client::ping(BoolHandler handler) {
    sendRequest(PacketType::ping, "",
        [handler = std::move(handler)](bool ok, const std::string &resp) mutable {
            if (handler)
                handler(ok && resp == "PONG");
        });
}
```

### Bước 4: Gọi từ UI

```cpp
// Trong DashboardPage, thêm nút Ping
_client.ping([this](bool ok) {
    _screen.Post([this, ok] {
        _status = ok ? "Ping OK!" : "Ping FAILED";
    });
});
```

---

## 2. Cách thêm một Page mới trong client

Ví dụ: thêm trang **Settings** để đổi mật khẩu.

> **Ví dụ thực tế:** Project đã có sẵn `SignupPage` (`app/pages/SignupPage.hpp/cpp`) — một page được thêm theo đúng pattern này. Bạn có thể dùng nó làm tài liệu tham khảo trực quan.

### Bước 1: Tạo file SettingsPage.hpp

```cpp
// app/pages/SettingsPage.hpp
#pragma once

#include <ftxui/component/component.hpp>
#include <string>

class Client;
struct LoginPage;
namespace ftxui { class ScreenInteractive; }

struct SettingsPage {
    SettingsPage(Client &client,
                 ftxui::ScreenInteractive &screen,
                 const std::string &username,
                 int &page,
                 LoginPage &loginPage);
    ftxui::Component build();

private:
    void doChangePassword();
    void onBack();

    Client &_client;
    ftxui::ScreenInteractive &_screen;
    const std::string &_username;
    int &_page;
    LoginPage &_loginPage;

    std::string _newPassword;
    std::string _status;
};
```

### Bước 2: Tạo SettingsPage.cpp

```cpp
// app/pages/SettingsPage.cpp
#include "app/pages/SettingsPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/LoginPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

SettingsPage::SettingsPage(Client &client, ftxui::ScreenInteractive &screen,
                           const std::string &username, int &page, LoginPage &loginPage)
    : _client(client), _screen(screen), _username(username),
      _page(page), _loginPage(loginPage) {}

void SettingsPage::doChangePassword() {
    if (_newPassword.empty()) {
        _status = "Password cannot be empty";
        return;
    }
    _client.changePassword(_username, _newPassword, [this](bool ok) {
        _screen.Post([this, ok] {
            _status = ok ? "Password changed!" : "Failed";
        });
    });
}

void SettingsPage::onBack() {
    _page = 1;  // Quay về Dashboard
}

ftxui::Component SettingsPage::build() {
    auto passInput = ftxui::Input(&_newPassword, "new password",
        ftxui::InputOption{.password = true, .multiline = false});

    auto changeBtn = ftxui::Button("Change", [this] { doChangePassword(); });
    auto backBtn = ftxui::Button("Back", [this] { onBack(); });

    auto container = ftxui::Container::Vertical({
        passInput,
        ftxui::Container::Horizontal({changeBtn, backBtn}),
    });

    return ftxui::Renderer(container, [this, passInput, changeBtn, backBtn] {
        return ftxui::vbox({
            ftxui::text("Settings") | ftxui::bold | ftxui::center,
            ftxui::separator(),
            ftxui::hbox({ftxui::text(" New Password: "), passInput->Render() | ftxui::flex}),
            ftxui::separator(),
            ftxui::hbox({changeBtn->Render(), backBtn->Render()}) | ftxui::center,
            ftxui::text(_status) | ftxui::center,
        }) | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) | ftxui::center;
    });
}
```

### Bước 3: Đăng ký vào App.cpp

```cpp
// app/App.cpp
#include "app/pages/SettingsPage.hpp"

void App::run() {
    // ... code hiện tại ...

    LoginPage loginPage(client, screen, appUsername, page);
    DashboardPage dashboard(client, screen, appUsername, page, loginPage);
    SettingsPage settings(client, screen, appUsername, page, loginPage);  // ★ NEW
    loginPage.setDashboard(dashboard);

    auto loginComp = loginPage.build();
    auto dashComp  = dashboard.build();
    auto settingsComp = settings.build();  // ★ NEW

    auto container = ftxui::Container::Tab(
        {loginComp, dashComp, settingsComp}, &page  // ★ Thêm settingsComp
    ) | ftxui::border;

    // ...
}
```

> **Lưu ý về page index:** `LoginPage` luôn ở index `0`, `DashboardPage` ở index `1`. Page mới (Settings, Signup...) lấy index tiếp theo (`2`, `3`,...). Giá trị `page` được dùng trong `Container::Tab` để quyết định page nào hiển thị.

### Bước 4: Thêm nút điều hướng

Trong **LoginPage**, thêm nút chuyển đến page mới:

```cpp
// app/pages/LoginPage.cpp — trong build()
auto settingsBtn = ftxui::Button("Settings", [this] {
    _page = 2;  // Chuyển đến Settings page
    _screen.RequestAnimationFrame();
});
```

Hoặc trong **DashboardPage**:

```cpp
// app/pages/DashboardPage.cpp — trong build()
auto settingsBtn = ftxui::Button("Settings", [this] { _page = 2; });
// Thêm vào container
```

> **Xem thực tế:** `LoginPage` có nút `"Sign Up"` set `_page = 2` để chuyển đến `SignupPage`.

---

## 3. Cách debug lỗi kết nối

### 3.1. Kiểm tra server đã chạy chưa

```bash
# Kiểm tra tiến trình server
ps aux | grep server

# Kiểm tra cổng 8080
ss -tlnp | grep 8080
# Output: LISTEN 0 128 0.0.0.0:8080 0.0.0.0:*
```

### 3.2. Đọc log server

Server ghi log ra terminal. Các loại log:

```
[INFO]   — thông tin chung (kết nối, ngắt kết nối)
[WARN]   — cảnh báo (packet sai định dạng)
[AUTH]   — đăng nhập
[CREATE] — tạo tài khoản
[BALANCE] — thay đổi số dư
[TRANSFER] — chuyển tiền
```

### 3.3. Thêm debug raw packet

Tạm thời thêm in hex dump trong `packet::asyncRecv` (`libs/network/Packet.cpp`):

```cpp
auto asyncRecv(...) -> void {
    // ... code hiện tại ...
    asio::async_read(socket, asio::buffer(*header),
        [&socket, header, handler](std::error_code ec, usize /*bytes*/) {
            // ★ THÊM DEBUG:
            std::cerr << "RECV header: ";
            for (auto b : *header)
                std::cerr << std::hex << (int)(unsigned char)b << " ";
            std::cerr << std::dec << "\n";

            // ... phần còn lại ...
        });
}
```

### 3.4. Các lỗi thường gặp

| Lỗi | Nguyên nhân | Giải pháp |
|-----|-------------|-----------|
| `Connection refused` | Server chưa chạy | `./build/server` |
| `Failed to open data.db` | Chưa tạo dữ liệu | `./build/generate_test_data` |
| `Authentication failed` | Sai user/pass | Thử `admin`/`admin123` |
| Client không kết nối được | Firewall | Kiểm tra `ufw status` |

---

## 4. Cách thêm trường dữ liệu mới vào database

Ví dụ: thêm trường `email` cho mỗi user.

### Bước 1: Sửa struct Record

```cpp
// server/Database.hpp
#pragma pack(push, 1)
struct Record {
    char username[24];   // Tối đa 23 ký tự + null terminator
    char password[24];   // Tối đa 23 ký tự + null terminator
    u64 balance;
    char logFile[32];
    char email[32];  // ★ THÊM
    b8 isLocked;
};
#pragma pack(pop)

struct DependElementRecord {
    char password[24];   // Tối đa 23 ký tự + null terminator
    u64 balance;
    char logFile[32];
    char email[32];  // ★ THÊM
    b8 isLocked;
};
```

### Bước 2: Cập nhật load()

```cpp
// server/Database.cpp — trong load()
auto &entry = _data[username];
// ... code cũ ...
copyFrom(asString(record.email), entry.email);  // ★ THÊM
entry.isLocked = record.isLocked;
```

### Bước 3: Cập nhật save()

```cpp
// server/Database.cpp — trong save()
// ... code cũ ...
std::memcpy(record.logFile, entry.logFile, sizeof(record.logFile));
std::memcpy(record.email, entry.email, sizeof(record.email));  // ★ THÊM
record.isLocked = entry.isLocked;
```

### Bước 4: Cập nhật processItem (khi tạo tài khoản)

```cpp
// Trong CreateAccountOp:
auto &entry = _data[op.username];
copyFrom(op.password, entry.password);
entry.balance = DEFAULT_BALANCE;
copyFrom("", entry.email);  // ★ THÊM: email mặc định rỗng
```

### Bước 5: Cập nhật generate_test_data.cpp

```cpp
void writeRecord(std::ofstream &f, const char *user, const char *pass,
                 u64 balance, bool locked) {
    Record r{};
    std::strncpy(r.username, user, sizeof(r.username) - 1);
    std::strncpy(r.password, pass, sizeof(r.password) - 1);
    r.balance = toNetwork(balance);
    std::strncpy(r.email, "", sizeof(r.email) - 1);  // ★ THÊM
    r.isLocked = locked ? 1 : 0;
    f.write(reinterpret_cast<const char *>(&r), sizeof(r));
}
```

---

## 5. Cách viết test cho base library

Hiện tại project chưa có unit test framework. Bạn có thể viết test đơn giản bằng `assert()`.

### 5.1. Test Vector

```cpp
// tests/test_vector.cpp
#include "libs/base/vector.hpp"
#include <cassert>

auto main() -> int {
    // Test push_back
    base::Vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    assert(v.size() == 3);
    assert(v[0] == 10);
    assert(v[1] == 20);
    assert(v[2] == 30);

    // Test pop_back
    v.pop_back();
    assert(v.size() == 2);

    // Test insert
    v.insert(v.begin() + 1, 15);
    assert(v[1] == 15);
    assert(v[2] == 20);

    // Test erase
    v.erase(v.begin());
    assert(v[0] == 15);

    // Test clear
    v.clear();
    assert(v.empty());

    return 0;
}
```

### 5.2. Test String

```cpp
// tests/test_string.cpp
#include "libs/base/string.hpp"
#include <cassert>

auto main() -> int {
    // Test SSO (chuỗi ngắn)
    base::String s1("hello");
    assert(s1.size() == 5);
    assert(s1 == "hello");

    // Test long string (vượt SSO_Capacity ~22 ký tự)
    base::String s2("this is a very long string that exceeds SSO capacity");
    assert(s2.size() > 22);

    // Test push_back
    s1.push_back('!');
    assert(s1 == "hello!");

    // Test pop_back
    s1.pop_back();
    assert(s1 == "hello");

    return 0;
}
```

### 5.3. Build và chạy test

Thêm vào `CMakeLists.txt`:

```cmake
add_executable(test_vector tests/test_vector.cpp)
target_include_directories(test_vector PUBLIC ${CMAKE_SOURCE_DIR})

add_executable(test_string tests/test_string.cpp)
target_include_directories(test_string PUBLIC ${CMAKE_SOURCE_DIR})
```

```bash
cmake --build build
./build/test_vector
./build/test_string
```

---

## 6. Cách xử lý lỗi thường gặp

### 6.1. Lỗi build: `fatal error: asio.hpp: No such file or directory`

```bash
sudo apt install libasio-dev
```

### 6.2. Lỗi build: `fatal error: ftxui/component/component.hpp: No such file or directory`

```bash
sudo apt install libftxui-dev
# Hoặc build từ source:
# git clone https://github.com/ArthurSonzogni/FTXUI
# cd FTXUI && cmake -B build && cmake --build build && sudo cmake --install build
```

### 6.3. Lỗi runtime: `data.db` không tồn tại

```bash
./build/generate_test_data
```

### 6.4. Lỗi runtime: `Address already in use`

Cổng 8080 đã được dùng bởi server khác:

```bash
# Kill tiến trình đang giữ cổng
sudo fuser -k 8080/tcp
# Hoặc đổi cổng trong code (Server.hpp)
Server server(9090);  // Dùng cổng khác
```

### 6.5. Lỗi: Client bị treo, không nhận response

Nguyên nhân thường gặp: `recvLoop()` bị dừng do lỗi socket.

Check bằng cách thêm log:

```cpp
// app/client/Client.cpp — trong recvLoop()
void Client::recvLoop() {
    packet::asyncRecv(_socket, [this](std::error_code ec, PacketType, std::string data) {
        if (ec) {
            std::cerr << "recvLoop error: " << ec.message() << "\n";  // Debug
            _connected = false;
            // ... drain queue ...
            return;
        }
        // ...
    });
}
```

---

## 7. Cách thêm một lệnh ghi log mới

Ví dụ: thêm log cho thao tác "đổi mật khẩu".

### Bước 1: Thêm method trong ServerLog

```cpp
// server/ServerLog.hpp
struct ServerLog {
    // ... các method hiện tại ...
    static void password(const std::string &msg);
};
// server/ServerLog.cpp đã có sẵn
```

### Bước 2: Gọi trong Server.cpp

```cpp
// Trong Session::doRead(), case account_change_password:
_db.changePassword(user, pass, [this, self, user] {
    ServerLog::password(_peer + " user '" + user + "' password changed");
    doWrite(std::to_underlying(PacketType::account_change_password), "OK");
});
```

---

## 8. Cách thêm nút bấm mới trong Dashboard

```cpp
// app/pages/DashboardPage.cpp — trong build()
auto pingBtn = ftxui::Button("Ping Server", [this] {
    _client.ping([this](bool ok) {
        _screen.Post([this, ok] {
            _status = ok ? "Server alive!" : "Server dead!";
        });
    });
});

auto container = ftxui::Container::Horizontal({
    refreshBtn,
    logoutBtn,
    pingBtn,  // ★ THÊM
});
```

Nhớ thêm `ping()` vào class Client trước (xem mục 1).
