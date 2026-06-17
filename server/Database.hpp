#pragma once

#include "libs/base/hash_map.hpp"
#include "libs/base/types.hpp"

#include <asio.hpp>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>

<<<<<<< HEAD
namespace base {
	// base::hash<T> mac dinh yeu cau T trivially_copyable (xem hash.hpp).
	// std::string khong thoa man dieu do, nen can specialization rieng
	// de base::HashMap<std::string, V> compile va hoat dong dung.
	template <>
	struct hash<std::string> {
			[[nodiscard]] usize operator()(const std::string& key) const noexcept {
				return murmur::hash64(key.data(), static_cast<i32>(key.size()));
			}
	};
} // namespace base

#pragma pack(push, 1)
struct Record {
		char username[24];
		char password[24];
=======
// Password security: store salt (16 bytes) + SHA-256 binary hash (32 bytes) = 48 bytes
inline constexpr usize UsernameMaxLen = 23;
inline constexpr usize PasswordSaltLen = 16;
inline constexpr usize PasswordHashLen = 32;
inline constexpr usize PasswordStoreLen = PasswordSaltLen + PasswordHashLen; // 48
inline constexpr usize LogFileMaxLen = 31;
inline constexpr usize DefaultBalance = 100'000;
inline constexpr const char *DataDir = "data";
inline constexpr const char *DatabasePath = "data.db";

#pragma pack(push, 1)
struct Record {
		char username[UsernameMaxLen + 1]; // 24
		char password[PasswordStoreLen];   // 48 (16 salt + 32 hash)
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
		u64 balance;
		char logFile[LogFileMaxLen + 1]; // 32
		b8 isLocked;
};

#pragma pack(pop)

<<<<<<< HEAD
struct DependElementRecord {
		char password[24];
=======
static_assert(sizeof(Record) == 24 + 48 + 8 + 32 + 1, "Record size mismatch");

struct UserRecord {
		char password[PasswordStoreLen]; // 48 (16 salt + 32 hash)
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
		u64 balance;
		char logFile[LogFileMaxLen + 1]; // 32
		b8 isLocked;
};

// Loai giao dich, dung cho ca luu file (TransactionRecord.type) va API trong.
enum class TransactionType : u8 {
	deposit       = 0, // nap tien (changeBalance voi change > 0)
	withdraw      = 1, // rut tien (changeBalance voi change < 0)
	transfer_out  = 2, // ben gui trong giao dich chuyen tien
	transfer_in   = 3, // ben nhan trong giao dich chuyen tien
};
 
#pragma pack(push, 1)
struct TransactionRecord {
		char username[24];      // tai khoan chu cua ban ghi nay (nguoi se thay ban ghi trong lich su cua minh)
		char counterparty[24];  // doi tac giao dich (nguoi gui/nhan), rong neu nap/rut
		u64  amount;
		u64  balanceAfter;      // so du cua "username" SAU giao dich nay
		u64  timestamp;         // unix epoch seconds
		u8   type;              // gia tri cua TransactionType
};

#pragma pack(pop)

struct DbManager {
		explicit DbManager(asio::io_context &io);
		~DbManager();

		DbManager(const DbManager &) = delete;
		DbManager(DbManager &&) = delete;
		auto operator=(const DbManager &) -> DbManager & = delete;
		auto operator=(DbManager &&) -> DbManager & = delete;

		using Callback = std::function<void()>;
		using BoolCallback = std::function<void(b1)>;
		using U64Callback = std::function<void(u64)>;

		auto load() -> void;
		auto save() -> void;

		auto authenticate(const std::string &username,
						  const std::string &password,
						  BoolCallback callback) -> void;
		auto changePassword(const std::string &username,
							const std::string &newPassword,
							BoolCallback callback) -> void;
		auto createAccount(const std::string &username,
						   const std::string &password,
<<<<<<< HEAD
						   Callback callback) -> void;
		auto toggleAccount(const std::string &username, Callback callback) -> void;
=======
						   BoolCallback callback) -> void;
		auto toggleAccount(const std::string &username, BoolCallback callback) -> void;
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
		auto getBalance(const std::string &username, U64Callback callback) -> void;
		auto changeBalance(const std::string &username, i64 change, Callback callback) -> void;
		auto transferBalance(const std::string &sender,
							 const std::string &recipient,
							 u64 amount,
							 Callback callback) -> void;

	private:
		struct AuthOp {
				std::string username;
				std::string password;
				BoolCallback callback;
		};

		struct ChangePasswordOp {
				std::string username;
				std::string newPassword;
				BoolCallback callback;
		};

		struct CreateAccountOp {
				std::string username;
				std::string password;
				Callback callback;
		};

		struct ToggleAccountOp {
				std::string username;
				BoolCallback callback;
		};

		struct GetBalanceOp {
				std::string username;
				U64Callback callback;
		};

		struct ChangeBalanceOp {
				std::string username;
				i64 change;
				Callback callback;
		};

		struct TransferBalanceOp {
				std::string sender;
				std::string recipient;
				u64 amount;
				Callback callback;
		};

		struct SaveOp {};

		using WorkItem = std::variant<AuthOp,
									  ChangePasswordOp,
									  CreateAccountOp,
									  ToggleAccountOp,
									  GetBalanceOp,
									  ChangeBalanceOp,
									  TransferBalanceOp,
									  SaveOp>;

		static constexpr auto SaveInterval = std::chrono::seconds(30);

		asio::io_context &_io;
<<<<<<< HEAD
		base::HashMap<std::string, DependElementRecord> _data;
=======
		std::map<std::string, UserRecord> _data;
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

		std::mutex _mutex;
		bool _dirty = false;

		std::queue<WorkItem> _queue;
		std::condition_variable _cv;
		std::thread _worker;
		bool _running = false;

		asio::steady_timer _saveTimer;

		void processItem(WorkItem &&item);
		void dbLoop();
		void onSaveTimer(std::error_code ec);
};
