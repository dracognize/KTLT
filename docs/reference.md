# Reference: Tra cứu API đầy đủ

Tài liệu tham khảo toàn diện cho project KTLT. Tra cứu từng file, class, struct, enum, function.

---

## 1. Sơ đồ tổng quan

```
libs/base/          libs/network/       server/             app/
──────────          ─────────────       ────────            ───────
types.hpp           PacketType.hpp      main.cpp            main.cpp
algorithm.hpp       Packet.hpp          Server.hpp/cpp      App.hpp/cpp
array.hpp           Packet.cpp          Database.hpp/cpp    client/Client.hpp/cpp
vector.hpp                              ServerLog.hpp/cpp   pages/LoginPage.hpp/cpp
string.hpp                                                  pages/DashboardPage.hpp/cpp
forward_list.hpp
list.hpp
rbtree.hpp
map.hpp
set.hpp
hash.hpp
hash_table.hpp
hash_map.hpp
hash_set.hpp
```

| Module | Phụ thuộc vào |
|--------|---------------|
| `libs/base` | Không (chỉ dùng std) |
| `libs/network` | `libs/base`, Asio |
| `server` | `libs/base`, `libs/network`, Asio, Threads |
| `app` | `libs/base`, `libs/network`, Asio, FTXUI, Threads |

---

## 2. Thư viện base (`libs/base/`)

Tất cả đều nằm trong `namespace base`.

### 2.1. `types.hpp` — Type aliases

```cpp
using u8   = std::uint8_t;      // 0..255
using u16  = std::uint16_t;     // 0..65535
using u32  = std::uint32_t;     // 0..4,294,967,295
using u64  = std::uint64_t;     // 0..18,446,744,073,709,551,615
using usize = std::size_t;      // kích thước (phụ thuộc kiến trúc 32/64-bit)

using i8   = std::int8_t;       // -128..127
using i16  = std::int16_t;      // -32768..32767
using i32  = std::int32_t;      // -2^31..2^31-1
using i64  = std::int64_t;      // -2^63..2^63-1
using isize = std::ptrdiff_t;   // hiệu con trỏ

using f32 = float;              // số thực 32-bit
using f64 = double;             // số thực 64-bit

using b1  = bool;               // boolean 1-bit (thực tế 1 byte)
using b8  = u8;                 // boolean 8-bit
using b16 = u16;                // boolean 16-bit
using b32 = u32;                // boolean 32-bit
using b64 = u64;                // boolean 64-bit

using byte = u8;                // byte đơn
```

### 2.2. `algorithm.hpp` — base::swap

```cpp
template <class T>
constexpr auto swap(T& a, T& b) noexcept(...) -> void;
```
Hoán đổi giá trị hai biến dùng move semantics. Tương tự `std::swap`.

---

### 2.3. `array.hpp` — base::Array<T, N>

Mảng tĩnh kích thước cố định. Tương tự `std::array`.

```cpp
template <class T, usize Size>
struct Array {
    // Type aliases
    using value_type      = T;
    using size_type       = usize;
    using reference       = T&;
    using const_reference = const T&;
    using iterator        = T*;
    using const_iterator  = const T*;

    // Element access
    auto operator[](usize index) -> T&;
    auto at(usize index) -> T&;           // throw std::out_of_range
    auto front() -> T&;
    auto back() -> T&;
    auto data() -> T*;

    // Iterators
    auto begin() -> T*;
    auto end() -> T*;
    auto rbegin() -> reverse_iterator;
    auto rend() -> reverse_iterator;
    auto cbegin() const -> const T*;
    auto cend() const -> const T*;
    auto crbegin() const -> const_reverse_iterator;
    auto crend() const -> const_reverse_iterator;

    // Capacity
    constexpr auto size() const -> usize;       // = Size
    constexpr auto max_size() const -> usize;   // = Size
    constexpr auto empty() const -> bool;       // = Size == 0

    // Modifiers
    constexpr auto fill(const T& value) -> void;
    constexpr auto swap(Array& other) -> void;

    // Comparisons (C++20 default)
    auto operator==(const Array&) const -> bool = default;
    auto operator<=>(const Array&) const = default;
};
```

**Special case:** `Array<T, 0>` — mảng rỗng, `operator[]` gọi `std::unreachable()`.

**Helper:**
```cpp
template <class T, class... Ts>
Array(T, Ts...) -> Array<T, 1 + sizeof...(Ts)>;  // CTAD

template <usize Index, class T, usize Size>
constexpr auto get(Array<T, Size>& arr) -> T&;   // tuple interface

template <class T, usize Size>
constexpr auto to_array(T (&src)[Size]) -> Array<T, Size>;  // C-style → Array
```

---

### 2.4. `vector.hpp` — base::Vector<T, Alloc>

Mảng động. Tương tự `std::vector`.

