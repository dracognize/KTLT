#pragma once

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

#pragma pack(push, 1)
struct Record {
		char username[24];
		char password[24];
		u64 balance;
		char logFile[32];
		b8 isLocked;
};
#pragma pack(pop)

struct DependElementRecord {
		char password[24];
		u64 balance;
		char logFile[32];
		b8 isLocked;
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

		auto load() -> void;
		auto save() -> void;

		auto authenticate(const std::string &username,
						  const std::string &password,
						  BoolCallback callback) -> void;
		auto changePassword(const std::string &username,
							const std::string &newPassword,
							Callback callback) -> void;
		auto createAccount(const std::string &username,
						   const std::string &password,
						   Callback callback) -> void;
		auto toggleAccount(const std::string &username, Callback callback) -> void;
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
				Callback callback;
		};

		struct CreateAccountOp {
				std::string username;
				std::string password;
				Callback callback;
		};

		struct ToggleAccountOp {
				std::string username;
				Callback callback;
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

		using WorkItem = std::variant<AuthOp,
									  ChangePasswordOp,
									  CreateAccountOp,
									  ToggleAccountOp,
									  GetBalanceOp,
									  ChangeBalanceOp,
									  TransferBalanceOp>;

		static constexpr const char *DATABASE_PATH = "data.db";
		static constexpr u64 DEFAULT_BALANCE = 100'000;

		asio::io_context &_io;
		std::map<std::string, DependElementRecord> _data;

		std::mutex _mutex;

		std::queue<WorkItem> _queue;
		std::condition_variable _cv;
		std::thread _worker;
		bool _running = false;

		void processItem(WorkItem &&item);
		void dbLoop();
};
