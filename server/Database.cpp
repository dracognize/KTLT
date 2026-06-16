#include "server/Database.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
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
		const auto len = (std::min)(src.size(), N - 1);
		std::memcpy(dest, src.data(), len);
	}

} // namespace

DbManager::DbManager(asio::io_context &io) : _io(io) {
	load();
	_running = true;
	_worker = std::thread(&DbManager::dbLoop, this);
}

DbManager::~DbManager() {
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
	std::ifstream file(DATABASE_PATH, std::ios::binary);
	if (!file) {
		return;
	}

	Record record;
	while (file.read(reinterpret_cast<char *>(&record), sizeof(record))) {
		record.balance = fromNetwork(record.balance);
		auto username = asString(record.username);
		auto &entry = _data[username];
		copyFrom(asString(record.password), entry.password);
		entry.balance = record.balance;
		copyFrom(asString(record.logFile), entry.logFile);
		entry.isLocked = record.isLocked;
	}
}

auto DbManager::save() -> void {
	std::ofstream file(DATABASE_PATH, std::ios::binary | std::ios::trunc);
	if (!file) {
		return;
	}

	Record record{};
	for (const auto &[username, entry] : _data) {
		std::memset(&record, 0, sizeof(record));
		copyFrom(username, record.username);
		std::memcpy(record.password, entry.password, sizeof(record.password));
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
							   Callback callback) -> void {
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

auto DbManager::toggleAccount(const std::string &username, Callback callback) -> void {
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
				auto ok = asString(it->second.password) == op.password;
				post([cb = std::move(op.callback), ok] {
					if (cb)
						cb(ok);
				});
			} else if constexpr (std::is_same_v<T, ChangePasswordOp>) {
				auto it = _data.find(op.username);
				if (it != _data.end()) {
					copyFrom(op.newPassword, it->second.password);
				}
				post([cb = std::move(op.callback)] {
					if (cb)
						cb();
				});
			} else if constexpr (std::is_same_v<T, CreateAccountOp>) {
				if (!_data.contains(op.username)) {
					auto &entry = _data[op.username];
					copyFrom(op.password, entry.password);
					entry.balance = DEFAULT_BALANCE;
					copyFrom(op.username + ".log", entry.logFile);
					entry.isLocked = 0;
				}
				post([cb = std::move(op.callback)] {
					if (cb)
						cb();
				});
			} else if constexpr (std::is_same_v<T, ToggleAccountOp>) {
				auto it = _data.find(op.username);
				if (it != _data.end()) {
					it->second.isLocked = !it->second.isLocked;
				}
				post([cb = std::move(op.callback)] {
					if (cb)
						cb();
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
				if (it != _data.end()) {
					auto bal = static_cast<i64>(it->second.balance) + op.change;
					if (bal < 0)
						bal = 0;
					it->second.balance = static_cast<u64>(bal);
				}
				post([cb = std::move(op.callback)] {
					if (cb)
						cb();
				});
			} else if constexpr (std::is_same_v<T, TransferBalanceOp>) {
				auto src = _data.find(op.sender);
				auto dst = _data.find(op.recipient);
				if (src != _data.end() && dst != _data.end() && src->second.balance >= op.amount) {
					src->second.balance -= op.amount;
					dst->second.balance += op.amount;
				}
				post([cb = std::move(op.callback)] {
					if (cb)
						cb();
				});
			}
		},
		std::move(item));
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
