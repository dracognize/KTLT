#include "app/client/Client.hpp"
#include "libs/network/Packet.hpp"

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
					if (handler)
						handler(_connected);
				});
		});
}

void Client::run() {
	_io.run();
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
					auto val = std::stoull(resp);
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

void Client::sendRequest(PacketType type,
						 const std::string &payload,
						 std::function<void(bool, std::string)> handler) {
	asio::post(_io, [this, type, payload, handler = std::move(handler)]() mutable {
		if (!_connected) {
			if (handler)
				handler(false, {});
			return;
		}

		packet::asyncSend(
			_socket, type, payload,
			[this, handler = std::move(handler)](std::error_code ec) mutable {
				if (ec) {
					_connected = false;
					if (handler)
						handler(false, {});
					return;
				}
				packet::asyncRecv(
					_socket,
					[handler = std::move(handler)](
						std::error_code ec2, PacketType, std::string data) mutable {
						if (ec2) {
							if (handler)
								handler(false, {});
							return;
						}
						if (handler)
							handler(true, data);
					});
			});
	});
}
