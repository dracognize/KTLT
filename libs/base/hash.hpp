#pragma once

#include <cstring>
#include <memory>
#include <type_traits>

#include "array.hpp"
#include "forward_list.hpp"
#include "list.hpp"
#include "string.hpp"
#include "types.hpp"
#include "vector.hpp"

namespace base {
	namespace murmur {
		inline constexpr auto rotl64(u64 x, i32 r) -> u64 {
			return (x << r) | (x >> (64 - r));
		}

		inline constexpr auto fmix64(u64 k) -> u64 {
			k ^= k >> 33;
			k *= 0xff51afd7ed558ccdULL;
			k ^= k >> 33;
			k *= 0xc4ceb9fe1a85ec53ULL;
			k ^= k >> 33;
			return k;
		}

		inline constexpr auto hash64(const void* key, i32 len, u64 seed = 0) -> u64 {
			const u8* data	  = static_cast<const u8*>(key);
			const i32 nblocks = len / 16;

			u64		  h1 = seed, h2 = seed;
			const u64 c1 = 0x87c37b91114253d5ULL;
			const u64 c2 = 0x4cf5ad432745937fULL;

			const u64* blocks = reinterpret_cast<const u64*>(data);
			for (i32 i = 0; i < nblocks; i++) {
				u64 k1, k2;
				std::memcpy(&k1, blocks + i * 2, sizeof(k1));
				std::memcpy(&k2, blocks + i * 2 + 1, sizeof(k2));

				k1 *= c1;
				k1 = rotl64(k1, 31);
				k1 *= c2;
				h1 ^= k1;
				h1 = rotl64(h1, 27);
				h1 += h2;
				h1 = h1 * 5 + 0x52dce729;

				k2 *= c2;
				k2 = rotl64(k2, 33);
				k2 *= c1;
				h2 ^= k2;
				h2 = rotl64(h2, 31);
				h2 += h1;
				h2 = h2 * 5 + 0x38495ab5;
			}

			const u8* tail = data + nblocks * 16;
			u64		  k1 = 0, k2 = 0;
			switch (len & 15) {
				case 15:
					k2 ^= static_cast<u64>(tail[14]) << 48;
					[[fallthrough]];
				case 14:
					k2 ^= static_cast<u64>(tail[13]) << 40;
					[[fallthrough]];
				case 13:
					k2 ^= static_cast<u64>(tail[12]) << 32;
					[[fallthrough]];
				case 12:
					k2 ^= static_cast<u64>(tail[11]) << 24;
					[[fallthrough]];
				case 11:
					k2 ^= static_cast<u64>(tail[10]) << 16;
					[[fallthrough]];
				case 10:
					k2 ^= static_cast<u64>(tail[9]) << 8;
					[[fallthrough]];
				case 9:
					k2 ^= static_cast<u64>(tail[8]);
					k2 *= c2;
					k2 = rotl64(k2, 33);
					k2 *= c1;
					h2 ^= k2;
					[[fallthrough]];
				case 8:
					k1 ^= static_cast<u64>(tail[7]) << 56;
					[[fallthrough]];
				case 7:
					k1 ^= static_cast<u64>(tail[6]) << 48;
					[[fallthrough]];
				case 6:
					k1 ^= static_cast<u64>(tail[5]) << 40;
					[[fallthrough]];
				case 5:
					k1 ^= static_cast<u64>(tail[4]) << 32;
					[[fallthrough]];
				case 4:
					k1 ^= static_cast<u64>(tail[3]) << 24;
					[[fallthrough]];
				case 3:
					k1 ^= static_cast<u64>(tail[2]) << 16;
					[[fallthrough]];
				case 2:
					k1 ^= static_cast<u64>(tail[1]) << 8;
					[[fallthrough]];
				case 1:
					k1 ^= static_cast<u64>(tail[0]);
					k1 *= c1;
					k1 = rotl64(k1, 31);
					k1 *= c2;
					h1 ^= k1;
			}

			h1 ^= static_cast<u64>(len);
			h2 ^= static_cast<u64>(len);
			h1 += h2;
			h2 += h1;
			h1 = fmix64(h1);
			h2 = fmix64(h2);
			h1 += h2;
			return h1;
		}
	} // namespace murmur

	// ── Primary hash template (trivially copyable types) ──────────────

	template <class t_Type>
	struct hash {
			[[nodiscard]] constexpr usize operator()(const t_Type& key) const noexcept {
				static_assert(std::is_trivially_copyable_v<t_Type>,
							  "base::hash<T> requires T to be trivially copyable. "
							  "Provide a specialization of base::hash for custom types.");
				return murmur::hash64(std::addressof(key), static_cast<i32>(sizeof(key)));
			}
	};

	// ── Hash specializations for base container types ────────────────

	template <class t_Char, class t_Traits, class t_Alloc>
	struct hash<BasicString<t_Char, t_Traits, t_Alloc>> {
			[[nodiscard]] usize operator()(const BasicString<t_Char, t_Traits, t_Alloc>& key) const noexcept {
				return murmur::hash64(key.data(), static_cast<i32>(key.size() * static_cast<usize>(sizeof(t_Char))));
			}
	};

	template <class t_Type, usize t_Size>
	struct hash<Array<t_Type, t_Size>> {
			[[nodiscard]] usize operator()(const Array<t_Type, t_Size>& key) const noexcept {
				if constexpr (std::is_trivially_copyable_v<t_Type>) {
					return murmur::hash64(&key, static_cast<i32>(sizeof(key)));
				}
				else {
					usize h = 0;
					for (const auto& elem : key) {
						h ^= hash<t_Type>{}(elem) + 0x9e3779b9 + (h << 6) + (h >> 2);
					}
					return h;
				}
			}
	};

	template <class t_Type, class t_Allocator>
	struct hash<Vector<t_Type, t_Allocator>> {
			[[nodiscard]] usize operator()(const Vector<t_Type, t_Allocator>& key) const noexcept {
				if constexpr (std::is_trivially_copyable_v<t_Type>) {
					return murmur::hash64(key.data(),
										  static_cast<i32>(key.size() * static_cast<usize>(sizeof(t_Type))));
				}
				else {
					usize h = 0;
					for (const auto& elem : key) {
						h ^= hash<t_Type>{}(elem) + 0x9e3779b9 + (h << 6) + (h >> 2);
					}
					return h;
				}
			}
	};

	template <class t_Type, class t_Allocator>
	struct hash<ForwardList<t_Type, t_Allocator>> {
			[[nodiscard]] usize operator()(const ForwardList<t_Type, t_Allocator>& key) const noexcept {
				usize h = 0;
				for (const auto& elem : key) {
					h ^= hash<t_Type>{}(elem) + 0x9e3779b9 + (h << 6) + (h >> 2);
				}
				return h;
			}
	};

	template <class t_Type, class t_Allocator>
	struct hash<List<t_Type, t_Allocator>> {
			[[nodiscard]] usize operator()(const List<t_Type, t_Allocator>& key) const noexcept {
				usize h = 0;
				for (const auto& elem : key) {
					h ^= hash<t_Type>{}(elem) + 0x9e3779b9 + (h << 6) + (h >> 2);
				}
				return h;
			}
	};
} // namespace base
