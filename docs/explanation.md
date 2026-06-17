# Explanation: Kiến trúc & Thiết kế Project KTLT

Tài liệu này giải thích **tại sao** project KTLT được thiết kế như vậy. Không chỉ là "cái gì", mà là "tại sao lại làm thế" — giúp fresher hiểu sâu về tư duy thiết kế phần mềm.

---

## 1. Tổng quan kiến trúc

### 1.1. Mô hình client-server qua TCP

```
┌──────────┐      TCP socket       ┌──────────┐
│  Server  │ ◄──────────────────►  │  Client  │
│ (C++26)  │      port 8080        │ (C++26)  │
└──────────┘                       └──────────┘
```

**Tại sao dùng TCP?**
- TCP đảm bảo dữ liệu đến đúng thứ tự, không mất mát — quan trọng cho giao dịch tài chính
- So với UDP, TCP đơn giản hơn cho người mới học (không cần xử lý mất gói, sắp xếp lại)
- Asio hỗ trợ TCP async rất tốt

**Tại sao không dùng HTTP?**
- HTTP là giao thức text, nặng nề cho ứng dụng đơn giản
- HTTP request/response mapping phức tạp hơn so với custom protocol
- Project là học thuật — tự xây dựng giao thức giúp hiểu sâu về network

### 1.2. Tại sao dùng Asio?

Asio là thư viện C++ cho I/O bất đồng bộ (async). Project dùng Asio vì:

- **Không block thread:** Thay vì tạo một thread riêng cho mỗi kết nối (tốn tài nguyên), Asio dùng `io_context` với event loop — một thread có thể xử lý hàng ngàn kết nối.
- **CROSS-PLATFORM:** Asio chạy trên Linux, Windows, macOS.
- **Tương lai:** Asio đang được đề xuất đưa vào chuẩn C++ (std::execution).

**So sánh blocking vs async:**

```
Blocking:                         Async:
thread 1: read(socket) ──block──► thread 1: io_context.run()
                                    ├─ async_read(socket, callback)
                                    ├─ async_accept(socket, callback)
                                    └─ ... (xử lý song song)
```

Với blocking, mỗi socket cần một thread. Với async, một thread xử lý được tất cả.

### 1.3. Tại sao dùng FTXUI?

FTXUI là thư viện UI cho terminal. Project dùng FTXUI vì:

- **Không cần đồ hoạ:** Chạy được trên terminal, không cần X11/Wayland
- **Reactive:** Cập nhật UI tự động khi state thay đổi
- **Nhẹ:** Chỉ dependency là một thư viện C++ duy nhất
- **Học thuật:** Dễ hiểu hơn Qt/GTK cho fresher

**Các thư viện UI khác cho terminal:**
- **ncurses:** Cũ, C-style API, dễ lỗi
- **FTXUI:** Modern C++, component-based, dùng được với CMake

---

## 2. Threading model

### 2.1. Server: single-threaded + worker thread

```
Server main thread                   Worker thread
─────────────────                   ─────────────
io_context.run()                     dbLoop()
  ├─ doAccept()                        ├─ wait(cv)
  ├─ Session::doRead()                 ├─ processItem(AuthOp)
  │    └─ DbManager::enqueue()         ├─ processItem(CreateAccountOp)
  │         └─ notify worker           ├─ ... 
  ├─ Session::doWrite()                └─ asio::post(callback)
  └─ ...
```

**Tại sao không dùng multi-threaded cho mỗi session?**
- Chi phí context switch
- Race condition phức tạp
- Với số lượng client nhỏ (học thuật), single thread đủ

**Tại sao cần worker thread riêng cho database?**
- I/O file (đọc/ghi data.db, ghi log) có thể block
- Worker thread giải phóng main thread khỏi I/O
- Queue-based processing: request được xếp hàng, xử lý tuần tự, không cần lock phức tạp

### 2.2. Client: dual-thread

```
Main thread (FTXUI)                  Client thread (Asio)
────────────────────                 ────────────────────
screen.Loop()                        io_context.run()
  ├─ Render UI                         ├─ connect()
  ├─ Event handling                    │    ├─ async_resolve
  │    └─ Button Login                 │    └─ async_connect
  │         └─ client.authenticate()   │         └─ recvLoop()
  │              └─ asio::post() ────► ├─ asyncSend(...)
  │                                    ├─ asyncRecv(...)
  │                                    │    └─ screen.Post() ──► UI update
  │                                    └─ ...
```