```cpp
template <class T, class Alloc = std::allocator<T>>
struct Vector {
    // Hằng số
    static constexpr size_type GrowthFactor    = 2;   // Nhân đôi khi đầy
    static constexpr size_type InitialCapacity = 1;   // Dung lượng ban đầu

    // Element access
    auto operator[](size_type pos) -> T&;
    auto at(size_type pos) -> T&;              // throw std::out_of_range
    auto front() -> T&;
    auto back() -> T&;
    auto data() -> T*;

    // Iterators
    auto begin() -> T*;
    auto end() -> T*;
    auto rbegin() -> reverse_iterator;
    auto rend() -> reverse_iterator;

    // Capacity
    auto empty() const -> bool;
    auto size() const -> size_type;
    auto max_size() const -> size_type;
    auto capacity() const -> size_type;
    auto reserve(size_type new_cap) -> void;    // Cấp phát trước
    auto shrink_to_fit() -> void;               // Giảm về đúng size

    // Modifiers
    auto clear() -> void;
    auto insert(const_iterator pos, const T& value) -> iterator;
    auto insert(const_iterator pos, T&& value) -> iterator;
    auto emplace(const_iterator pos, Args&&... args) -> iterator;
    auto erase(const_iterator pos) -> iterator;
    auto erase(const_iterator first, const_iterator last) -> iterator;
    auto push_back(const T& value) -> void;
    auto push_back(T&& value) -> void;
    auto emplace_back(Args&&... args) -> reference;
    auto pop_back() -> void;
    auto resize(size_type count) -> void;
    auto resize(size_type count, const T& value) -> void;
    auto swap(Vector& other) -> void;

    // Allocator
    auto get_allocator() const -> allocator_type;
};
```

**Sơ đồ bộ nhớ:**

```
_data: [ T0 ][ T1 ][ T2 ][   ]...[   ]
        ^                    ^         ^
       begin                end      end_of_cap
       _size = 3           _cap = 8
```

**Cơ chế reallocate:** Khi `_size == _cap`, cấp phát vùng mới gấp đôi, copy/move từ vùng cũ, destroy vùng cũ. Exception-safe: nếu copy fail, vùng mới được giải phóng, dữ liệu cũ vẫn còn nguyên.

---

### 2.5. `string.hpp` — base::BasicString<Char, Traits, Alloc>

Chuỗi ký tự với Small String Optimization (SSO). Tương tự `std::string`.

```cpp
template <class Char, class Traits = std::char_traits<Char>,
          class Alloc = std::allocator<Char>>
class BasicString {
    // SSO_Capacity = (sizeof(pointer) + sizeof(size_type)) / sizeof(Char) - 1
    // Trên 64-bit: (8 + 8) / 1 - 1 = 15 byte cho char
    //              (8 + 8) / 4 - 1 = 3  cho char32_t
};
using String   = BasicString<char>;
using U8String = BasicString<char8_t>;
```

**Sơ đồ SSO:**

```
Short (≤ 15 ký tự):          Long (> 15 ký tự):
┌──────────────────────┐     ┌──────────────────────┐
│  _buffer[0..14]      │     │  _data (pointer) ────► heap
│  _size               │     │  _size                │
└──────────────────────┘     │  _cap                 │
                              └──────────────────────┘
```

```cpp
// Element access
auto operator[](size_type pos) -> Char&;
auto at(size_type pos) -> Char&;     // throw std::out_of_range
auto front() -> Char&;
auto back() -> Char&;
auto data() -> Char*;
auto c_str() const -> const Char*;   // null-terminated

// Capacity
auto size() const -> size_type;
auto length() const -> size_type;
auto capacity() const -> size_type;
auto empty() const -> bool;
auto reserve(size_type new_cap) -> void;

// Modifiers
auto clear() -> void;
auto push_back(Char c) -> void;
auto pop_back() -> void;

// Comparisons
auto operator==(const BasicString&) const -> bool;

// Output
friend auto operator<<(ostream&, const BasicString&) -> ostream&;
```

**std::formatter specialization:** `String` có thể dùng với `std::format`.

---

### 2.6. `forward_list.hpp` — base::ForwardList<T, Alloc>

Danh sách liên kết đơn. Tương tự `std::forward_list`.

