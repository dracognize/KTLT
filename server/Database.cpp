#include "server/Database.hpp"

#include "libs/base/sha256.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
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

	
	template <size_t N> auto copyBinary(std::span<const u8> src, char (&dest)[N]) -> void {
		std::memset(dest, 0, N);
		auto len = (std::min)(src.size(), N);
		std::memcpy(dest, src.data(), len);
	}

	
	[[nodiscard]] auto isSafePathComponent(std::string_view name) -> bool {
		return !name.empty() && name.size() <= UsernameMaxLen
			   && name.find('/') == std::string_view::npos
			   && name.find('\\') == std::string_view::npos
			   && name.find("..") == std::string_view::npos
			   && name.find('\0') == std::string_view::npos;
	}

	
	void hashPasswordInto(std::string_view password, char (&dest)[PasswordStoreLen]) {
		auto salt = base::generateSalt();
		auto hash = base::hashPassword(password, salt);
		
		
		std::array<u8, PasswordStoreLen> buf{};
		std::memcpy(buf.data(), salt.data(), PasswordSaltLen);
		auto rawHash = base::Sha256::hash(salt + ":" + std::string{password});
		std::memcpy(buf.data() + PasswordSaltLen, rawHash.data(), PasswordHashLen);
		copyBinary(std::span<const u8>(buf), dest);
	}

	
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

} 

DbManager::DbManager(asio::io_context &io) : _io(io), _saveTimer(io) {
	std::filesystem::create_directories(DataDir);
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
		std::memcpy(entry.record.password, record.password, PasswordStoreLen);
		entry.record.balance = record.balance;
		copyFrom(asString(record.logFile), entry.record.logFile);
		entry.record.isLocked = record.isLocked;
	}

	
	for (auto &[username, state] : _data) {
		auto txnPath = std::string{DataDir} + "/" + username + ".txn";
		std::ifstream txnFile(txnPath, std::ios::binary);
		if (!txnFile)
			continue;
		TransactionRecord txnRecord;
		while (txnFile.read(reinterpret_cast<char *>(&txnRecord), sizeof(txnRecord))) {
			txnRecord.amount = fromNetwork(txnRecord.amount);
			txnRecord.balanceAfter = fromNetwork(txnRecord.balanceAfter);
			txnRecord.timestamp = fromNetwork(txnRecord.timestamp);
			state.transactions.push_back(txnRecord);
		}
	}
}

