#pragma once

#include "libs/base/types.hpp"
#include "libs/network/PacketType.hpp"

#include <asio.hpp>
#include <atomic>
#include <queue>
#include <functional>
#include <optional>
#include <string>

struct Client {
		Client(const std::string &host, u16 port);
		~Client();

		Client(const Client &) = delete;
		Client(Client &&) = delete;
		auto operator=(const Client &) -> Client & = delete;
		auto operator=(Client &&) -> Client & = delete;

		using BoolHandler = std::function<void(bool)>;
		using U64Handler = std::function<void(std::optional<u64>)>;
		using StringHandler = std::function<void(std::optional<std::string>)>;
		using VoidHandler = std::function<void()>;

		void stop();
		void connect(BoolHandler handler);
		void run();

		void onDisconnect(VoidHandler handler);

		void
		authenticate(const std::string &username, const std::string &password, BoolHandler handler);
		void createAccount(const std::string &username,
						   const std::string &password,
						   BoolHandler handler);
		void getBalance(const std::string &username, U64Handler handler);
		void changeBalance(const std::string &username, i64 change, BoolHandler handler);
		void transferBalance(const std::string &sender,
							 const std::string &recipient,
							 u64 amount,
							 BoolHandler handler);
		void changePassword(const std::string &username,
							const std::string &newPassword,
							BoolHandler handler);
		void toggleAccount(const std::string &username, BoolHandler handler);
		void ping(BoolHandler handler);
		void getLogs(const std::string &username, StringHandler handler);
		void getTransactionHistory(const std::string &username, u64 count, StringHandler handler);
		void searchUsers(const std::string &prefix, StringHandler handler);

	private:
		using Handler = std::function<void(bool, std::string)>;

		void doConnect();
		void sendRequest(PacketType type, const std::string &payload, Handler handler);
		void recvLoop();

		asio::io_context _io;
		asio::executor_work_guard<asio::io_context::executor_type> _work;
		asio::ip::tcp::socket _socket;
		std::string _host;
		u16 _port;
		std::atomic<bool> _connected{false};
		std::queue<Handler> _pending;
		VoidHandler _disconnectHandler;
};
