# Giao thức Socket Server

> Tài liệu này định nghĩa giao thức truyền thông giữa client và server qua TCP — định dạng packet, các loại thao tác, payload format, và xử lý lỗi.

---

## Mục lục

1. [Tổng quan giao thức](#1-tổng-quan-giao-thức)
2. [Định dạng Packet](#2-định-dạng-packet)
   - 2.1. [Header (5 byte)](#21-header-5-byte)
   - 2.2. [Byte Order](#22-byte-order)
   - 2.3. [Ví dụ Packet trên Wire](#23-ví-dụ-packet-trên-wire)
3. [Các loại Packet](#3-các-loại-packet)
   - 3.1. [Bảng tổng hợp](#31-bảng-tổng-hợp)
   - 3.2. [account_auth (0) — Xác thực](#32-account_auth-0--xác-thực)
   - 3.3. [account_create (1) — Tạo tài khoản](#33-account_create-1--tạo-tài-khoản)
   - 3.4. [balance_req (2) — Lấy số dư](#34-balance_req-2--lấy-số-dư)
   - 3.5. [balance_change (3) — Thay đổi số dư](#35-balance_change-3--thay-đổi-số-dư)
   - 3.6. [balance_transfer (4) — Chuyển khoản](#36-balance_transfer-4--chuyển-khoản)
   - 3.7. [ping (5) — Kiểm tra kết nối](#37-ping-5--kiểm-tra-kết-nối)
   - 3.8. [account_change_password (6) — Đổi mật khẩu](#38-account_change_password-6--đổi-mật-khẩu)
   - 3.9. [account_toggle (7) — Khoá/Mở khoá](#39-account_toggle-7--khóamở-khoá)
4. [Định dạng Payload & Delimiter](#4-định-dạng-payload--delimiter)
5. [Xử lý lỗi](#5-xử-lý-lỗi)
6. [Ghi chú triển khai](#6-ghi-chú-triển-khai)

---

## 1. Tổng quan giao thức

```
┌─────────────────────────────────────────────────────────────┐
│                     TCP Connection                          │
│                     ───────────────                         │
│                                                             │
│  Client                          Server                     │
│  ──────                          ──────                     │
│    │                                │                       │
│    ├───── [Packet: Request] ──────► │                       │
│    │                                ├─ Xử lý (DbManager)   │
│    │◄──── [Packet: Response] ────── │                       │
│    │                                │                       │
│    ├───── [Packet: Request] ──────► │                       │
│    │                                ├─ Xử lý               │
│    │◄──── [Packet: Response] ────── │                       │
│    │                                │                       │
└─────────────────────────────────────────────────────────────┘
```

- Mỗi request tương ứng một response (trừ trường hợp mất kết nối)
- Giao thức dạng **half-duplex**: client gửi, server trả, client gửi tiếp...
- Client dùng **recv loop** để bắt response bất kỳ lúc nào (xem `MECHANICS.md`)
- Tất cả số đa byte đều ở **big-endian** (network byte order)

---

## 2. Định dạng Packet

### 2.1. Header (5 byte)

```
Byte:     0          1          2          3          4
       ┌──────────┬──────────┬──────────┬──────────┬──────────┐
       │      Payload Length (BE u32)    │   Type   │
       │         (4 byte)                │  (1 byte)│
       └──────────┴──────────┴──────────┴──────────┴──────────┘
```

| Trường | Kích thước | Kiểu | Mô tả |
|--------|-----------|------|-------|
| `length` | 4 byte | `u32` big-endian | Kích thước payload tính bằng byte. Tối đa 2^32 - 1 (~4GB) |
| `type` | 1 byte | `u8` (PacketType) | Loại packet, xem bảng ở mục 3 |

Payload theo sau header, độ dài chính xác bằng `length` byte.

```cpp
// libs/network/Packet.hpp
inline constexpr usize HeaderSize = 5;   // 4 + 1
inline constexpr usize LengthSize = 4;
inline constexpr usize TypeSize   = 1;
```

### 2.2. Byte Order

```cpp
// libs/network/Packet.cpp
auto toNetwork(u32 val) noexcept -> u32 {
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(val);  // Đảo byte trên x86/ARM
    }
    return val;  // Giữ nguyên nếu đã là big-endian
}
```

**Ví dụ:** Giá trị `14` (0x0000000E)

```
Big-endian (trên wire):   [0x00 0x00 0x00 0x0E]
Little-endian (x86 RAM):  [0x0E 0x00 0x00 0x00]
```

### 2.3. Ví dụ Packet trên Wire

**Request — Đăng nhập "admin:admin123":**

```
Byte 0-3 (length):  0x00 0x00 0x00 0x0E    → 14 byte (độ dài "admin:admin123")
Byte 4 (type):      0x00                    → PacketType::account_auth
Byte 5-18 (payload): "admin:admin123"       → UTF-8, 14 byte

Toàn bộ packet: [00 00 00 0E] [00] [61 64 6D 69 6E 3A 61 64 6D 69 6E 31 32 33]
                              ↑     └─ "a"  "d"  "m"  "i"  "n"  ":"  "a"  "d"  "m"  "i"  "n"  "1"  "2"  "3"
                              type=0 (account_auth)
```

**Response — Thành công:**

```
Byte 0-3 (length):  0x00 0x00 0x00 0x02    → 2 byte ("OK")
Byte 4 (type):      0x00                    → PacketType::account_auth
Byte 5-6 (payload): "OK"

Toàn bộ packet: [00 00 00 02] [00] [4F 4B]
                                    └─ "O"  "K"
```

---

## 3. Các loại Packet

### 3.1. Bảng tổng hợp

| ID | Tên enum | Request Payload | Response Payload | Ghi chú |
|----|----------|----------------|-----------------|---------|
| 0 | `account_auth` | `username:password` | `"OK"` / `"FAIL"` | — |
| 1 | `account_create` | `username:password` | `"OK"` / `"FAIL"` | Reject nếu username tồn tại |
| 2 | `balance_req` | `username` | Số thập phân (VD: `"100000"`) | Trả `"0"` nếu user ko tồn tại |
| 3 | `balance_change` | `username:change` | `"OK"` / `"FAIL"` | `change` là số nguyên có dấu |
| 4 | `balance_transfer` | `sender:recipient:amount` | `"OK"` / `"FAIL"` | `amount` là số không dấu |
| 5 | `ping` | *(rỗng)* | `"PONG"` | Chưa dùng |
| 6 | `account_change_password` | `username:newpassword` | `"OK"` | Idempotent |
| 7 | `account_toggle` | `username` | `"OK"` | Đảo trạng thái khoá |

> **Lưu ý:** `balance_req` dùng cùng type (2) cho cả request và response. Client dùng **recv loop** và **FIFO queue** để ghép mà không cần phân biệt.

### 3.2. account_auth (0) — Xác thực

**Request:**
```
payload = username + ":" + password
VD: "admin:admin123"
```

**Response:**
```
"OK"   — username tồn tại, không bị khoá, password đúng
"FAIL" — username không tồn tại, hoặc bị khoá, hoặc sai password
```

**Logic server:**
```
// server/Database.cpp — AuthOp
if (user not found)           → FAIL
if (user is locked)           → FAIL (kể cả password đúng)
if (password == stored_pass)  → OK
else                          → FAIL
```

**Tài khoản mẫu để test:**
```
admin  / admin123  → OK
user1  / password1 → OK
locked / lockedpw  → FAIL (bị khoá)
admin  / wrongpass → FAIL
```

### 3.3. account_create (1) — Tạo tài khoản

**Request:**
```
payload = username + ":" + password
VD: "newuser:pass123"
```

**Response:**
```
"OK"   — tài khoản mới đã được tạo
"FAIL" — username đã tồn tại
```

**Hành vi:**
- Nếu username chưa tồn tại → tạo mới với:
  - `balance` = `100'000`
  - `logFile` = `username + ".log"`
  - `isLocked` = `false`
- Nếu username đã tồn tại → không làm gì, trả `"FAIL"`

### 3.4. balance_req (2) — Lấy số dư

**Request:**
```
payload = username
VD: "admin"
```

**Response:**
```
Chuỗi thập phân biểu diễn số dư.
VD: "999999"  — số dư hiện tại
     "0"      — nếu username không tồn tại
```

### 3.5. balance_change (3) — Thay đổi số dư

**Request:**
```
payload = username + ":" + change
VD: "admin:-5000"   (rút 5000)
    "admin:+10000"  (nạp 10000)
    "admin:10000"   (nạp 10000, dấu + không bắt buộc)
```

`change` là số nguyên có dấu (`i64`). Dùng `std::stoll` để parse.

**Response:**
```
"OK"   — thay đổi thành công (hoặc số dư kết quả >= 0)
"FAIL" — không đủ tiền để rút, hoặc username không tồn tại
```

**Logic:**
```
if (username not found)                      → FAIL
if (change < 0 && balance < abs(change))     → FAIL (không đủ tiền)
balance = balance + change                   → OK
appendLog("deposit/withdrawal ±change newBalance")
```

**Ghi log:** Mỗi lần thay đổi số dư, ghi vào file `data/<username>.log`:
```
deposit +10000 110000
withdrawal -5000 105000
```

### 3.6. balance_transfer (4) — Chuyển khoản

**Request:**
```
payload = sender + ":" + recipient + ":" + amount
VD: "admin:user1:10000"   (admin chuyển user1 10000)
```

`amount` là số không dấu (`u64`). Dùng `std::stoull` để parse.

**Response:**
```
"OK"   — chuyển thành công
"FAIL" — sender không tồn tại, hoặc recipient không tồn tại, hoặc sender không đủ tiền
```

**Logic:**
```
if (sender not found || recipient not found)         → FAIL
if (sender.balance < amount)                         → FAIL
sender.balance   -= amount
recipient.balance += amount
appendLog(sender.log,   "transfer to recipient -amount newbalance")
appendLog(recipient.log, "transfer from sender +amount newbalance")
                                                     → OK
```

**Ghi log:** Cả hai bên đều được ghi log:
```
# data/admin.log:
transfer to user1 -10000 989999

# data/user1.log:
transfer from admin +10000 60000
```

### 3.7. ping (5) — Kiểm tra kết nối

**Request:**
```
payload = "" (rỗng)
```

**Response:**
```
"PONG"
```

> **Ghi chú:** Packet type này có trong enum nhưng chưa được implement ở server. Để thêm, xem `docs/how-to.md` mục 1.

### 3.8. account_change_password (6) — Đổi mật khẩu

**Request:**
```
payload = username + ":" + newpassword
VD: "admin:newpass456"
```

**Response:**
```
"OK" — mật khẩu đã được đổi (kể cả nếu username không tồn tại — idempotent)
```

### 3.9. account_toggle (7) — Khoá/Mở khoá

**Request:**
```
payload = username
VD: "admin"
```

**Response:**
```
"OK" — đã đảo trạng thái locked/unlocked (kể cả nếu username không tồn tại)
```

**Hành vi:** Mỗi lần gọi, đảo cờ `isLocked`. Tài khoản bị khoá không thể đăng nhập (kể cả đúng password).

---

## 4. Định dạng Payload & Delimiter

Payload là raw UTF-8, không có cấu trúc nhị phân. Server dùng `':'` (0x3A) làm delimiter.

### Quy tắc parse:

```
Một trường:       payload là toàn bộ nội dung
                  VD: "admin"                     → username = "admin"

Hai trường:       payload.split(':') lần đầu
                  VD: "admin:admin123"            → username="admin", password="admin123"

Ba trường:        payload.split(':') hai lần
                  VD: "admin:user1:10000"         → sender="admin", recipient="user1", amount="10000"
```

### Bảng chi tiết:

| Số trường | Loại packet | Thứ tự |
|-----------|-------------|--------|
| 1 | `balance_req`, `account_toggle`, `ping` | `[value]` |
| 2 | `account_auth`, `account_create`, `balance_change`, `account_change_password` | `[key]:[value]` |
| 3 | `balance_transfer` | `[sender]:[recipient]:[amount]` |

---

## 5. Xử lý lỗi

### Lỗi payload sai định dạng

Server kiểm tra số lượng delimiter trước khi xử lý:

```cpp
// server/Server.cpp
case account_auth: {
    auto delim = data.find(':');
    if (delim == std::string::npos) {     // ← Không tìm thấy ':'
        doWrite(std::to_underlying(account_auth), "FAIL");
        return;
    }
    // ... xử lý tiếp
}
```

Nếu payload thiếu delimiter → server trả `"FAIL"` ngay lập tức.

### Lỗi mạng (timeout, mất kết nối)

Client phát hiện qua `std::error_code` trong callback:

```
recvLoop:
  asyncRecv → ec != 0
    ├─ _connected = false
    ├─ Drain _pending queue: tất cả handler nhận (false, {})
    └─ Dừng recvLoop
```

### Bảng response theo lỗi:

| Loại lỗi | Packet type | Response |
|-----------|-------------|----------|
| Thiếu delimiter | Tất cả 2-3 trường | `"FAIL"` |
| Username không tồn tại | `balance_req` | `"0"` |
| Username không tồn tại | `auth` | `"FAIL"` |
| Username đã tồn tại | `account_create` | `"FAIL"` |
| Sai password | `auth` | `"FAIL"` |
| Tài khoản bị khoá | `auth` | `"FAIL"` |
| Không đủ tiền | `balance_change`, `balance_transfer` | `"FAIL"` |
| Recipient không tồn tại | `balance_transfer` | `"FAIL"` |
| Lỗi mạng | Tất cả | callback(false, {}) |

---

## 6. Ghi chú triển khai

### 6.1. C++ Implementation

```cpp
// libs/network/PacketType.hpp
enum class PacketType : u8 {
    account_auth          = 0,
    account_create        = 1,
    balance_req           = 2,
    balance_change        = 3,
    balance_transfer      = 4,
    ping                  = 5,
    account_change_password = 6,
    account_toggle        = 7,
};

// libs/network/Packet.cpp
auto asyncSend(asio::ip::tcp::socket& socket, PacketType type,
               const std::string& data, auto handler) -> void {
    auto header = std::make_shared<std::array<char, HeaderSize>>();

    // Ghi length (big-endian)
    auto len = toNetwork(static_cast<u32>(data.size()));
    std::memcpy(header->data(), &len, LengthSize);

    // Ghi type
    (*header)[LengthSize] = static_cast<char>(type);

    auto body = std::make_shared<std::string>(data);

    // Gửi header + body trong một lần
    asio::async_write(socket,
        {asio::buffer(*header), asio::buffer(*body)},
        [handler, header, body](std::error_code ec, usize) {
            if (handler) handler(ec);
        });
}
```

### 6.2. AsyncRecv — Hai giai đoạn

```
[1] async_read 5 byte header
    ├─ parse length (big-endian → host)
    └─ parse type

[2] async_read length byte body
    └─ callback(ec, type, body_string)
```

```cpp
auto asyncRecv(asio::ip::tcp::socket& socket, auto handler) -> void {
    auto header = std::make_shared<std::array<char, HeaderSize>>();

    asio::async_read(socket, asio::buffer(*header),
        [&socket, header, handler](std::error_code ec, usize) {
            if (ec) { handler(ec, {}, {}); return; }

            u32 len;
            std::memcpy(&len, header->data(), LengthSize);
            len = toNetwork(len);  // BE → host

            auto type = static_cast<PacketType>((*header)[LengthSize]);

            auto body = std::make_shared<std::string>(len, '\0');
            asio::async_read(socket, asio::buffer(*body),
                [body, type, handler](std::error_code ec2, usize) {
                    handler(ec2, ec2 ? PacketType{} : type,
                            ec2 ? std::string{} : *body);
                });
        });
}
```

### 6.3. Database format

Dữ liệu được lưu trong `data.db` — file nhị phân với các `Record` struct:

```cpp
#pragma pack(push, 1)
struct Record {
    char username[24];   // Tên đăng nhập, fixed-size (tối đa 23 ký tự + null)
    char password[24];   // Mật khẩu, fixed-size (tối đa 23 ký tự + null)
    u64  balance;        // Số dư (big-endian trên disk!)
    char logFile[32];    // Tên file log
    b8   isLocked;       // 0 = unlocked, 1 = locked
};
#pragma pack(pop)        // Tổng: 89 byte / record
```

### 6.4. Ghi log giao dịch

Mỗi user có file log riêng trong thư mục `data/`:

```
data/admin.log:
deposit +10000 110000
withdrawal -5000 105000
transfer to user1 -10000 95000

data/user1.log:
transfer from admin +10000 60000
```

Định dạng: `<operation> <số tiền> <số dư sau>`

### 6.5. Luồng xử lý trên server

```
doRead() → parse header + body → switch type → DbManager.op()
                                                     │
                                                     └─ enqueue → worker
                                                                   │
                                                                   └─ processItem → asio::post(callback)
                                                                                                │
                                                                                          doWrite() → asyncSend
                                                                                                      │
                                                                                                      └─ doRead() ← quay lại
```
