#pragma once

#include <array>
#include <asio.hpp>
#include <bit>
#include <functional>
#include <libs/base/types.hpp>
#include <memory>
#include <string>
#include <vector>

#include "PacketType.hpp"

namespace packet {

	inline constexpr usize HeaderSize = 5;
	inline constexpr usize LengthSize = 4;
	inline constexpr usize TypeSize = 1;

	[[nodiscard]] inline auto toNetwork(u32 val) noexcept -> u32 {
		if constexpr (std::endian::native == std::endian::little) {
			return std::byteswap(val);
		}
		return val;
	}

	auto send(asio::ip::tcp::socket &socket, PacketType type, const std::string &data) -> void;
	auto recv(asio::ip::tcp::socket &socket) -> std::pair<PacketType, std::string>;

	auto asyncSend(asio::ip::tcp::socket &socket,
				   PacketType type,
				   const std::string &data,
				   std::function<void(std::error_code)> handler) -> void;

	
	
	auto asyncRecv(asio::ip::tcp::socket &socket,
				   std::function<void(std::error_code, PacketType, std::string)> handler) -> void;

} 
