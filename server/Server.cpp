#include "server/Database.hpp"
#include "server/ServerLog.hpp"

#include "libs/network/Packet.hpp"
#include "libs/network/PacketType.hpp"
#include "server/Server.hpp"

#include <utility>

Session::Session(asio::ip::tcp::socket socket, DbManager &db)
	: _socket(std::move(socket)), _db(db) {
	auto endpoint = _socket.remote_endpoint();
	_peer = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}

void Session::start() {
	ServerLog::info(_peer + " connected");
	doRead();
}

void Session::doRead() {
	auto self = shared_from_this();
	packet::asyncRecv(_socket, [this, self](std::error_code ec, PacketType type, std::string data) {
		if (ec) {
			ServerLog::info(_peer + " disconnected");
			return;
		}

		switch (type) {
			using enum PacketType;
		case account_auth: {
			auto delim = data.find(':');
			if (delim == std::string::npos) {
				ServerLog::warn( _peer + " malformed auth packet");
				doWrite(std::to_underlying(account_auth), "FAIL");
				return;
			}
			auto user = data.substr(0, delim);
			auto pass = data.substr(delim + 1);
			_db.authenticate(user, pass, [this, self, user](bool ok) {
				ServerLog::auth( _peer + " user '" + user + "' " + (ok ? "OK" : "FAIL"));
				doWrite(std::to_underlying(PacketType::account_auth), ok ? "OK" : "FAIL");
			});
			break;
		}
		case account_create: {
			auto delim = data.find(':');
			if (delim == std::string::npos) {
				doWrite(std::to_underlying(account_create), "FAIL");
				return;
			}
			auto user = data.substr(0, delim);
			auto pass = data.substr(delim + 1);
			_db.createAccount(user, pass, [this, self, user] {
				ServerLog::create( _peer + " user '" + user + "' created");
				doWrite(std::to_underlying(PacketType::account_create), "OK");
			});
			break;
		}
		case balance_req: {
			_db.getBalance(data, [this, self](u64 balance) {
				doWrite(std::to_underlying(PacketType::balance_resp), std::to_string(balance));
			});
			break;
		}
		case balance_change: {
			auto delim = data.find(':');
			if (delim == std::string::npos) {
				doWrite(std::to_underlying(balance_change), "FAIL");
				return;
			}
			auto user = data.substr(0, delim);
			auto change = std::stoll(data.substr(delim + 1));
			_db.changeBalance(user, change, [this, self, user] {
				ServerLog::balance( _peer + " user '" + user + "' balance changed");
				doWrite(std::to_underlying(PacketType::balance_change), "OK");
			});
			break;
		}
		case balance_transfer: {
			auto delim1 = data.find(':');
			if (delim1 == std::string::npos) {
				doWrite(std::to_underlying(balance_transfer), "FAIL");
				return;
			}
			auto delim2 = data.find(':', delim1 + 1);
			if (delim2 == std::string::npos) {
				doWrite(std::to_underlying(balance_transfer), "FAIL");
				return;
			}
			auto sender = data.substr(0, delim1);
			auto recipient = data.substr(delim1 + 1, delim2 - delim1 - 1);
			auto amount = std::stoull(data.substr(delim2 + 1));
			_db.transferBalance(sender, recipient, amount, [this, self, sender] {
				ServerLog::transfer( _peer + " transfer from '" + sender + "' OK");
				doWrite(std::to_underlying(PacketType::balance_transfer), "OK");
			});
			break;
		}
		default:
			ServerLog::warn(
					  _peer + " unknown packet type " + std::to_string(std::to_underlying(type)));
			break;
		}
	});
}

void Session::doWrite(u8 type, const std::string &response) {
	auto self = shared_from_this();
	packet::asyncSend(
		_socket, static_cast<PacketType>(type), response, [this, self](std::error_code ec) {
			if (ec) {
				ServerLog::info(_peer + " disconnected");
				return;
			}
			doRead();
		});
}

Server::Server(u16 port)
	: _acceptor(_io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), _db(_io) {
	ServerLog::info("listening on port " + std::to_string(port));
	doAccept();
}

void Server::run() {
	_io.run();
	ServerLog::info("shutting down");
}

void Server::doAccept() {
	_acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
		if (!ec) {
			std::make_shared<Session>(std::move(socket), _db)->start();
		}
		doAccept();
	});
}