**Tại sao phải tách thread?**
- `io_context.run()` là blocking — nếu chạy trên main thread, UI sẽ bị đơ
- FTXUI cần main thread để render
- Giải pháp: network thread cho I/O, main thread cho UI

### 2.3. Tại sao cần work guard?

```cpp
// Client.hpp
asio::executor_work_guard<asio::io_context::executor_type> _work;
```

`io_context.run()` trả về ngay khi không còn việc gì để làm. Khi client mới tạo, chưa có connect, chưa có send — `_io.run()` sẽ trả về ngay lập tức, thread kết thúc.

Work guard giữ `io_context` sống, giống như nói: "sẽ có việc đến, hãy chờ".

```
Không work guard:           Có work guard:
_io.run()                    _io.run()
  ├─ không có handler          ├─ chờ (work guard giữ)
  └─ return ngay               ├─ connect() đến
                               ├─ xử lý...
                               └─ _io.stop() → work guard giải phóng
```

### 2.4. An toàn luồng

Bảng dưới đây giải thích từng tài nguyên được bảo vệ thế nào:

| Tài nguyên | Thread truy cập | Cơ chế bảo vệ | Giải thích |
|---|---|---|---|
| `Client::_socket` | Chỉ client thread | `asio::post` | Mọi thao tác socket đều post vào `_io`, đảm bảo luôn chạy trên đúng thread |
| `Client::_connected` | Client thread (write), Main thread (read) | `std::atomic<bool>` | Đọc/ghi không xung đột |
| `Client::_pending` | Chỉ client thread | `asio::post` | Queue chỉ truy cập trong io_context context |
| `DbManager::_queue` | Main + Worker | `std::mutex` + `std::condition_variable` | Mutex bảo vệ queue, CV thông báo worker |
| `DbManager::_data` | Chỉ worker thread | Không cần lock | Worker thread xử lý tuần tự |
| UI state (LoginPage, DashboardPage) | Chỉ main thread | `screen.Post()` | Mọi cập nhật từ network thread đều qua `screen.Post()` |

**Tại sao main thread có thể đọc `_connected` mà không cần lock?**
- `bool` là atomic trên thực tế (hầu hết CPU)
- Nếu đọc sai (stale value), hậu quả không nghiêm trọng (chỉ là show UI hơi sai)

---

## 3. Giao thức mạng

### 3.1. Tại sao header 5 byte? (length prefix + type)

```
[4 byte length (BE)] [1 byte type] [n byte payload]
```

**Length prefix (4 byte):**
- Giúp receiver biết trước kích thước payload, chỉ cần đọc đúng số byte
- Dùng big-endian (network byte order) — chuẩn mạng, đảm bảo tương thích giữa các kiến trúc CPU
- 4 byte → tối đa ~4GB payload (quá đủ cho ứng dụng ngân hàng)

**Type (1 byte):**
- 8 loại packet (hiện tại dùng 0-7)
- 1 byte = 256 loại — dư sức mở rộng

**Tại sao không dùng delimiter-based (VD: JSON)?**
- JSON phải parse, tốn CPU
- Delimiter (VD: `\n`) có thể xuất hiện trong dữ liệu
- Length prefix đơn giản, nhanh, chính xác

### 3.2. Tại sao dùng big-endian?

```cpp
auto toNetwork(u32 val) noexcept -> u32 {
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(val);  // Đảo byte nếu là little-endian
    }
    return val;
}
```

**Big-endian vs Little-endian:**

```
Giá trị 0x12345678:

Big-endian (mạng):     [0x12][0x34][0x56][0x78]
Little-endian (x86):   [0x78][0x56][0x34][0x12]
```

- CPU Intel/AMD dùng little-endian
- Mạng Internet dùng big-endian ("network byte order")
- Dùng `toNetwork` khi gửi, tự động đảo byte nếu cần

### 3.3. Tại sao payload là raw string thay vì binary struct?

Ví dụ, `authenticate` gửi `"admin:admin123"` thay vì struct nhị phân.

**Ưu điểm của raw string:**
- Dễ debug (có thể đọc trực tiếp)
- Không lo padding, alignment, endianness
- Flexible (dễ thêm field: `"user:pass:extra"`)

**Nhược điểm:**
- Lớn hơn binary một chút
- Phải parse delimiter