```cpp
template <class T, class Alloc = std::allocator<T>>
struct ForwardList {
    struct Node {
        T    _value;
        Node* _next;
    };

    // Iterators (forward_iterator_tag)
    using iterator       = ForwardIterator<Node, T&, T*>;
    using const_iterator = ForwardIterator<const Node, const T&, const T*>;

    // Đặc biệt: before_begin() — iterator trước phần tử đầu
    auto before_begin() -> iterator;
    auto cbefore_begin() const -> const_iterator;

    auto begin() -> iterator;
    auto end() -> iterator;

    // Capacity
    auto empty() const -> bool;
    auto size() const -> size_type;
    auto max_size() const -> size_type;

    // Element access
    auto front() -> T&;

    // Modifiers
    auto clear() -> void;
    auto push_front(const T&) -> void;
    auto push_front(T&&) -> void;
    auto pop_front() -> void;
    auto emplace_front(Args&&...) -> reference;
    auto insert_after(const_iterator pos, const T&) -> iterator;
    auto emplace_after(const_iterator pos, Args&&...) -> iterator;
    auto erase_after(const_iterator pos) -> iterator;
    auto erase_after(const_iterator first, const_iterator last) -> iterator;
    auto resize(size_type count) -> void;

    // Operations
    auto reverse() -> void;
    auto remove(const T& val) -> size_type;
    auto remove_if(UnaryPred pred) -> size_type;
    auto unique() -> size_type;
    auto merge(ForwardList& other) -> void;
};
```

**Sơ đồ:**

```
_head (sentinel) ──► [Node0] ──► [Node1] ──► [Node2] ──► nullptr
                      ^
                   begin()
```

Điểm đặc biệt: `before_begin()` trả về iterator tới `_head` (sentinel), cho phép `insert_after` và `erase_after` ở đầu danh sách.

---

### 2.7. `list.hpp` — base::List<T, Alloc>

Danh sách liên kết đôi vòng. Tương tự `std::list`.

```cpp
template <class T, class Alloc = std::allocator<T>>
struct List {
    struct Node {
        T     _value;
        Node* _prev;
        Node* _next;
    };

    // Iterators (bidirectional_iterator_tag)
    using iterator       = ListIterator<Node, T&, T*>;
    using const_iterator = ListIterator<const Node, const T&, const T*>;

    // Element access
    auto front() -> T&;
    auto back() -> T&;

    // Modifiers
    auto insert(const_iterator pos, const T&) -> iterator;
    auto emplace(const_iterator pos, Args&&...) -> iterator;
    auto emplace_front(Args&&...) -> reference;
    auto emplace_back(Args&&...) -> reference;
    auto push_front(const T&) -> void;
    auto push_back(const T&) -> void;
    auto pop_front() -> void;
    auto pop_back() -> void;
    auto erase(const_iterator pos) -> iterator;
    auto erase(const_iterator first, const_iterator last) -> iterator;
    auto resize(size_type count) -> void;

    // Operations
    auto reverse() -> void;
    auto remove(const T&) -> size_type;
    auto remove_if(UnaryPred) -> size_type;
    auto unique() -> size_type;
    auto merge(List& other) -> void;
    auto sort() -> void;                         // Merge sort
    auto sort(Compare comp) -> void;
    auto splice(const_iterator pos, List& other) -> void;
};
```

**Sơ đồ cấu trúc vòng:**

```
             ┌──────────────────────────────────────┐
             │                                      ▼
  _head (sentinel) ◄══► [Node0] ◄══► [Node1] ◄══► [Node2]
             ▲                                      │
             └──────────────────────────────────────┘
```

`_head` là sentinel (node đặc biệt). List rỗng: `_head._next == _head._prev == &_head`.

**Merge sort:** `sort()` dùng thuật toán merge sort với `_split` (tìm midpoint bằng 2 con trỏ) và `_sorted_merge`.

---

### 2.8. `rbtree.hpp` — base::RedBlackTree<T, Comp, Alloc>

Cây đỏ-đen theo thuật toán CLRS. Là nền tảng cho `Map` và `Set`.

```cpp
template <class T, class Comp = std::less<T>, class Alloc = std::allocator<T>>
struct RedBlackTree {
    struct Node {
        T     _value;
        Node* _parent;
        Node* _left;
        Node* _right;
        bool  _isBlack;     // true = đen, false = đỏ
    };

    // Iterators (bidirectional, đi qua _nil sentinel)
    using iterator       = TreeIterator<Node, T&, T*>;
    using const_iterator = TreeIterator<const Node, const T&, const T*>;

    // Modifiers
    auto insert(const T& val) -> pair<iterator, bool>;
    auto erase(const_iterator pos) -> iterator;
    auto erase(const T& val) -> size_type;
    auto clear() -> void;

    // Lookup
    auto find(const T& val) -> iterator;
    auto contains(const T& val) -> bool;
    auto lower_bound(const T& val) -> iterator;
    auto upper_bound(const T& val) -> iterator;
};
```

**Sơ đồ cấu trúc:**

```
                        _header (giống sentinel)
                       /     |     \
                   _left   _parent  _right
                     │        │        │
                 min node   root node   max node
                               │
                          ┌────┴────┐
                       left       right
                          │          │
                      ┌───┴──┐    ┌──┴──┐
                      nil   nil  nil  nil
```

