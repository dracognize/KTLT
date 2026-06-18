# Network Library Reference

The `libs/network/` directory implements a custom binary communication protocol for the Banking Terminal. It provides both synchronous and asynchronous APIs for sending and receiving packets over TCP/IP using **ASIO**.

## Protocol Specification

The protocol uses a fixed-size header followed by a variable-size body.

### Packet Structure
| Offset | Size | Field | Description |
| :--- | :--- | :--- | :--- |
| 0 | 4 bytes | `Length` | Size of the payload (in bytes). Encoded in **Network Byte Order** (Big-Endian). |
| 4 | 1 byte | `Type` | The type of the packet (maps to `PacketType` enum). |
| 5 | `Length` bytes | `Payload` | The actual data content of the packet. |

### Packet Types (`PacketType`)
Defined in `libs/network/PacketType.hpp`:
- `account_auth` (0): Authentication request.
- `account_create` (1): New account creation.
- `balance_req` (2): Balance inquiry.
- `balance_change` (3): Deposit or withdrawal.
- `balance_transfer` (4): Money transfer between users.
- `ping` (5): Keep-alive / Connection check.
- `account_change_password` (6): Password update.
- `account_toggle` (7): Lock/Unlock account.
- `log_req` (8): Server log request.
- `txn_history_req` (9): Transaction history request.
- `user_search_req` (10): User search (by prefix).

## API Reference

### Core Functions
- **`packet::send(socket, type, data)`**: Synchronously sends a packet.
- **`packet::recv(socket)`**: Synchronously receives a packet, returning a `std::pair<PacketType, std::string>`.
- **`packet::asyncSend(socket, type, data, handler)`**: Asynchronously sends a packet.
- **`packet::asyncRecv(socket, handler)`**: Asynchronously receives a packet.

### Utilities
- **`packet::toNetwork(u32)`**: Converts a 32-bit unsigned integer to network byte order.
- **`packet::HeaderSize`**: Constant (5 bytes).

## Implementation Details
- **Header Formatting**: Uses `std::memcpy` and `std::byteswap` (C++23) to ensure platform-independent binary representation.
- **Asynchronous Operations**: Uses `asio::async_read` and `asio::async_write` with shared memory pointers (`std::make_shared`) to manage the lifecycle of buffers during async operations.
- **Buffer Management**: Combines header and body into a single `asio::write` operation using a vector of buffers (`asio::const_buffer`) to minimize system calls.