**Lựa chọn này phù hợp với project học thuật** — ưu tiên đơn giản và dễ hiểu hơn hiệu năng.

---

## 4. Database thiết kế

### 4.1. Tại sao dùng worker thread riêng?

```
Main thread                  Worker thread
──────────                   ─────────────
db.authenticate()            dbLoop()
  ├─ enqueue(AuthOp)            ├─ dequeue()
  └─ return ngay                ├─ processItem()
                                │   ├─ tìm user
                                │   ├─ so sánh password
                                │   └─ asio::post(callback)
                                └─ loop
```

**Lý do:**
- I/O file (load/save data.db, appendLog) có thể block lâu
- Nếu xử lý trên main thread, tất cả session đều bị treo
- Worker thread giải phóng main thread cho network I/O
- Queue đảm bảo thứ tự xử lý — đúng với giao dịch tài chính

> **Lưu ý về persistence:** `save()` chỉ được gọi trong `~DbManager()`. Dữ liệu mới chỉ tồn tại trong memory cho đến khi server tắt. Nếu server bị kill (Ctrl+C) mà không có signal handler, destructor không chạy → dữ liệu **mất**. Server hiện tại đã cài signal handler cho SIGINT/SIGTERM để graceful shutdown.

### 4.2. Tại sao dùng std::variant cho WorkItem?

```cpp
using WorkItem = std::variant<AuthOp, CreateAccountOp, ChangeBalanceOp, ...>;
// Xử lý:
std::visit([...](auto&& op) {
    using T = std::decay_t<decltype(op)>;
    if constexpr (std::is_same_v<T, AuthOp>) { ... }
    else if constexpr (std::is_same_v<T, CreateAccountOp>) { ... }
    ...
}, item);
```

**So sánh với các cách khác:**

| Cách | Ưu | Nhược |
|------|-----|-------|
| `std::variant` + `std::visit` | Type-safe, nhanh, không cần vtable | Hơi khó đọc |
| Base class + virtual | Dễ mở rộng | Slow (virtual call), heap alloc |
| `enum` + `union` | Nhanh | Không type-safe, dễ lỗi |
| `std::any` + `typeid` | Flexible | Chậm, runtime check |

`std::variant` là lựa chọn tối ưu: kiểm tra kiểu tại compile time, hiệu năng cao, không heap allocation.

### 4.3. Tại sao dùng queue thay vì lock từng record?

**Phương án 1 — Lock từng record:**
```cpp
std::mutex _mutex;
std::unordered_map<std::string, Record> _data;
// Mỗi operation: lock → read/write → unlock
```

Vấn đề:
- Race condition phức tạp (nhiều operation trên cùng record)
- Deadlock nếu không cẩn thận
- Không đảm bảo thứ tự giao dịch

**Phương án 2 — Queue (project dùng):**
```cpp
std::queue<WorkItem> _queue;
// Worker thread xử lý tuần tự
```

Ưu điểm:
- Tự nhiên: giao dịch ngân hàng nên xử lý tuần tự
- Không race condition
- Dễ maintain

### 4.4. Binary format: packed struct + big-endian

```cpp
#pragma pack(push, 1)  // Không padding
struct Record {
    char username[24];  // Fixed-size (tối đa 23 ký tự + null, so với std::string)
    char password[24];  // Fixed-size (tối đa 23 ký tự + null)
    u64  balance;        // Big-endian trên disk
    char logFile[32];
    b8   isLocked;
};
#pragma pack(pop)
```

**Tại sao fixed-size array thay vì std::string?**
- File binary cần kích thước cố định để seek
- `std::string` chứa pointer (không serialize được)
- Fixed-size array cho phép đọc/ghi trực tiếp vào struct

**Tại sao `#pragma pack(1)`?**
- Mặc định, compiler thêm padding để align memory
- Record trên disk không có padding — phải pack để struct khớp với file
- Nếu không pack, `sizeof(Record)` sẽ lớn hơn 89 byte, đọc file sẽ sai

**Sơ đồ Record trên disk:**

```
Offset 0:  username[24]    ─ 24 byte
Offset 24: password[24]    ─ 24 byte
Offset 48: balance (u64)   ─ 8 byte (big-endian!)
Offset 56: logFile[32]     ─ 32 byte
Offset 88: isLocked (b8)   ─ 1 byte
                               ─────
                          Total: 89 byte
```