- `_nil` là sentinel (con trỏ tới `_header` khi cây rỗng)
- `_header._parent` = root
- `_header._left` = node nhỏ nhất (để begin() O(1))
- `_header._right` = node lớn nhất

**Thuật toán:**
- Insert: BST insert → `_insert_fixup` (xoay + đổi màu)
- Erase: BST transplant → `_erase_fixup`
- Rotations: `_left_rotate`, `_right_rotate` (O(1))

---

### 2.9. `map.hpp` — base::Map<K, V, Comp, Alloc>

Map có thứ tự (dùng RedBlackTree). Tương tự `std::map`.

```cpp
template <class K, class V, class Comp = std::less<K>,
          class Alloc = std::allocator<pair<const K, V>>>
struct Map {
    using key_type    = K;
    using mapped_type = V;
    using value_type  = pair<const K, V>;

    // Element access
    auto at(const K& key) -> V&;           // throw std::out_of_range
    auto operator[](const K& key) -> V&;   // Tạo mới nếu chưa tồn tại

    // Modifiers
    auto insert(const value_type&) -> pair<iterator, bool>;
    auto emplace(const_iterator, Args&&...) -> iterator;
    auto try_emplace(const K& key, Args&&... args) -> pair<iterator, bool>;
    auto insert_or_assign(const K& key, V&& val) -> pair<iterator, bool>;
    auto erase(const_iterator pos) -> iterator;
    auto erase(const K& key) -> size_type;
    auto clear() -> void;
    auto extract(const_iterator pos) -> value_type;

    // Lookup
    auto find(const K& key) -> iterator;
    auto contains(const K& key) -> bool;
    auto lower_bound(const K& key) -> iterator;
    auto upper_bound(const K& key) -> iterator;
    auto equal_range(const K& key) -> pair<iterator, iterator>;
};
```

**Lưu ý:** `operator[]` tạo value mặc định nếu key chưa tồn tại (giống `std::map`).

---

### 2.10. `set.hpp` — base::Set<K, Comp, Alloc>

Set có thứ tự (dùng RedBlackTree). Tương tự `std::set`.

```cpp
template <class K, class Comp = std::less<K>, class Alloc = std::allocator<K>>
struct Set {
    // iterator và const_iterator đều là const_iterator (không cho sửa value)
    using iterator       = _base::const_iterator;
    using const_iterator = _base::const_iterator;

    auto insert(const K&) -> pair<iterator, bool>;
    auto erase(const_iterator) -> iterator;
    auto erase(const K&) -> size_type;
    auto find(const K&) const -> const_iterator;
    auto contains(const K&) const -> bool;
    auto extract(const_iterator pos) -> value_type;
};
```

---

### 2.11. `hash.hpp` — base::hash<T>

```cpp
template <class T>
struct hash {
    // Chỉ dùng được cho trivially copyable types
    usize operator()(const T& key) const noexcept;
};
```

Dùng **MurmurHash3 128-bit** (rút gọn về 64-bit). Có specializations cho:
- `BasicString<Char, Traits, Alloc>` — hash theo nội dung
- `Array<T, Size>` — hash theo nội dung
- `Vector<T, Alloc>` — hash theo nội dung
- `ForwardList<T, Alloc>` — hash từng phần tử (boost::hash_combine style)
- `List<T, Alloc>` — hash từng phần tử

---

### 2.12. `hash_table.hpp` — base::HashTable<K, V, Hash, KeyEqual, Alloc>

Bảng băm dùng separate chaining. Là nền tảng cho `HashMap` và `HashSet`.

```cpp
template <class K, class V, class Hash = hash<K>,
          class KeyEqual = std::equal_to<K>,
          class Alloc = std::allocator<pair<const K, V>>>
struct HashTable {
    static constexpr size_type DEFAULT_BUCKET_COUNT = 8;
    static constexpr size_type REHASH_GROWTH_FACTOR = 2;
    static constexpr float     DEFAULT_MAX_LOAD     = 1.0f;

    // Modifiers
    auto insert(const value_type&) -> pair<iterator, bool>;
    auto emplace(Args&&...) -> pair<iterator, bool>;
    auto erase(const_iterator) -> iterator;
    auto erase(const K&) -> size_type;
    auto clear() -> void;

    // Lookup
    auto find(const K&) -> iterator;
    auto contains(const K&) -> bool;

    // Bucket interface
    auto bucket_count() const -> size_type;
    auto bucket_size(size_type n) const -> size_type;
    auto bucket(const K& key) const -> size_type;

    // Hash policy
    auto load_factor() const -> float;
    auto max_load_factor() const -> float;
    auto max_load_factor(float ml) -> void;
    auto rehash(size_type n) -> void;
    auto reserve(size_type n) -> void;
};
```

**Sơ đồ bucket chain:**

