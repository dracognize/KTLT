#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include <algorithm>
#include <chrono>
#include <span>

#include "types.hpp"

namespace base {

	
	

	namespace detail {

		[[nodiscard]] constexpr auto rotr(u32 x, u32 n) noexcept -> u32 {
			return (x >> n) | (x << (32 - n));
		}

		[[nodiscard]] constexpr auto ch(u32 x, u32 y, u32 z) noexcept -> u32 {
			return (x & y) ^ (~x & z);
		}

		[[nodiscard]] constexpr auto maj(u32 x, u32 y, u32 z) noexcept -> u32 {
			return (x & y) ^ (x & z) ^ (y & z);
		}

		[[nodiscard]] constexpr auto sigma0(u32 x) noexcept -> u32 {
			return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
		}

		[[nodiscard]] constexpr auto sigma1(u32 x) noexcept -> u32 {
			return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
		}

		[[nodiscard]] constexpr auto gamma0(u32 x) noexcept -> u32 {
			return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
		}

		[[nodiscard]] constexpr auto gamma1(u32 x) noexcept -> u32 {
			return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
		}

		constexpr std::array<u32, 64> K = {
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
			0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
			0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
			0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
			0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
			0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
			0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
			0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
			0xc67178f2,
		};

	} 

	struct Sha256 {
			std::array<u32, 8> state = {
				0x6a09e667,
				0xbb67ae85,
				0x3c6ef372,
				0xa54ff53a,
				0x510e527f,
				0x9b05688c,
				0x1f83d9ab,
				0x5be0cd19,
			};
			u64 count = 0;
			std::array<u8, 64> buf{};
			usize bufLen = 0;

			void ingest(std::span<const u8> data) noexcept {
				count += data.size();
				while (!data.empty()) {
					auto space = 64 - bufLen;
					auto take = (std::min)(data.size(), space);
					std::memcpy(buf.data() + bufLen, data.data(), take);
					bufLen += take;
					data = data.subspan(take);
					if (bufLen == 64) {
						processBlock(buf);
						bufLen = 0;
					}
				}
			}

			void ingest(std::string_view s) noexcept {
				ingest(std::span<const u8>(reinterpret_cast<const u8 *>(s.data()), s.size()));
			}

			[[nodiscard]] auto finalize() noexcept -> std::array<u8, 32> {
				
				buf[bufLen++] = 0x80;

				
				if (bufLen > 56) {
					std::memset(buf.data() + bufLen, 0, 64 - bufLen);
					processBlock(buf);
					bufLen = 0;
				}
				std::memset(buf.data() + bufLen, 0, 56 - bufLen);

				
				auto bits = count * 8;
				if constexpr (std::endian::native == std::endian::little) {
					bits = std::byteswap(bits);
				}
				std::memcpy(buf.data() + 56, &bits, 8);

				processBlock(buf);

				
				std::array<u8, 32> hash{};
				for (usize i = 0; i < 8; ++i) {
					auto val = state[i];
					if constexpr (std::endian::native == std::endian::little) {
						val = std::byteswap(val);
					}
					std::memcpy(hash.data() + i * 4, &val, 4);
				}
				return hash;
			}

			[[nodiscard]] static auto hash(std::string_view data) noexcept -> std::array<u8, 32> {
				Sha256 ctx;
				ctx.ingest(data);
				return ctx.finalize();
			}

			[[nodiscard]] static auto hashHex(std::string_view data) noexcept -> std::string {
				auto h = hash(data);
				std::string result(64, '\0');
				constexpr const char hex[] = "0123456789abcdef";
				for (usize i = 0; i < 32; ++i) {
					result[i * 2] = hex[(h[i] >> 4) & 0xf];
					result[i * 2 + 1] = hex[h[i] & 0xf];
				}
				return result;
			}

		private:
			void processBlock(std::span<const u8, 64> block) noexcept {
				std::array<u32, 64> w{};
				for (int i = 0; i < 16; ++i) {
					u32 word{};
					std::memcpy(&word, block.data() + i * 4, 4);
					if constexpr (std::endian::native == std::endian::little) {
						word = std::byteswap(word);
					}
					w[i] = word;
				}
				for (int i = 16; i < 64; ++i) {
					w[i] = detail::gamma1(w[i - 2]) + w[i - 7] + detail::gamma0(w[i - 15])
						   + w[i - 16];
				}

				auto a = state[0];
				auto b = state[1];
				auto c = state[2];
				auto d = state[3];
				auto e = state[4];
				auto f = state[5];
				auto g = state[6];
				auto h = state[7];

				for (int i = 0; i < 64; ++i) {
					auto t1 = detail::sigma1(e) + detail::ch(e, f, g) + h + detail::K[i] + w[i];
					auto t2 = detail::sigma0(a) + detail::maj(a, b, c);
					h = g;
					g = f;
					f = e;
					e = d + t1;
					d = c;
					c = b;
					b = a;
					a = t1 + t2;
				}

				state[0] += a;
				state[1] += b;
				state[2] += c;
				state[3] += d;
				state[4] += e;
				state[5] += f;
				state[6] += g;
				state[7] += h;
			}
	};

	
	
	[[nodiscard]] inline auto hashPassword(std::string_view password,
										   std::string_view salt) noexcept -> std::string {
		return Sha256::hashHex(std::string{salt} + ":" + std::string{password});
	}

	
	[[nodiscard]] inline auto generateSalt() -> std::string {
		
		
		static constexpr const char chars[]
			= "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		std::string salt(16, '\0');
		
		auto now = std::chrono::system_clock::now().time_since_epoch().count();
		thread_local u64 counter = 0;
		++counter;
		auto seed = static_cast<u64>(now) ^ reinterpret_cast<u64>(&salt) ^ (counter << 32);
		for (auto &c : salt) {
			seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
			c = chars[(seed >> 32) % 62];
		}
		return salt;
	}

} 