---

## 5. Base library — triết lý thiết kế

### 5.1. Tại sao tự viết STL?

Đây là câu hỏi quan trọng nhất. Dự án tự viết lại `Vector`, `String`, `List`, `Map`... dù `std::` đã có sẵn.

**Lý do:**
1. **Học tập:** Hiểu cấu trúc dữ liệu hoạt động thế nào — không chỉ dùng mà còn biết cách xây dựng
2. **Kiến thức nền:** Sinh viên CS cần hiểu:
   - Vector: cấp phát động, growth factor, exception safety
   - String: SSO, COW (copy-on-write)
   - List/RBTree/HashTable: con trỏ, sentinel, rotation, fixup
   - Iterator pattern: generic iteration
3. **Tự do:** Có thể thay đổi implementation để học hỏi

### 5.2. SSO (Small String Optimization) trong String

```
Chuỗi ngắn (≤ 15 ký tự):        Chuỗi dài (> 15 ký tự):
┌──────────────────────────┐    ┌──────────────────────────┐
│ [h][e][l][l][o][\0]...   │    │ [ptr] ──────────► heap   │
│ _size = 5                │    │ _size = 30              │
│ _isLong = false          │    │ _cap = 30               │
└──────────────────────────┘    │ _isLong = true           │
                                 └──────────────────────────┘
```

**Tại sao cần SSO?**
- Hầu hết chuỗi trong thực tế đều ngắn (< 20 ký tự)
- Nếu luôn dùng heap: mỗi chuỗi tốn 1 allocation (chậm, phân mảnh bộ nhớ)
- SSO tránh heap allocation cho chuỗi ngắn → nhanh hơn

**SSO_Capacity = (sizeof(pointer) + sizeof(size_type)) / sizeof(Char) - 1**

Tính trên 64-bit với `char`:
- `sizeof(pointer) + sizeof(size_type) = 8 + 8 = 16`
- `16 / 1 - 1 = 15` ký tự

**Tại sao không dùng COW (Copy-on-Write)?**
- COW từng được `std::string` dùng (trước C++11), nhưng bỏ vì:
  - Thread safety phức tạp
  - `operator[]` trả về reference — không biết ai sửa
  - SSO đơn giản và nhanh hơn

### 5.3. RBTree (Map/Set) vs HashTable (HashMap/HashSet)

```
RBTree:                        HashTable:
───────                        ─────────
Cây cân bằng                   Mảng bucket + chain
├─ insert: O(log n)            ├─ insert: O(1) average
├─ find:   O(log n)            ├─ find:   O(1) average
├─ erase:  O(log n)            ├─ erase:  O(1) average
├─ Duyệt có thứ tự: O(n)       ├─ Duyệt không thứ tự
└─ lower_bound/upper_bound: ✔  └─ lower_bound: ✗
```

**Khi nào dùng Map (RBTree)?**
- Cần duyệt theo thứ tự key
- Cần `lower_bound` / `upper_bound` / `equal_range`
- Cần tìm key gần nhất (VD: tìm ≤ key)

**Khi nào dùng HashMap (HashTable)?**
- Chỉ cần tìm chính xác (không cần thứ tự)
- Cần tốc độ O(1)
- Dữ liệu lớn (HashTable nhanh hơn RBTree)

Project KTLT dùng **cả hai** — cùng interface gần như nhau — để người học so sánh.

### 5.4. Growth factor 2 trong Vector

```cpp
static constexpr size_type GrowthFactor = 2;  // Nhân đôi
```

**Tại sao factor 2?**
- Amortized O(1) cho `push_back`
- Nếu factor quá nhỏ (VD: 1.2): phải reallocate nhiều lần
- Nếu factor quá lớn (VD: 4): lãng phí bộ nhớ
- Factor 2 là cân bằng tốt giữa speed và memory

**Các lựa chọn khác:**
- Factor 1.5: tiết kiệm bộ nhớ hơn, có thể tái sử dụng bộ nhớ đã free
- Factor 2: đơn giản, dễ tính toán (dùng shift thay vì multiply)

### 5.5. Exception safety

Khi `Vector` reallocate (trong `transfer_to`):