```
_buckets: [ * ]──► [K1,V1]──► [K5,V5]──► nullptr
          [ * ]──► nullptr
          [ * ]──► [K2,V2]──► nullptr
          [ * ]──► nullptr
          ...
```

Iterator đi qua tất cả bucket, trong mỗi bucket đi theo chain.

---

### 2.13. `hash_map.hpp` — base::HashMap<K, V, Hash, KeyEqual, Alloc>

`HashMap` wrap `HashTable`. Tương tự `std::unordered_map`.

```cpp
template <class K, class V, class Hash = hash<K>,
          class KeyEqual = std::equal_to<K>,
          class Alloc = std::allocator<pair<const K, V>>>
struct HashMap {
    // Element access
    auto at(const K&) -> V&;
    auto operator[](const K&) -> V&;
    auto operator[](K&&) -> V&;

    // Modifiers
    auto insert(const value_type&) -> pair<iterator, bool>;
    auto try_emplace(const K& key, Args&&...) -> pair<iterator, bool>;
    auto insert_or_assign(const K& key, V&& val) -> pair<iterator, bool>;

    // Bucket & hash policy (delegate to HashTable)
    auto bucket_count() const -> size_type;
    auto load_factor() const -> float;
    auto rehash(size_type) -> void;
    auto reserve(size_type) -> void;
};
```

---

### 2.14. `hash_set.hpp` — base::HashSet<K, Hash, KeyEqual, Alloc>

`HashSet` wrap `HashTable<K, K>` (key = value). Tương tự `std::unordered_set`.

```cpp
template <class K, class Hash = hash<K>, class KeyEqual = std::equal_to<K>,
          class Alloc = std::allocator<K>>
struct HashSet {
    // iterator và const_iterator đều là const_iterator
    using iterator       = _base::const_iterator;
    using const_iterator = _base::const_iterator;

    auto insert(const K&) -> pair<iterator, bool>;
    auto emplace(Args&&...) -> pair<iterator, bool>;
    auto erase(const_iterator) -> iterator;
    auto erase(const K&) -> size_type;
    auto find(const K&) const -> const_iterator;
    auto contains(const K&) const -> bool;
    auto extract(const_iterator) -> value_type;
};
```

---

## 3. Thư viện network (`libs/network/`)

### 3.1. `PacketType.hpp`

```cpp
enum class PacketType : u8 {
    account_auth          = 0,   // Xác thực
    account_create        = 1,   // Tạo tài khoản
    balance_req           = 2,   // Yêu cầu số dư
    balance_change        = 3,   // Thay đổi số dư (nạp/rút)
    balance_transfer      = 4,   // Chuyển khoản
    ping                  = 5,   // Kiểm tra kết nối
    account_change_password = 6, // Đổi mật khẩu
    account_toggle        = 7,   // Khoá/Mở khoá
};
```

### 3.2. `Packet.hpp` / `Packet.cpp` — Namespace `packet`

```cpp
namespace packet {
    inline constexpr usize HeaderSize = 5;  // 4 byte length + 1 byte type
    inline constexpr usize LengthSize = 4;
    inline constexpr usize TypeSize   = 1;

    // Blocking I/O
    auto send(asio::ip::tcp::socket& socket, PacketType type,
              const std::string& data) -> void;
    auto recv(asio::ip::tcp::socket& socket)
        -> std::pair<PacketType, std::string>;

    // Async I/O
    auto asyncSend(asio::ip::tcp::socket& socket, PacketType type,
                   const std::string& data,
                   std::function<void(std::error_code)> handler) -> void;
    auto asyncRecv(asio::ip::tcp::socket& socket,
                   std::function<void(std::error_code, PacketType, std::string)> handler) -> void;

    // Byte ordering
    auto toNetwork(u32 val) noexcept -> u32;  // host → big-endian
}
```

**Định dạng packet trên wire:**

```
┌─────────────────┬──────────────┬────────────────────────┐
│  4 byte         │  1 byte      │  n byte                │
│  length (BE)    │  type        │  payload (UTF-8)       │
│  = data.size()  │  PacketType  │  VD: "admin:admin123"  │
└─────────────────┴──────────────┴────────────────────────┘
```

**Cơ chế asyncRecv (2 giai đoạn):**

```
asyncRecv()
  ├─ async_read header (5 byte)
  │    └─ parse length (big-endian) + type
  │         └─ async_read body (length byte)
  │              └─ handler(ec, type, data)
```

**Cơ chế asyncSend:**

```
asyncSend(socket, type, data)
  ├─ Tạo header[5]: [4 byte BE length][1 byte type]
  ├─ async_write [header + body] (scattered buffers)
  └─ handler(ec)
```

`asyncSend` dùng `shared_ptr` để giữ header và body sống đến khi async_write hoàn thành.

---

## 4. Server (`server/`)

### 4.1. `main.cpp`

```cpp
auto main() -> int {
    Server server(8080);
    server.run();
}
```

