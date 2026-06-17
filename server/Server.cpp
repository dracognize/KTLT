#include "server/Database.hpp"
#include "server/ServerLog.hpp"

#include "libs/network/Packet.hpp"
#include "libs/network/PacketType.hpp"
#include "server/Server.hpp"

#include <fstream>
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
				ServerLog::warn(_peer + " malformed auth packet");
				doWrite(std::to_underlying(account_auth), "FAIL");
				return;
			}
			auto user = data.substr(0, delim);
			auto pass = data.substr(delim + 1);
			_db.authenticate(user, pass, [this, self, user](bool ok) {
				ServerLog::auth(_peer + " user '" + user + "' " + (ok ? "OK" : "FAIL"));
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
<<<<<<< HEAD
			_db.createAccount(user, pass, [this, self, user] {
				ServerLog::create( _peer + " user '" + user + "' created");
				doWrite(std::to_underlying(PacketType::account_create), "OK");
=======
			_db.createAccount(user, pass, [this, self, user](bool created) {
				if (created) {
					ServerLog::create(_peer + " user '" + user + "' created");
				} else {
					ServerLog::warn(_peer + " user '" + user + "' already exists");
				}
				doWrite(std::to_underlying(PacketType::account_create), created ? "OK" : "FAIL");
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
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
<<<<<<< HEAD
			auto change = std::stoll(data.substr(delim + 1));
			_db.changeBalance(user, change, [this, self, user] {
				ServerLog::balance( _peer + " user '" + user + "' balance changed");
				doWrite(std::to_underlying(PacketType::balance_change), "OK");
=======
			i64 change = 0;
			try {
				change = std::stoll(data.substr(delim + 1));
			} catch (...) {
				doWrite(std::to_underlying(balance_change), "FAIL");
				return;
			}
			_db.changeBalance(user, change, [this, self, user](bool ok) {
				if (ok) {
					ServerLog::balance(_peer + " user '" + user + "' balance changed");
				}
				doWrite(std::to_underlying(PacketType::balance_change), ok ? "OK" : "FAIL");
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
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
<<<<<<< HEAD
			auto amount = std::stoull(data.substr(delim2 + 1));
			_db.transferBalance(sender, recipient, amount, [this, self, sender] {
				ServerLog::transfer( _peer + " transfer from '" + sender + "' OK");
				doWrite(std::to_underlying(PacketType::balance_transfer), "OK");
			});
			break;
		}
		case account_change_password: {
			auto delim = data.find(':');
			if (delim == std::string::npos) {
				doWrite(std::to_underlying(account_change_password), "FAIL");
				return;
			}
			auto user = data.substr(0, delim);
			auto newPass = data.substr(delim + 1);
			_db.changePassword(user, newPass, [this, self, user] {
				ServerLog::password( _peer + " user '" + user + "' password changed");
				doWrite(std::to_underlying(PacketType::account_change_password), "OK");
			});
			break;
		}
		case account_toggle: {
			_db.toggleAccount(data, [this, self, user = data] {
				ServerLog::toggle( _peer + " user '" + user + "' lock toggled");
				doWrite(std::to_underlying(PacketType::account_toggle), "OK");
=======
			u64 amount = 0;
			try {
				amount = std::stoull(data.substr(delim2 + 1));
			} catch (...) {
				doWrite(std::to_underlying(balance_transfer), "FAIL");
				return;
			}
			_db.transferBalance(sender, recipient, amount, [this, self, sender](bool ok) {
				if (ok) {
					ServerLog::transfer(_peer + " transfer from '" + sender + "' OK");
				}
				doWrite(std::to_underlying(PacketType::balance_transfer), ok ? "OK" : "FAIL");
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
			});
			break;
		}
		case ping:
			doWrite(std::to_underlying(ping), "PONG");
			break;
		case log_req: {
			// Validate path component to prevent directory traversal
			if (data.empty() || data.find('/') != std::string::npos
				|| data.find("..") != std::string::npos) {
				doWrite(std::to_underlying(log_req), "");
				break;
			}
			auto path = std::string{DataDir} + "/" + data + ".log";
			auto file = std::ifstream(path);
			if (!file) {
				doWrite(std::to_underlying(log_req), "");
				break;
			}
			std::vector<std::string> lines;
			std::string line;
			while (std::getline(file, line)) {
				lines.push_back(std::move(line));
			}
			auto count = lines.size();
			auto start = count > 20 ? count - 20 : 0;
			std::string result;
			for (auto i = start; i < count; ++i) {
				result += lines[i] + '\n';
			}
			doWrite(std::to_underlying(log_req), result);
			break;
		}
		case account_change_password: {
			auto delim = data.find(':');
			if (delim == std::string::npos) {
				doWrite(std::to_underlying(account_change_password), "FAIL");
				return;
			}
			auto user = data.substr(0, delim);
			auto newPass = data.substr(delim + 1);
			_db.changePassword(user, newPass, [this, self, user](bool ok) {
				if (ok) {
					ServerLog::password(_peer + " user '" + user + "' password changed");
				} else {
					ServerLog::warn(_peer + " user '" + user + "' password change failed");
				}
				doWrite(std::to_underlying(PacketType::account_change_password),
						ok ? "OK" : "FAIL");
			});
			break;
		}
		case account_toggle: {
			_db.toggleAccount(data,
							  [this, self](bool) { doWrite(std::to_underlying(account_toggle), "OK"); });
			break;
		}
		default:
			ServerLog::warn(_peer + " unknown packet type "
							+ std::to_string(std::to_underlying(type)));
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

<<<<<<< HEAD
=======
void Server::stop() {
	_io.stop();
}

auto Server::ioContext() -> asio::io_context & {
	return _io;
}

>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
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