```cpp
constexpr auto transfer_to(pointer new_data, size_type new_cap) -> void {
    size_type idx = 0;
    try {
        for (; idx < _size; ++idx) {
            // Copy/move từ cũ → mới
            allocator_traits::construct(_alloc, new_data + idx,
                                        std::move_if_noexcept(_data[idx]));
        }
    } catch (...) {
        // Nếu fail: huỷ các phần tử đã copy, giải phóng vùng mới
        for (size_type j = 0; j < idx; ++j)
            allocator_traits::destroy(_alloc, new_data + j);
        allocator_traits::deallocate(_alloc, new_data, new_cap);
        throw;  // Dữ liệu cũ vẫn còn nguyên
    }
    // Nếu thành công: destroy dữ liệu cũ, swap pointer
    destroy_range(_data, _data + _size);
    deallocate(_data, _cap);
    _data = new_data;
    _cap  = new_cap;
}
```

Đây là **strong exception guarantee**: nếu `push_back` throw, Vector giữ nguyên trạng thái cũ. Không mất dữ liệu.

---

## 6. Client UI thiết kế

### 6.1. Tại sao dùng recv loop thay vì request-response?

**Phương án cũ (blocking):**

```
send("admin:admin123")
recv()  ← block chờ response
// Response đến
```

Vấn đề:
- Nếu server chậm, UI bị treo
- Nếu gửi 2 request nhanh: response 1 và 2 dễ bị swap
- Cần mutex/phức tạp để đảm bảo thứ tự

**Phương án mới (recv loop):**

```
sendRequest(payload, handler):
  _pending.push(handler)
  asyncSend()

recvLoop():
  asyncRecv(...) → _pending.pop() → handler(response)
  recvLoop()  ← lặp
```

**Tại sao tốt hơn?**
- Non-blocking: UI không bị treo
- FIFO queue: handler gọi đúng thứ tự
- Error handling: lỗi mạng drain toàn bộ queue với handler(false, {})
- Đơn giản: không cần request ID, không cần map request→response

### 6.2. Tại sao cần screen.Post()?

```cpp
// Trong callback (chạy trên client thread):
_screen.Post([this, ok] {
    // Code này chạy trên main thread
    _loading = false;
    if (ok) { _page = 1; }
});
```

FTXUI (và hầu hết UI framework) không thread-safe. Mọi thay đổi UI phải trên main thread.

`screen.Post()` đưa lambda vào hàng đợi của main thread, đảm bảo:
- UI state chỉ sửa trên một thread
- Không race condition
- render() thấy state nhất quán

### 6.3. Tại sao SignupPage có validation riêng?

SignupPage kiểm tra `_password == _confirmPassword` **trước khi gửi request**:

```cpp
void SignupPage::doCreateAccount() {
    if (_password != _confirmPassword) {
        _status = "Passwords do not match";
        return;  // ← Không gửi request!
    }
    _client.createAccount(...);
}
```

**Tại sao không để server kiểm tra?**

1. **Tiết kiệm tài nguyên:** Không gửi request vô ích qua mạng → giảm tải server
2. **Phản hồi tức thì:** User thấy lỗi ngay, không cần chờ round-trip mạng
3. **Nguyên tắc defence-in-depth:** Validate client-side + server-side. Server vẫn kiểm tra tính hợp lệ (username tồn tại, v.v.), client kiểm tra những gì có thể (empty fields, password match)

Đây là pattern phổ biến trong thiết kế web/app: **client validation cho trải nghiệm, server validation cho bảo mật**.

### 6.4. Container::Tab pattern

```cpp
auto container = ftxui::Container::Tab({loginComp, dashComp, signupComp}, &page);
```

`Container::Tab` là cơ chế chuyển trang của FTXUI:
- `page = 0` → hiện Login
- `page = 1` → hiện Dashboard
- `page = 2` → hiện Signup (tạo tài khoản)
- Chuyển trang = thay đổi biến `page` — có thể từ nhiều nguồn (nút bấm, callback)

**Tại sao dùng `int&` thay vì setter?**
- FTXUI cần reference để tự động re-render
- `&page` được giữ và theo dõi — khi page thay đổi, Tab tự cập nhật
- Không cần gọi hàm render lại — FTXUI làm việc đó tự động qua `screen.Loop()`

---

## 7. Tổng kết: kiến trúc tổng thể

