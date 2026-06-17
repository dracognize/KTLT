#include "server/Database.hpp"

#include "libs/base/sha256.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
<<<<<<< HEAD
=======
#include <filesystem>
#include <format>
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
#include <fstream>

namespace {

	[[nodiscard]] auto toNetwork(u64 val) noexcept -> u64 {
		if constexpr (std::endian::native == std::endian::little) {
			return std::byteswap(val);
		}
		return val;
	}

	[[nodiscard]] auto fromNetwork(u64 val) noexcept -> u64 {
		if constexpr (std::endian::native == std::endian::little) {
			return std::byteswap(val);
		}
		return val;
	}

	template <size_t N> [[nodiscard]] auto asString(const char (&buf)[N]) -> std::string {
		auto end = std::find(buf, buf + N, '\0');
		return std::string(buf, end);
	}

	template <size_t N> auto copyFrom(std::string_view src, char (&dest)[N]) -> void {
		std::memset(dest, 0, N);
		auto len = (std::min)(src.size(), N - 1);
		std::memcpy(dest, src.data(), len);
	}

<<<<<<< HEAD
} // namespace

DbManager::DbManager(asio::io_context &io) : _io(io) {
=======
	// Copy binary data into a fixed-size buffer (no null-termination assumed)
	template <size_t N> auto copyBinary(std::span<const u8> src, char (&dest)[N]) -> void {
		std::memset(dest, 0, N);
		auto len = (std::min)(src.size(), N);
		std::memcpy(dest, src.data(), len);
	}

	// Validate that a username does not contain path traversal characters
	[[nodiscard]] auto isSafePathComponent(std::string_view name) -> bool {
		return !name.empty() && name.size() <= UsernameMaxLen
			   && name.find('/') == std::string_view::npos
			   && name.find('\\') == std::string_view::npos
			   && name.find("..") == std::string_view::npos
			   && name.find('\0') == std::string_view::npos;
	}

	// Store salt + SHA-256(salt + ":" + password) into a password buffer
	void hashPasswordInto(std::string_view password, char (&dest)[PasswordStoreLen]) {
		auto salt = base::generateSalt();
		auto hash = base::hashPassword(password, salt);
		// salt is 16 chars, hash is 64 hex chars
		// Store as binary: first 16 bytes = salt chars, next 32 bytes = binary hash
		std::array<u8, PasswordStoreLen> buf{};
		std::memcpy(buf.data(), salt.data(), PasswordSaltLen);
		auto rawHash = base::Sha256::hash(salt + ":" + std::string{password});
		std::memcpy(buf.data() + PasswordSaltLen, rawHash.data(), PasswordHashLen);
		copyBinary(std::span<const u8>(buf), dest);
	}

	// Verify a password against a stored salt+hash buffer
	[[nodiscard]] auto verifyPassword(std::string_view password,
									  const char (&stored)[PasswordStoreLen]) -> bool {
		std::string salt(stored, strnlen(stored, PasswordSaltLen));
		if (salt.size() != PasswordSaltLen) {
			return false;
		}
		auto expected = base::Sha256::hash(salt + ":" + std::string{password});
		auto actual = std::span<const u8>(reinterpret_cast<const u8 *>(stored + PasswordSaltLen),
										  PasswordHashLen);
		return std::memcmp(expected.data(), actual.data(), PasswordHashLen) == 0;
	}

	void appendLog(std::string_view logFile, std::string_view line) {
		auto path = std::string{DataDir} + "/" + std::string{logFile};
		if (auto f = std::ofstream(path, std::ios::app)) {
			auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
			f << std::format("[{:%F %T}] {}\n", now, line);
		}
	}

} // namespace

DbManager::DbManager(asio::io_context &io) : _io(io), _saveTimer(io) {
	std::filesystem::create_directories(DataDir);
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
	load();
	_running = true;
	_worker = std::thread(&DbManager::dbLoop, this);
	_saveTimer.expires_after(SaveInterval);
	_saveTimer.async_wait([this](std::error_code ec) { onSaveTimer(ec); });
}

