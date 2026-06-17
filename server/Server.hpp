#pragma once

#include "libs/base/types.hpp"
#include "server/Database.hpp"

#include <asio.hpp>
#include <memory>
#include <string>

class Session : public std::enable_shared_from_this<Session> {
	public:
		Session(asio::ip::tcp::socket socket, DbManager &db);
		void start();

	private:
		void doRead();
		void doWrite(u8 type, const std::string &response);

		asio::ip::tcp::socket _socket;
		DbManager &_db;
		std::string _peer;
};

struct Server {
		explicit Server(u16 port);
		void run();
<<<<<<< HEAD
=======
		void stop();
		auto ioContext() -> asio::io_context &;
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

	private:
		void doAccept();

		asio::io_context _io;
		asio::ip::tcp::acceptor _acceptor;
		DbManager _db;
};
