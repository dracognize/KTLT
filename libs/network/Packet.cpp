#include "Packet.hpp"

namespace packet {

	auto send(asio::ip::tcp::socket &socket, PacketType type, const std::string &data) -> void {
		auto len = toNetwork(static_cast<u32>(data.size()));

		std::array<char, HeaderSize> header{};
		std::memcpy(header.data(), &len, LengthSize);
		header[LengthSize] = static_cast<char>(type);

		std::vector<asio::const_buffer> buffers = {
			asio::buffer(header.data(), HeaderSize),
			asio::buffer(data),
		};
		asio::write(socket, buffers);
	}

	auto recv(asio::ip::tcp::socket &socket) -> std::pair<PacketType, std::string> {
		std::array<char, HeaderSize> header{};
		asio::read(socket, asio::buffer(header));

		u32 len{};
		std::memcpy(&len, header.data(), LengthSize);
		len = toNetwork(len);

		auto type = static_cast<PacketType>(header[LengthSize]);

		std::string data(len, '\0');
		asio::read(socket, asio::buffer(data));

		return {type, data};
	}

	auto asyncSend(asio::ip::tcp::socket &socket,
				   PacketType type,
				   const std::string &data,
				   std::function<void(std::error_code)> handler) -> void {
		auto header = std::make_shared<std::array<char, HeaderSize>>();

		auto len = toNetwork(static_cast<u32>(data.size()));
		std::memcpy(header->data(), &len, LengthSize);
		(*header)[LengthSize] = static_cast<char>(type);

		auto body = std::make_shared<std::string>(data);

		std::vector<asio::const_buffer> buffers = {
			asio::buffer(header->data(), HeaderSize),
			asio::buffer(*body),
		};

		asio::async_write(
			socket, buffers, [handler, header, body](std::error_code ec, usize /*bytes*/) {
				if (handler) {
					handler(ec);
				}
			});
	}

	auto asyncRecv(asio::ip::tcp::socket &socket,
				   std::function<void(std::error_code, PacketType, std::string)> handler) -> void {
		auto header = std::make_shared<std::array<char, HeaderSize>>();

		asio::async_read(
			socket,
			asio::buffer(*header),
			[&socket, header, handler](std::error_code ec, usize /*bytes*/) {
				if (ec) {
					if (handler) {
						handler(ec, PacketType{}, {});
					}
					return;
				}

				u32 len{};
				std::memcpy(&len, header->data(), LengthSize);
				len = toNetwork(len);

				auto type = static_cast<PacketType>((*header)[LengthSize]);

				auto body = std::make_shared<std::string>(len, '\0');
				asio::async_read(
					socket,
					asio::buffer(body->data(), len),
					[body, type, handler](std::error_code ec2, usize /*bytes*/) {
						if (handler) {
							handler(ec2, ec2 ? PacketType{} : type, ec2 ? std::string{} : *body);
						}
					});
			});
	}

} // namespace packet