### 4.2. `Server.hpp` / `Server.cpp` — Class Server

```cpp
struct Server {
    explicit Server(u16 port);   // Tạo acceptor, bắt đầu doAccept()
    void run();                  // _io.run() — blocking

private:
    void doAccept();             // async_accept → tạo Session

    asio::io_context _io;
    asio::ip::tcp::acceptor _acceptor;
    DbManager _db;
};
```

**Server::doAccept():**
```
doAccept()
  └─ async_accept
       └─ connection mới → make_shared<Session>(socket, db)->start()
       └─ doAccept()  ← lặp lại
```

### 4.3. Class Session

```cpp
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(asio::ip::tcp::socket socket, DbManager &db);
    void start();

private:
    void doRead();              // asyncRecv → parse → dispatch
    void doWrite(u8 type, const std::string &response);  // asyncSend → doRead

    asio::ip::tcp::socket _socket;
    DbManager &_db;
    std::string _peer;          // Địa chỉ client (VD: "127.0.0.1:54321")
};
```

**Vòng đời Session:**

```
start()
  └─ doRead()
       └─ switch (PacketType) ...
            ├─ account_auth:    _db.authenticate() → doWrite("OK"/"FAIL")
             ├─ account_create:  _db.createAccount() → doWrite("OK"/"FAIL")
            ├─ balance_req:     _db.getBalance() → doWrite(số dư)
            ├─ balance_change:  _db.changeBalance() → doWrite("OK"/"FAIL")
            ├─ balance_transfer: _db.transferBalance() → doWrite("OK"/"FAIL")
            └─ ping:            doWrite("PONG")
       └─ doWrite()
            └─ asyncSend → doRead()  ← quay lại
```

### 4.4. `Database.hpp` / `Database.cpp` — DbManager

#### Struct Record (packed, 89 byte)

```cpp
#pragma pack(push, 1)
struct Record {
    char username[24];   // Tối đa 23 ký tự + null terminator
    char password[24];   // Tối đa 23 ký tự + null terminator
    u64  balance;
    char logFile[32];
    b8   isLocked;
};
#pragma pack(pop)
```

**Biểu diễn trong file data.db:**

```
[username 24B][password 24B][balance 8B BE][logFile 32B][isLocked 1B]
├──────────────────────────┴──────────────────────────────────────────┤
                          89 byte / record
```

#### Struct DependElementRecord (in-memory)

```cpp
struct DependElementRecord {
    char password[24];   // Tối đa 23 ký tự + null terminator
    u64  balance;
    char logFile[32];
    b8   isLocked;
};
```

#### Class DbManager

```cpp
struct DbManager {
    explicit DbManager(asio::io_context &io);
    ~DbManager();

    // Persistence
    auto load() -> void;   // Đọc data.db → _data map (gọi trong constructor)
    auto save() -> void;   // Ghi _data map → data.db (gọi trong destructor)

    // Operations (tất cả đều enqueue vào worker thread)
    auto authenticate(const std::string &username,
                      const std::string &password,
                      BoolCallback callback) -> void;
    auto createAccount(const std::string &username,
                       const std::string &password,
                       Callback callback) -> void;
    auto changePassword(const std::string &username,
                        const std::string &newPassword,
                        Callback callback) -> void;
    auto toggleAccount(const std::string &username,
                       Callback callback) -> void;
    auto getBalance(const std::string &username,
                    U64Callback callback) -> void;
    auto changeBalance(const std::string &username,
                       i64 change, BoolCallback callback) -> void;
    auto transferBalance(const std::string &sender,
                         const std::string &recipient,
                         u64 amount, BoolCallback callback) -> void;

    // Hằng số
    static constexpr u64 DEFAULT_BALANCE = 100'000;
};
```

#### WorkItem variant

```cpp
using WorkItem = std::variant<
    AuthOp,             // username, password, BoolCallback
    ChangePasswordOp,   // username, newPassword, Callback
    CreateAccountOp,    // username, password, Callback
    ToggleAccountOp,    // username, Callback
    GetBalanceOp,       // username, U64Callback
    ChangeBalanceOp,    // username, change, BoolCallback
    TransferBalanceOp   // sender, recipient, amount, BoolCallback
>;
```

**Cơ chế worker thread:**

```
Enqueue (gọi từ main thread):
  lock(mutex) → queue.push(op) → notify_one() → unlock

Dequeue + process (worker thread):
  wait(cv) → queue.pop() → processItem()
    └─ std::visit([...], item)
         └─ xử lý operation
         └─ asio::post(io_context, callback)
```

**Xử lý từng operation:**
- `AuthOp`: kiểm tra username tồn tại, không locked, password đúng
- `CreateAccountOp`: tạo mới với DEFAULT_BALANCE = 100000, logFile = username + ".log"
- `ChangeBalanceOp`: kiểm tra đủ tiền nếu rút, ghi log `data/<user>.log`
- `TransferBalanceOp`: kiểm tra 2 user tồn tại, đủ tiền, ghi log cả 2 bên

