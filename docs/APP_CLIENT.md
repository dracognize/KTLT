# Client Application Reference

The `app/` directory contains the source code for the Banking Terminal client, a TUI-based application using **FTXUI** and a custom asynchronous client logic.

## Client Core (`app/client/`)

### `Client` Class
The `Client` class handles all network communication with the server. It abstracts the raw packet protocol into high-level asynchronous methods.
- **Header**: `app/client/Client.hpp`
- **Key Features**:
  - **Asynchronous API**: Every request takes a callback handler (e.g., `BoolHandler`, `U64Handler`).
  - **Request Queueing**: Manages a queue of pending handlers to ensure responses are routed back to the correct request.
  - **Automatic Disconnect Handling**: Supports an `onDisconnect` hook to notify the UI when the connection is lost.
  - **Internal IO Context**: Runs its own `asio::io_context` in a dedicated thread to avoid blocking the UI.

## TUI Architecture (`app/pages/`)

The application uses a tab-based navigation system with two main states: **Pre-Authentication** and **Post-Authentication**.

### Component Wrapper: `NonFocusable`
A custom utility class that wraps FTXUI components to prevent them from participating in Tab-key focus cycling while still allowing mouse interactions.

### Pre-Authentication Pages
- **`LoginPage`**: Handles user authentication. Stores username and password locally during the attempt and updates a status message.
- **`SignupPage`**: Allows new users to register.

### Post-Authentication Pages
- **`DashboardPage`**: Displays current balance and recent activity.
- **`HistoryPage`**: Shows a detailed transaction history retrieved from the server.
- **`DepositPage`**: Interface for adding funds to the account.
- **`WithdrawPage`**: Interface for withdrawing funds.
- **`TransferPage`**: Allows transferring money to another user (supports user searching).
- **`SettingPage`**: Allows changing the user's password or toggling account status.

## UI Logic (`app/App.cpp`)

### Navigation Flow
1. **Startup**: Initializes the `Client` and attempts connection to `127.0.0.1:8080`.
2. **Pre-Auth**: The UI shows the Login/Signup tabs.
3. **Login Success**: Upon successful authentication, the `section` variable switches, revealing the main banking dashboard.
4. **Global Shortcuts**:
   - `ESC`: Exit application.
   - `Ctrl + Left/Right` or `[`/`]`: Switch between tabs.

### State Management
- `appUsername`: Stores the currently logged-in username.
- `section`: Toggles between Pre-Auth (0) and Post-Auth (1).
- `preAuthPage` / `postAuthPage`: Current active tab index within each section.

## Dependencies
- **FTXUI**: Used for rendering, layout, and event handling.
- **ASIO**: Used for the network communication layer within `Client`.