DbManager::~DbManager() {
	_saveTimer.cancel();
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_running = false;
	}
	_cv.notify_one();
	if (_worker.joinable()) {
		_worker.join();
	}
	save();
}

auto DbManager::load() -> void {
	std::ifstream file(DatabasePath, std::ios::binary);
	if (!file) {
		return;
	}

	Record record;
	while (file.read(reinterpret_cast<char *>(&record), sizeof(record))) {
		record.balance = fromNetwork(record.balance);
		auto username = asString(record.username);
		auto &entry = _data[username];
		std::memcpy(entry.password, record.password, PasswordStoreLen);
		entry.balance = record.balance;
		copyFrom(asString(record.logFile), entry.logFile);
		entry.isLocked = record.isLocked;
	}
}

auto DbManager::save() -> void {
	std::ofstream file(DatabasePath, std::ios::binary | std::ios::trunc);
	if (!file) {
		return;
	}

	Record record{};
	for (const auto &[username, entry] : _data) {
		std::memset(&record, 0, sizeof(record));
		copyFrom(username, record.username);
		std::memcpy(record.password, entry.password, PasswordStoreLen);
		record.balance = toNetwork(entry.balance);
		std::memcpy(record.logFile, entry.logFile, sizeof(record.logFile));
		record.isLocked = entry.isLocked;
		file.write(reinterpret_cast<const char *>(&record), sizeof(record));
	}
}