### 4.5. `ServerLog.hpp` / `ServerLog.cpp`

```cpp
struct ServerLog {
    static void info(const std::string &msg);      // [INFO]
    static void warn(const std::string &msg);      // [WARN]
    static void auth(const std::string &msg);      // [AUTH]
    static void create(const std::string &msg);    // [CREATE]
    static void balance(const std::string &msg);   // [BALANCE]
    static void transfer(const std::string &msg);  // [TRANSFER]
    static void password(const std::string &msg);  // [PASSWORD]
    static void toggle(const std::string &msg);    // [TOGGLE]
};
```

Định dạng: `[TAG] [YYYY-MM-DD HH:MM:SS] message`

---

## 5. Client (`app/`)

### 5.1. `main.cpp`

```cpp
auto main() -> int {
    App app;
    app.run();
}
```

### 5.2. `App.hpp` / `App.cpp`

```cpp
struct App {
    void run();
};
```

**App::run()** — khởi tạo toàn bộ client:

```
1. Client client("127.0.0.1", 8080)
2. Thread: client.run() → _io.run()
3. client.connect(callback)
4. Tạo LoginPage, DashboardPage, SignupPage
5. loginPage.setDashboard(dashboard)
6. signupPage.setDashboard(dashboard)
7. Container::Tab({login, dashboard, signup}, &page)  ← 3 page
8. screen.Loop(container) → vòng lặp UI
9. Escape → screen.Exit()
10. client.stop(), clientThread.join()
```

**Điều hướng page:**

| Giá trị `page` | Trang hiển thị |
|----------------|---------------|
| `0` | LoginPage |
| `1` | DashboardPage |
| `2` | SignupPage |

### 5.3. `client/Client.hpp` / `client/Client.cpp`

```cpp
struct Client {
    Client(const std::string &host, u16 port);
    ~Client();

    // Control
    void stop();
    void connect(BoolHandler handler);
    void run();             // _io.run() — blocking, chạy trên thread riêng

    // Các method mạng
    void authenticate(const std::string &username, const std::string &password,
                      BoolHandler handler);
    void createAccount(const std::string &username, const std::string &password,
                       BoolHandler handler);
    void getBalance(const std::string &username, U64Handler handler);
    void changeBalance(const std::string &username, i64 change,
                       BoolHandler handler);
    void transferBalance(const std::string &sender, const std::string &recipient,
                         u64 amount, BoolHandler handler);
    void changePassword(const std::string &username, const std::string &newPassword,
                        BoolHandler handler);
    void toggleAccount(const std::string &username, BoolHandler handler);

    using BoolHandler = std::function<void(bool)>;
    using U64Handler  = std::function<void(std::optional<u64>)>;
};
```

**Cơ chế nội bộ:**

```
sendRequest(type, payload, handler):
  asio::post(_io, ...) → chạy trên client thread
    ├─ _pending.push(handler)
    └─ packet::asyncSend(socket, type, payload)

recvLoop():
  packet::asyncRecv(socket, ...)
    └─ response → _pending.front() → pop → handler(true, data)
    └─ lỗi → drain hết _pending với handler(false, {})
```

**Biến thành viên private:**

```cpp
asio::io_context _io;
asio::executor_work_guard<...> _work;  // Giữ _io.run() sống
asio::ip::tcp::socket _socket;
std::string _host;
u16 _port;
std::atomic<bool> _connected{false};
std::queue<Handler> _pending;          // Queue handler FIFO
```

**Payload format từng method:**

| Method | Payload |
|--------|---------|
| authenticate | `"username:password"` |
| createAccount | `"username:password"` |
| getBalance | `"username"` |
| changeBalance | `"username:change"` |
| transferBalance | `"sender:recipient:amount"` |
| changePassword | `"username:newpassword"` |
| toggleAccount | `"username"` |

### 5.4. `pages/LoginPage.hpp` / `pages/LoginPage.cpp`

```cpp
struct LoginPage {
    LoginPage(Client &client, ftxui::ScreenInteractive &screen,
              std::string &appUsername, int &page);
    ftxui::Component build();
    void reset();
    void setDashboard(DashboardPage &dashboard);

private:
    void doLogin();
    void onLogin();

    Client &_client;
    ftxui::ScreenInteractive &_screen;
    DashboardPage *_dashboard = nullptr;
    std::string &_appUsername;
    int &_page;
    std::string _username;
    ftxui::Component _userInput;
    std::string _password;
    ftxui::Component _passInput;
    ftxui::Component _loginBtn;
    ftxui::Component _signupBtn;       // ← Nút chuyển sang SignupPage
    std::string _status;
    bool _loading = false;
};
```