auto DbManager::save() -> void {
	std::ofstream file(DatabasePath, std::ios::binary | std::ios::trunc);
	if (!file) {
		return;
	}

	Record record{};
	for (const auto &[username, state] : _data) {
		std::memset(&record, 0, sizeof(record));
		copyFrom(username, record.username);
		std::memcpy(record.password, state.record.password, PasswordStoreLen);
		record.balance = toNetwork(state.record.balance);
		std::memcpy(record.logFile, state.record.logFile, sizeof(record.logFile));
		record.isLocked = state.record.isLocked;
		file.write(reinterpret_cast<const char *>(&record), sizeof(record));
	}

	
	for (const auto &[username, state] : _data) {
		auto txnPath = std::string{DataDir} + "/" + username + ".txn";
		std::ofstream txnFile(txnPath, std::ios::binary | std::ios::trunc);
		if (!txnFile)
			continue;
		for (const auto &rec : state.transactions) {
			auto netRec = rec;
			netRec.amount = toNetwork(netRec.amount);
			netRec.balanceAfter = toNetwork(netRec.balanceAfter);
			netRec.timestamp = toNetwork(netRec.timestamp);
			txnFile.write(reinterpret_cast<const char *>(&netRec), sizeof(netRec));
		}
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
							  BoolCallback callback) -> void {
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

auto DbManager::changeBalance(const std::string &username, i64 change, BoolCallback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(ChangeBalanceOp{username, change, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::transferBalance(const std::string &sender,
								const std::string &recipient,
								u64 amount,
								BoolCallback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(TransferBalanceOp{sender, recipient, amount, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::getTransactions(const std::string &username,
								u64 count,
								StringCallback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(GetTxnHistoryOp{username, count, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::searchUsers(const std::string &prefix, StringCallback callback) -> void {
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.emplace(SearchUsersOp{prefix, std::move(callback)});
	_cv.notify_one();
}

auto DbManager::processItem(WorkItem &&item) -> void {
	auto post = [this](auto cb) { asio::post(_io, std::move(cb)); };

	std::visit(
		[this, &post](auto &&op) {
			using T = std::decay_t<decltype(op)>;

			if constexpr (std::is_same_v<T, AuthOp>) {
				auto it = _data.find(op.username);
				if (it == _data.end() || it->second.record.isLocked) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				auto ok = verifyPassword(op.password, it->second.record.password);
				post([cb = std::move(op.callback), ok] {
					if (cb)
						cb(ok);
				});
			} else if constexpr (std::is_same_v<T, ChangePasswordOp>) {
				auto it = _data.find(op.username);
				auto ok = false;
				if (it != _data.end()) {
					hashPasswordInto(op.newPassword, it->second.record.password);
					appendLog(asString(it->second.record.logFile), "Password changed");
					_dirty = true;
					ok = true;
				}
				post([cb = std::move(op.callback), ok] {
					if (cb)
						cb(ok);
				});
			} else if constexpr (std::is_same_v<T, CreateAccountOp>) {
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
					hashPasswordInto(op.password, entry.record.password);
					entry.record.balance = DefaultBalance;
					copyFrom(op.username + ".log", entry.record.logFile);
					entry.record.isLocked = 0;
					_dirty = true;
					appendLog(asString(entry.record.logFile),
							  "Account created with balance " + std::to_string(entry.record.balance));

					
					auto ts = std::chrono::duration_cast<std::chrono::seconds>(
						std::chrono::system_clock::now().time_since_epoch())
								  .count();
					TransactionRecord rec{};
					copyFrom(op.username, rec.username);
					
					rec.amount = DefaultBalance;
					rec.balanceAfter = entry.record.balance;
					rec.timestamp = static_cast<u64>(ts);
					rec.type = static_cast<u8>(TransactionType::deposit);
					entry.transactions.push_back(rec);
				}
				post([cb = std::move(op.callback), created] {
					if (cb)
						cb(created);
				});
			} else if constexpr (std::is_same_v<T, ToggleAccountOp>) {
				auto it = _data.find(op.username);
				auto ok = false;
				if (it != _data.end()) {
					it->second.record.isLocked = !it->second.record.isLocked;
					_dirty = true;
					auto status = it->second.record.isLocked ? "locked" : "unlocked";
					appendLog(asString(it->second.record.logFile), "Account " + std::string(status));
					ok = true;
				}
				post([cb = std::move(op.callback), ok] {
					if (cb)
						cb(ok);
				});
			} else if constexpr (std::is_same_v<T, GetBalanceOp>) {
				auto it = _data.find(op.username);
				auto bal = it != _data.end() ? it->second.record.balance : 0;
				post([cb = std::move(op.callback), bal] {
					if (cb)
						cb(bal);
				});
			} else if constexpr (std::is_same_v<T, ChangeBalanceOp>) {
				auto it = _data.find(op.username);
				if (it == _data.end() || it->second.record.isLocked) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				if (op.change < 0 && it->second.record.balance < static_cast<u64>(-op.change)) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				auto newBal = static_cast<i64>(it->second.record.balance) + op.change;
				it->second.record.balance = static_cast<u64>(newBal);
				_dirty = true;
				auto opName = op.change >= 0 ? "Deposit" : "Withdrawal";
				auto line = std::string{opName} + " " + (op.change >= 0 ? "+" : "")
							+ std::to_string(op.change) + " " + std::to_string(it->second.record.balance);
				appendLog(asString(it->second.record.logFile), line);

				
				{
					auto ts = std::chrono::duration_cast<std::chrono::seconds>(
						std::chrono::system_clock::now().time_since_epoch())
								  .count();
					TransactionRecord rec{};
					copyFrom(op.username, rec.username);
					
					rec.amount = static_cast<u64>(op.change < 0 ? -op.change : op.change);
					rec.balanceAfter = it->second.record.balance;
					rec.timestamp = static_cast<u64>(ts);
					rec.type = op.change >= 0 ? static_cast<u8>(TransactionType::deposit)
											  : static_cast<u8>(TransactionType::withdraw);
					it->second.transactions.push_back(rec);
				}
				post([cb = std::move(op.callback)] {
					if (cb)
						cb(true);
				});
			} else if constexpr (std::is_same_v<T, TransferBalanceOp>) {
				auto src = _data.find(op.sender);
				auto dst = _data.find(op.recipient);
				if (src == _data.end() || dst == _data.end() || src->second.record.isLocked
					|| dst->second.record.isLocked || src->second.record.balance < op.amount) {
					post([cb = std::move(op.callback)] {
						if (cb)
							cb(false);
					});
					return;
				}
				src->second.record.balance -= op.amount;
				dst->second.record.balance += op.amount;
				_dirty = true;
				appendLog(asString(src->second.record.logFile),
						  "Transfer to " + op.recipient + " -" + std::to_string(op.amount) + " "
							  + std::to_string(src->second.record.balance));
				appendLog(asString(dst->second.record.logFile),
						  "Transfer from " + op.sender + " +" + std::to_string(op.amount) + " "
							  + std::to_string(dst->second.record.balance));

				
				{
					auto ts = std::chrono::duration_cast<std::chrono::seconds>(
						std::chrono::system_clock::now().time_since_epoch())
								  .count();

					TransactionRecord senderRec{};
					copyFrom(op.sender, senderRec.username);
					copyFrom(op.recipient, senderRec.counterparty);
					senderRec.amount = op.amount;
					senderRec.balanceAfter = src->second.record.balance;
					senderRec.timestamp = static_cast<u64>(ts);
					senderRec.type = static_cast<u8>(TransactionType::transfer_out);
					src->second.transactions.push_back(senderRec);

					TransactionRecord recipientRec{};
					copyFrom(op.recipient, recipientRec.username);
					copyFrom(op.sender, recipientRec.counterparty);
					recipientRec.amount = op.amount;
					recipientRec.balanceAfter = dst->second.record.balance;
					recipientRec.timestamp = static_cast<u64>(ts);
					recipientRec.type = static_cast<u8>(TransactionType::transfer_in);
					dst->second.transactions.push_back(recipientRec);
				}
				post([cb = std::move(op.callback)] {
					if (cb)
						cb(true);
				});
			} else if constexpr (std::is_same_v<T, GetTxnHistoryOp>) {
				auto it = _data.find(op.username);
				std::string result;
				if (it != _data.end()) {
					auto &txns = it->second.transactions;
					auto start = txns.size() > op.count ? txns.size() - op.count : 0;
					for (auto i = start; i < txns.size(); ++i) {
						const auto &rec = txns[i];
						result += std::to_string(static_cast<int>(rec.type)) + "|";
						result += asString(rec.counterparty) + "|";
						result += std::to_string(rec.amount) + "|";
						result += std::to_string(rec.balanceAfter) + "|";
						result += std::to_string(rec.timestamp) + "\n";
					}
				}
				post([cb = std::move(op.callback), result = std::move(result)] {
					if (cb)
						cb(result);
				});
			} else if constexpr (std::is_same_v<T, SearchUsersOp>) {
				std::string result;
				for (const auto &[username, _] : _data) {
					if (op.prefix.empty()
						|| username.starts_with(op.prefix)) {
						if (!result.empty())
							result += '|';
						result += username;
					}
				}
				post([cb = std::move(op.callback), result = std::move(result)] {
					if (cb)
						cb(result);
				});
			} else if constexpr (std::is_same_v<T, SaveOp>) {
				if (_dirty) {
					save();
					_dirty = false;
				}
				
			}
		},
		std::move(item));
}

auto DbManager::onSaveTimer(std::error_code ec) -> void {
	if (ec) {
		
		return;
	}
	
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_queue.emplace(SaveOp{});
	}
	_cv.notify_one();
	
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
