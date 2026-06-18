# Server and Database Reference

The `server/` directory contains the core logic for the Banking Terminal server, including connection management, request processing, and persistent data storage.

## Server Architecture (`server/Server.cpp`)

The server is built on **ASIO** and follows an asynchronous, non-blocking event-driven model.

### `Server` Class
- **Purpose**: Manages the listening socket and accepts new client connections.
- **Key Methods**:
  - `run()`: Starts the ASIO event loop.
  - `doAccept()`: Continuously listens for incoming TCP connections.
  - `stop()`: Gracefully shuts down the server.

### `Session` Class
- **Purpose**: Represents a single active client connection.
- **Key Features**:
  - **Shared Ownership**: Uses `std::enable_shared_from_this` to manage its lifecycle during asynchronous operations.
  - **Request-Response Loop**: Implements a continuous `doRead()` -> `process` -> `doWrite()` cycle.
  - **Packet Dispatching**: Uses a `switch` statement on `PacketType` to route requests to the appropriate `DbManager` method.

## Database Management (`server/Database.cpp`)

The server uses a custom, flat-file database system optimized for the banking application's specific record types.

### `DbManager` Class
- **Asynchronous Design**: All database operations are non-blocking. Requests are added to a `std::queue<WorkItem>` and processed by a dedicated worker thread.
- **Thread Safety**: Uses a `std::mutex` and `std::condition_variable` to synchronize access between the networking threads and the database worker thread.
- **Persistence**: 
  - **Account Data**: Stored in a binary format in `data.db`.
  - **Transaction Logs**: Each user has an individual transaction log file in the `data/` directory (e.g., `data/username.log`).
  - **Auto-Save**: Periodically flushes memory-resident data to disk using a steady timer.

### Data Structures
- **`Record`**: Fixed-size binary structure for user account data (username, hashed password, balance, lock status).
- **`TransactionRecord`**: Binary structure for individual transactions (amount, timestamp, type, counterparty).
- **`UserState`**: In-memory representation of a user's account and their recent transactions.

### Security
- **Password Hashing**: Uses SHA-256 with a unique 16-byte salt for each user.
- **Input Validation**: Sanitizes file paths and input strings to prevent directory traversal and buffer overflow attacks.

## Logging System (`server/ServerLog.cpp`)

A dedicated logging utility that provides formatted output to the console for monitoring server activity.
- **Log Levels**: `INFO`, `WARN`, `AUTH`, `CREATE`, `BALANCE`, `TRANSFER`, `PASSWORD`.
- **Formatting**: Includes timestamps and peer IP information for every log entry.

## Transaction Types
Supported transactions include:
- `deposit`: Adding funds to an account.
- `withdraw`: Removing funds from an account.
- `transfer_out` / `transfer_in`: Moving funds between two registered accounts.