**UI layout:**

```
┌────────────────────────┐
│         Login          │
├────────────────────────┤
│  Username: [input   ]  │
│  Password: [******* ]  │
├────────────────────────┤
│        [Login]         │
│       [Sign Up]        │  ← Chuyển đến SignupPage (page = 2)
│    Authentication failed│
└────────────────────────┘
```

**Luồng doLogin():**
1. Validate `_username` và `_password` không rỗng
2. Set `_loading = true`
3. Gọi `_client.authenticate()`
4. Callback chạy trên client thread → `_screen.Post()` đưa về main thread
5. Nếu OK: gán `_appUsername`, gọi `onLogin()` → `_dashboard->doRefresh()`, set `_page = 1`
6. Nếu FAIL: hiển thị `_status = "Authentication failed"`

### 5.5. `pages/DashboardPage.hpp` / `pages/DashboardPage.cpp`

```cpp
struct DashboardPage {
    DashboardPage(Client &client, ftxui::ScreenInteractive &screen,
                  const std::string &username, int &page,
                  LoginPage &loginPage);
    ftxui::Component build();
    void doRefresh();

private:
    void onLogout();

    Client &_client;
    ftxui::ScreenInteractive &_screen;
    const std::string &_username;
    int &_page;
    LoginPage &_loginPage;
    std::string _balanceStr;
    std::string _status;
};
```

**UI layout:**

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

**doRefresh():** Gọi `_client.getBalance()` → cập nhật `_balanceStr` qua `screen.Post()`.

**onLogout():** Reset LoginPage, set `_page = 0` (quay lại login).

### 5.6. `pages/SignupPage.hpp` / `pages/SignupPage.cpp`

```cpp
struct SignupPage {
    SignupPage(Client &client, ftxui::ScreenInteractive &screen,
               std::string &appUsername, int &page);
    ftxui::Component build();
    void setDashboard(DashboardPage &dashboard);

private:
    void doCreateAccount();
    void onSuccess();

    Client &_client;
    ftxui::ScreenInteractive &_screen;
    DashboardPage *_dashboard = nullptr;
    std::string &_appUsername;
    int &_page;
    std::string _username;
    ftxui::Component _userInput;
    std::string _password;
    ftxui::Component _passInput;
    std::string _confirmPassword;
    ftxui::Component _confirmInput;
    ftxui::Component _createBtn;
    std::string _status;
    bool _loading = false;
};
```

**UI layout:**

```
┌────────────────────────────┐
│       Create Account       │
├────────────────────────────┤
│  Username: [input        ] │
│  Password: [********     ] │
│  Confirm:  [********     ] │
├────────────────────────────┤
│  [Create Account] [Back]   │
│  Passwords do not match    │
└────────────────────────────┘
```

**Luồng doCreateAccount():**
1. Validate: không để trống, `_password == _confirmPassword`, `_username` ≤ 23 ký tự
2. Set `_loading = true`
3. Gọi `_client.createAccount()`
4. Callback chạy trên client thread → `_screen.Post()` đưa về main thread
5. Nếu OK: gán `_appUsername`, gọi `onSuccess()` → `_dashboard->doRefresh()`, set `_page = 1`
6. Nếu FAIL: hiển thị `_status = "Account creation failed (username may exist)"`

**Nút Back:** set `_page = 0` (quay về Login).

**Điểm khác biệt so với LoginPage:**
- Có thêm `_confirmPassword` + `_confirmInput` — xác nhận mật khẩu
- Validation client-side trước khi gửi request (tránh gửi request vô ích)
- Dùng `PacketType::account_create` thay vì `account_auth`
- Username và password bị giới hạn **23 ký tự** do `Record` dùng `char[24]` trong database

---

## 6. Tiện ích (`tests/`)

### `generate_test_data.cpp`

Tạo file `data.db` với 3 tài khoản mẫu (dùng `Record` struct giống hệt server).

```cpp
int main();
// Ghi 3 record:
//   "admin"/"admin123"   balance=999999  unlocked
//   "user1"/"password1"  balance=50000   unlocked
//   "locked"/"lockedpw"  balance=1000    locked
```

---

## 7. Cấu hình build (`CMakeLists.txt`)

```cmake
cmake_minimum_required(VERSION 4.2)
project(Study LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)

# Targets:
#   network   — static library (libs/network/Packet.cpp)
#   app       — executable (client UI, links: network + ftxui)
#   server    — executable (server, links: network)
#   generate_test_data — executable (utility)

# Includes: ${CMAKE_SOURCE_DIR} (để #include "libs/..." hoạt động)
```

**Lưu ý:** Các file header trong `libs/base/` không được khai báo trong CMakeLists (header-only, include qua đường dẫn `${CMAKE_SOURCE_DIR}`).
