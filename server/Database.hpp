#pragma once

#include "libs/base/types.hpp"
#include "libs/base/vector.hpp"

#include <asio.hpp>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>

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
		u64 balance;
		char logFile[LogFileMaxLen + 1]; // 32
		b8 isLocked;
};

#pragma pack(pop)

static_assert(sizeof(Record) == 24 + 48 + 8 + 32 + 1, "Record size mismatch");

struct UserRecord {
		char password[PasswordStoreLen]; // 48 (16 salt + 32 hash)
		u64 balance;
		char logFile[LogFileMaxLen + 1]; // 32
		b8 isLocked;
};

// Loai giao dich, dung cho ca luu file (TransactionRecord.type) va API trong.
enum class TransactionType : u8 {
	deposit = 0,	  // nap tien (changeBalance voi change > 0)
	withdraw = 1,	  // rut tien (changeBalance voi change < 0)
	transfer_out = 2, // ben gui trong giao dich chuyen tien
	transfer_in = 3,  // ben nhan trong giao dich chuyen tien
};

#pragma pack(push, 1)
struct TransactionRecord {
		char username[24]; // tai khoan chu cua ban ghi nay (nguoi se thay ban ghi trong lich su cua
						   // minh)
		char counterparty[24]; // doi tac giao dich (nguoi gui/nhan), rong neu nap/rut
		u64 amount;
		u64 balanceAfter; // so du cua "username" SAU giao dich nay
		u64 timestamp;	  // unix epoch seconds
		u8 type;		  // gia tri cua TransactionType
};

#pragma pack(pop)

struct UserState {
		UserRecord record;
		base::Vector<TransactionRecord> transactions;
};

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
		using StringCallback = std::function<void(std::string)>;

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
						   BoolCallback callback) -> void;
		auto toggleAccount(const std::string &username, BoolCallback callback) -> void;
		auto getBalance(const std::string &username, U64Callback callback) -> void;
		auto changeBalance(const std::string &username, i64 change, BoolCallback callback) -> void;
		auto transferBalance(const std::string &sender,
							 const std::string &recipient,
							 u64 amount,
							 BoolCallback callback) -> void;
		auto getTransactions(const std::string &username,
							 u64 count,
							 StringCallback callback) -> void;
		auto searchUsers(const std::string &prefix, StringCallback callback) -> void;

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
				BoolCallback callback;
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
				BoolCallback callback;
		};

		struct TransferBalanceOp {
				std::string sender;
				std::string recipient;
				u64 amount;
				BoolCallback callback;
		};

		struct GetTxnHistoryOp {
				std::string username;
				u64 count;
				StringCallback callback;
		};

		struct SearchUsersOp {
				std::string prefix;
				StringCallback callback;
		};

		struct SaveOp {};

		using WorkItem = std::variant<AuthOp,
									  ChangePasswordOp,
									  CreateAccountOp,
									  ToggleAccountOp,
									  GetBalanceOp,
									  ChangeBalanceOp,
									  TransferBalanceOp,
									  GetTxnHistoryOp,
									  SearchUsersOp,
									  SaveOp>;

		static constexpr auto SaveInterval = std::chrono::seconds(30);

		asio::io_context &_io;
		std::map<std::string, UserState> _data;

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