auto DbManager::authenticate(const std::string &username,
							 const std::string &password,
							 BoolCallback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(AuthOp{username, password, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::changePassword(const std::string &username,
							   const std::string &newPassword,
							   BoolCallback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(ChangePasswordOp{username, newPassword, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::createAccount(const std::string &username,
							  const std::string &password,
							  Callback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(CreateAccountOp{username, password, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::toggleAccount(const std::string &username, BoolCallback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(ToggleAccountOp{username, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::getBalance(const std::string &username, U64Callback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(GetBalanceOp{username, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::changeBalance(const std::string &username, i64 change, Callback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(ChangeBalanceOp{username, change, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::transferBalance(const std::string &sender,
								const std::string &recipient,
								u64 amount,
								Callback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(TransferBalanceOp{sender, recipient, amount, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::processItem(WorkItem &&item) -> void {
	auto post = [this](auto cb) { asio::post(_io, std::move(cb)); };

	std::visit(
		[this, &post](auto &&op) {
			using T = std::decay_t<decltype(op)>;

			if constexpr (std::is_same_v<T, AuthOp>) {
				auto it = _data.find(op.username);
				if (it == _data.end() || it->second.isLocked) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				auto ok = verifyPassword(op.password, it->second.password);
				post([cb = std::move(op.callback), ok] {
					if (cb)
						cb(ok);
				});
			} else if constexpr (std::is_same_v<T, ChangePasswordOp>) {
				auto it = _data.find(op.username);
				auto ok = false;
				if (it != _data.end()) {
					hashPasswordInto(op.newPassword, it->second.password);
					appendLog(asString(it->second.logFile), "Password changed");
					_dirty = true;
					ok = true;
				}
				post([cb = std::move(op.callback), ok] {
					if (cb)
						cb(ok);
				});
			} else if constexpr (std::is_same_v<T, CreateAccountOp>) {
<<<<<<< HEAD
				if (!_data.contains(op.username)) {
					auto &entry = _data[op.username];
					copyFrom(op.password, entry.password);
					entry.balance = DEFAULT_BALANCE;
					copyFrom(op.username + ".log", entry.logFile);
					entry.isLocked = 0;
				}
				post([cb = std::move(op.callback)] {
=======
				if (!isSafePathComponent(op.username)) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				auto created = !_data.contains(op.username);
				if (created) {
					auto &entry = _data[op.username];
					hashPasswordInto(op.password, entry.password);
					entry.balance = DefaultBalance;
					copyFrom(op.username + ".log", entry.logFile);
					entry.isLocked = 0;
					_dirty = true;
					appendLog(asString(entry.logFile),
							  "Account created with balance " + std::to_string(entry.balance));
				}
				post([cb = std::move(op.callback), created] {
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
					if (cb)
						cb();
				});
			} else if constexpr (std::is_same_v<T, ToggleAccountOp>) {
				auto it = _data.find(op.username);
				auto ok = false;
				if (it != _data.end()) {
					it->second.isLocked = !it->second.isLocked;
					_dirty = true;
					auto status = it->second.isLocked ? "locked" : "unlocked";
					appendLog(asString(it->second.logFile), "Account " + std::string(status));
					ok = true;
				}
				post([cb = std::move(op.callback), ok] {
					if (cb)
						cb(ok);
				});
			} else if constexpr (std::is_same_v<T, GetBalanceOp>) {
				auto it = _data.find(op.username);
				auto bal = it != _data.end() ? it->second.balance : 0;
				post([cb = std::move(op.callback), bal] {
					if (cb)
						cb(bal);
				});
			} else if constexpr (std::is_same_v<T, ChangeBalanceOp>) {
				auto it = _data.find(op.username);
<<<<<<< HEAD
				if (it != _data.end()) {
					auto bal = static_cast<i64>(it->second.balance) + op.change;
					if (bal < 0)
						bal = 0;
					it->second.balance = static_cast<u64>(bal);
				}
=======
				if (it == _data.end() || it->second.isLocked) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				if (op.change < 0 && it->second.balance < static_cast<u64>(-op.change)) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				auto newBal = static_cast<i64>(it->second.balance) + op.change;
				it->second.balance = static_cast<u64>(newBal);
				_dirty = true;
				auto opName = op.change >= 0 ? "Deposit" : "Withdrawal";
				auto line = std::string{opName} + " " + (op.change >= 0 ? "+" : "")
							+ std::to_string(op.change) + " " + std::to_string(it->second.balance);
				appendLog(asString(it->second.logFile), line);
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
				post([cb = std::move(op.callback)] {
					if (cb)
						cb();
				});
			} else if constexpr (std::is_same_v<T, TransferBalanceOp>) {
				auto src = _data.find(op.sender);
				auto dst = _data.find(op.recipient);
<<<<<<< HEAD
				if (src != _data.end() && dst != _data.end() && src->second.balance >= op.amount) {
					src->second.balance -= op.amount;
					dst->second.balance += op.amount;
				}
=======
				if (src == _data.end() || dst == _data.end() || src->second.isLocked
					|| dst->second.isLocked || src->second.balance < op.amount) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				src->second.balance -= op.amount;
				dst->second.balance += op.amount;
				_dirty = true;
				appendLog(asString(src->second.logFile),
						  "Transfer to " + op.recipient + " -" + std::to_string(op.amount) + " "
							  + std::to_string(src->second.balance));
				appendLog(asString(dst->second.logFile),
						  "Transfer from " + op.sender + " +" + std::to_string(op.amount) + " "
							  + std::to_string(dst->second.balance));
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
				post([cb = std::move(op.callback)] {
					if (cb)
						cb();
				});
			} else if constexpr (std::is_same_v<T, SaveOp>) {
				if (_dirty) {
					save();
					_dirty = false;
				}
				// No callback for SaveOp
			}
		},
		std::move(item));
}

auto DbManager::onSaveTimer(std::error_code ec) -> void {
	if (ec) {
		// Timer cancelled or error — stop
		return;
	}
	// Queue a save operation (runs on worker thread)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_queue.emplace(SaveOp{});
	}
	_cv.notify_one();
	// Reschedule
	_saveTimer.expires_after(SaveInterval);
	_saveTimer.async_wait([this](std::error_code ec2) { onSaveTimer(ec2); });
}

auto DbManager::dbLoop() -> void {
	while (true) {
		WorkItem item;
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_cv.wait(lock, [this] { return !_queue.empty() || !_running; });
			if (!_running && _queue.empty()) {
				break;
			}
			item = std::move(_queue.front());
			_queue.pop();
		}
		processItem(std::move(item));
	}
}
