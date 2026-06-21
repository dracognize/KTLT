#include "app/client/Client.hpp"
#include "libs/network/Packet.hpp"

#include <charconv>

Client::Client(const std::string &host, u16 port)
	: _work(asio::make_work_guard(_io)), _socket(_io), _host(host), _port(port) {
}

Client::~Client() {
	_io.stop();
	if (_socket.is_open()) {
		std::error_code ec;
		std::ignore = _socket.close(ec);
	}
}

void Client::stop() {
	_io.stop();
}

void Client::onDisconnect(Client::VoidHandler handler) {
	_disconnectHandler = std::move(handler);
}

void Client::connect(BoolHandler handler) {
	auto resolver = std::make_shared<asio::ip::tcp::resolver>(_io);
	resolver->async_resolve(
		_host,
		std::to_string(_port),
		[this, resolver, handler = std::move(handler)](std::error_code ec, auto results) mutable {
			if (ec) {
				if (handler)
					handler(false);
				return;
			}
			asio::async_connect(
				_socket,
				results,
				[this, handler = std::move(handler)](std::error_code ec2, auto) mutable {
					_connected = !ec2;
					if (_connected)
						recvLoop();
					if (handler)
						handler(_connected);
				});
		});
}

void Client::run() {
	_io.run();
}

void Client::sendRequest(PacketType type, const std::string &payload, Handler handler) {
	asio::post(_io, [this, type, payload, handler = std::move(handler)]() mutable {
		if (!_connected) {
			if (handler)
				handler(false, {});
			return;
		}
		_pending.push(std::move(handler));
		packet::asyncSend(_socket, type, payload, [this](std::error_code ec) {
			if (ec) {
				_connected = false;
				if (auto h = std::move(_disconnectHandler))
					h();
			}
		});
	});
}

void Client::recvLoop() {
	packet::asyncRecv(_socket, [this](std::error_code ec, PacketType, std::string data) {
		if (ec) {
			_connected = false;
			if (auto h = std::move(_disconnectHandler))
				h();
			while (!_pending.empty()) {
				auto h = std::move(_pending.front());
				_pending.pop();
				if (h)
					h(false, {});
			}
			return;
		}
		if (_pending.empty()) {
			
			recvLoop();
			return;
		}
		auto handler = std::move(_pending.front());
		_pending.pop();
		if (handler)
			handler(true, std::move(data));
		recvLoop();
	});
}

void Client::authenticate(const std::string &username,
						  const std::string &password,
						  BoolHandler handler) {
	auto payload = username + ":" + password;
	sendRequest(PacketType::account_auth,
				payload,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok && resp == "OK");
				});
}

void Client::createAccount(const std::string &username,
						   const std::string &password,
						   BoolHandler handler) {
	auto payload = username + ":" + password;
	sendRequest(PacketType::account_create,
				payload,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok && resp == "OK");
				});
}

void Client::getBalance(const std::string &username, U64Handler handler) {
	sendRequest(PacketType::balance_req,
				username,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (!ok || resp.empty()) {
						if (handler)
							handler(std::nullopt);
						return;
					}
					u64 val{};
					auto [ptr, ec2] = std::from_chars(resp.data(), resp.data() + resp.size(), val);
					if (ec2 != std::errc{}) {
						if (handler)
							handler(std::nullopt);
						return;
					}
					if (handler)
						handler(val);
				});
}

void Client::changeBalance(const std::string &username, i64 change, BoolHandler handler) {
	auto payload = username + ":" + std::to_string(change);
	sendRequest(PacketType::balance_change,
				payload,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok && resp == "OK");
				});
}

void Client::transferBalance(const std::string &sender,
							 const std::string &recipient,
							 u64 amount,
							 BoolHandler handler) {
	auto payload = sender + ":" + recipient + ":" + std::to_string(amount);
	sendRequest(PacketType::balance_transfer,
				payload,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok && resp == "OK");
				});
}

void Client::changePassword(const std::string &username,
							const std::string &newPassword,
							BoolHandler handler) {
	auto payload = username + ":" + newPassword;
	sendRequest(PacketType::account_change_password,
				payload,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok && resp == "OK");
				});
}

void Client::toggleAccount(const std::string &username, BoolHandler handler) {
	sendRequest(PacketType::account_toggle,
				username,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok && resp == "OK");
				});
}

void Client::ping(BoolHandler handler) {
	sendRequest(PacketType::ping,
				"",
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok && resp == "PONG");
				});
}

void Client::getLogs(const std::string &username, StringHandler handler) {
	sendRequest(PacketType::log_req,
				username,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok ? std::optional(resp) : std::nullopt);
				});
}

void Client::getTransactionHistory(const std::string &username, u64 count, StringHandler handler) {
	auto payload = username + ":" + std::to_string(count);
	sendRequest(PacketType::txn_history_req,
				payload,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok ? std::optional(resp) : std::nullopt);
				});
}

void Client::searchUsers(const std::string &prefix, StringHandler handler) {
	sendRequest(PacketType::user_search_req,
				prefix,
				[handler = std::move(handler)](bool ok, const std::string &resp) mutable {
					if (handler)
						handler(ok ? std::optional(resp) : std::nullopt);
				});
}