```
                         ┌─────────────────────────────────────────┐
                         │                Client                   │
                         │  ┌─────────────────────────────────────┐ │
                         │  │         Main Thread (FTXUI)        │ │
                          │  │  ┌──────────┐  ┌───────────────┐  ┌────────────┐ │ │
                          │  │  │ LoginPage│  │ DashboardPage │  │ SignupPage│ │ │
                          │  │  └────┬─────┘  └──────┬────────┘  └─────┬──────┘ │ │
                          │  │       │                │                │        │ │
                          │  │       └──────┬─────────┴────────┬───────┘        │ │
                          │  │              │ Container::Tab                   │ │
                          │  │              ▼ page=0/1/2                      │ │
                         │  │        ┌─────────────┐            │ │
                         │  │        │ screen.Loop │            │ │
                         │  │        └─────────────┘            │ │
                         │  └─────────────────────────────────────┘ │
                         │              │ screen.Post()            │
                         │              ▼                          │
                         │  ┌─────────────────────────────────────┐ │
                         │  │       Client Thread (Asio)         │ │
                         │  │  ┌──────────┐  ┌──────────────┐   │ │
                         │  │  │ recvLoop │  │ sendRequest  │   │ │
                         │  │  └────┬─────┘  └──────┬───────┘   │ │
                         │  │       │                │           │ │
                         │  │       └───────┬────────┘           │ │
                         │  │               ▼                    │ │
                         │  │      ┌────────────────┐            │ │
                         │  │      │ asyncSend/Recv │            │ │
                         │  │      └───────┬────────┘            │ │
                         │  └──────────────┼───────────────────────┘ │
                         └─────────────────┼─────────────────────────┘
                                           │ TCP socket port 8080
                         ┌─────────────────┼─────────────────────────┐
                         │                 ▼                         │
                         │               Server                      │
                         │  ┌─────────────────────────────────────┐ │
                         │  │      Main Thread (Asio)            │ │
                         │  │  ┌──────────┐  ┌───────────────┐  │ │
                         │  │  │ doAccept │  │ Session (n)   │  │ │
                         │  │  └──────────┘  │ ┌───────────┐ │  │ │
                         │  │                │ │ doRead()  │ │  │ │
                         │  │                │ │ doWrite() │ │  │ │
                         │  │                │ └───────────┘ │  │ │
                         │  │                └───────┬───────┘  │ │
                         │  │                        │          │ │
                         │  │                        ▼          │ │
                         │  │              ┌────────────────┐   │ │
                         │  │              │ DbManager      │   │ │
                         │  │              │ enqueue + cv   │   │ │
                         │  │              └───────┬────────┘   │ │
                         │  └──────────────────────┼───────────────┘ │
                         │                         ▼                │
                         │  ┌─────────────────────────────────────┐ │
                         │  │       Worker Thread                │ │
                         │  │  ┌─────────────────────────────┐   │ │
                         │  │  │ dbLoop: dequeue → process  │   │ │
                         │  │  │ ┌───────────────────────┐   │   │ │
                         │  │  │ │ processItem(variant)  │   │   │ │
                         │  │  │ │ └─ đọc/ghi _data      │   │   │ │
                         │  │  │ │ └─ ghi log file       │   │   │ │
                         │  │  │ │ └─ asio::post(cb)     │   │   │ │
                         │  │  │ └───────────────────────┘   │   │ │
                         │  │  └─────────────────────────────┘   │ │
                         │  └─────────────────────────────────────┘ │
                         └───────────────────────────────────────────┘
```

**Ba luồng chính:**
1. **Main thread (Client):** Render UI + xử lý sự kiện bàn phím
2. **Client thread (Client):** Kết nối mạng, gửi/nhận packet
3. **Main thread (Server):** Chấp nhận kết nối, đọc/ghi socket
4. **Worker thread (Server):** Xử lý database (I/O file)

**Luồng dữ liệu cho một giao dịch:**

```
User bấm Login
  → Main thread: doLogin()
    → Client::authenticate()
      → asio::post(client thread)
        → asyncSend() ──TCP──→ Server::doRead()
          → DbManager::authenticate()
            → notify worker thread
              → processItem(AuthOp)
                → asio::post(lại server thread)
                  → Session::doWrite()
                    → asyncSend() ──TCP──→ Client::recvLoop()
                      → pop handler
                        → handler(true, "OK")
                          → screen.Post(main thread)
                            → UI cập nhật: hiện Dashboard
```

Đây là luồng bất đồng bộ hoàn chỉnh — không thread nào bị block chờ thread khác.
