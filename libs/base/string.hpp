#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <ostream>
#include <utility>

#include "types.hpp"

namespace base {
	template <class t_Char,
			  class t_Traits = std::char_traits<t_Char>,
			  class t_Allocator = std::allocator<t_Char>>
	class BasicString {
		public:
			using value_type = t_Char;
			using traits_type = t_Traits;
			using allocator_type = t_Allocator;
			using size_type = usize;
			using difference_type = isize;
			using reference = value_type &;
			using const_reference = const value_type &;
			using pointer = std::allocator_traits<t_Allocator>::pointer;
			using const_pointer = std::allocator_traits<t_Allocator>::const_pointer;
			using iterator = value_type *;
			using const_iterator = const value_type *;
			using reverse_iterator = std::reverse_iterator<iterator>;
			using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		private:
			static constexpr size_type SSO_Capacity
				= (sizeof(pointer) + sizeof(size_type)) / sizeof(value_type) - 1;
			static constexpr size_type GrowthFactor = 2;

			struct Long {
					pointer _data;
					size_type _size;
					size_type _cap;
			};

			struct Short {
					value_type _buffer[SSO_Capacity + 1];
					size_type _size;
			};

			union Storage {
					Long _long;
					Short _short;

					constexpr Storage() : _short{} {
					}
			};

			Storage _storage;
			t_Allocator _alloc;
			bool _isLong = false;

		public:
			constexpr BasicString() noexcept(noexcept(t_Allocator())) : _alloc() {
				_storage._short._size = 0;
				_storage._short._buffer[0] = value_type(0);
			}

			constexpr explicit BasicString(const t_Allocator &alloc) noexcept : _alloc(alloc) {
				_storage._short._size = 0;
				_storage._short._buffer[0] = value_type(0);
			}

			constexpr BasicString(const value_type *s,
								  size_type n,
								  const t_Allocator &alloc = t_Allocator())
				: _alloc(alloc) {
				if (n <= SSO_Capacity) {
					_isLong = false;
					t_Traits::copy(_storage._short._buffer, s, n);
					_storage._short._buffer[n] = value_type(0);
					_storage._short._size = n;
				} else {
					_isLong = true;
					_storage._long._data
						= std::allocator_traits<t_Allocator>::allocate(_alloc, n + 1);
					t_Traits::copy(_storage._long._data, s, n);
					_storage._long._data[n] = value_type(0);
					_storage._long._size = n;
					_storage._long._cap = n;
				}
			}

			constexpr BasicString(const value_type *s, const t_Allocator &alloc = t_Allocator())
				: BasicString(s, t_Traits::length(s), alloc) {
			}

			constexpr ~BasicString() {
				if (_isLong) {
					std::allocator_traits<t_Allocator>::deallocate(
						_alloc, _storage._long._data, _storage._long._cap + 1);
				}
			}

			[[nodiscard]] constexpr auto size() const noexcept -> size_type {
				return _isLong ? _storage._long._size : _storage._short._size;
			}

			[[nodiscard]] constexpr auto length() const noexcept -> size_type {
				return size();
			}

			[[nodiscard]] constexpr auto capacity() const noexcept -> size_type {
				return _isLong ? _storage._long._cap : SSO_Capacity;
			}

			[[nodiscard]] constexpr auto empty() const noexcept -> bool {
				return size() == 0;
			}

			constexpr auto data(this auto &&self) noexcept -> decltype(auto) {
				return self._isLong ? self._storage._long._data : self._storage._short._buffer;
			}

			constexpr auto c_str() const noexcept -> const value_type * {
				return data();
			}

			constexpr auto operator[](this auto &&self, size_type pos) noexcept -> decltype(auto) {
				return self.data()[pos];
			}

			constexpr auto at(this auto &&self, size_type pos) -> decltype(auto) {
				if (pos >= self.size())
					throw std::out_of_range("[String] index out of range");
				return self.data()[pos];
			}

			constexpr auto front(this auto &&self) noexcept -> decltype(auto) {
				return self.data()[0];
			}

			constexpr auto back(this auto &&self) noexcept -> decltype(auto) {
				return self.data()[self.size() - 1];
			}

			constexpr auto begin(this auto &&self) noexcept {
				return self.data();
			}

			constexpr auto end(this auto &&self) noexcept {
				return self.data() + self.size();
			}

			constexpr auto cbegin() const noexcept -> const_iterator {
				return data();
			}

			constexpr auto cend() const noexcept -> const_iterator {
				return data() + size();
			}

			constexpr auto rbegin(this auto &&self) noexcept {
				return std::reverse_iterator{self.end()};
			}

			constexpr auto rend(this auto &&self) noexcept {
				return std::reverse_iterator{self.begin()};
			}

			constexpr auto crbegin() const noexcept -> const_reverse_iterator {
				return const_reverse_iterator{cend()};
			}

			constexpr auto crend() const noexcept -> const_reverse_iterator {
				return const_reverse_iterator{cbegin()};
			}

			constexpr auto push_back(value_type c) -> void {
				const auto curr_size = size();
				if (curr_size + 1 > capacity())
					reserve((curr_size + 1) * GrowthFactor);

				auto ptr = data();
				ptr[curr_size] = c;
				ptr[curr_size + 1] = value_type(0);

				if (_isLong)
					++_storage._long._size;
				else
					++_storage._short._size;
			}

			constexpr auto pop_back() -> void {
				const auto curr_size = size();
				auto ptr = data();
				ptr[curr_size - 1] = value_type(0);
				if (_isLong)
					--_storage._long._size;
				else
					--_storage._short._size;
			}

			constexpr auto reserve(size_type new_cap) -> void {
				if (new_cap <= capacity())
					return;

				const auto curr_size = size();
				const auto ptr = data();
				auto new_ptr = std::allocator_traits<t_Allocator>::allocate(_alloc, new_cap + 1);

				t_Traits::copy(new_ptr, ptr, curr_size);
				new_ptr[curr_size] = value_type(0);

				if (_isLong) {
					std::allocator_traits<t_Allocator>::deallocate(
						_alloc, _storage._long._data, _storage._long._cap + 1);
				}

				_storage._long._data = new_ptr;
				_storage._long._size = curr_size;
				_storage._long._cap = new_cap;
				_isLong = true;
			}

			constexpr auto clear() noexcept -> void {
				if (_isLong) {
					std::allocator_traits<t_Allocator>::deallocate(
						_alloc, _storage._long._data, _storage._long._cap + 1);
					_isLong = false;
				}
				_storage._short._size = 0;
				_storage._short._buffer[0] = value_type(0);
			}

			constexpr BasicString(const BasicString &other)
				: _alloc(std::allocator_traits<t_Allocator>::select_on_container_copy_construction(
					  other._alloc)) {
				const auto n = other.size();
				if (n <= SSO_Capacity) {
					_isLong = false;
					t_Traits::copy(_storage._short._buffer, other.data(), n);
					_storage._short._buffer[n] = value_type(0);
					_storage._short._size = n;
				} else {
					_isLong = true;
					_storage._long._data
						= std::allocator_traits<t_Allocator>::allocate(_alloc, n + 1);
					t_Traits::copy(_storage._long._data, other.data(), n);
					_storage._long._data[n] = value_type(0);
					_storage._long._size = n;
					_storage._long._cap = n;
				}
			}

			constexpr BasicString(BasicString &&other) noexcept
				: _storage(other._storage), _alloc(std::move(other._alloc)),
				  _isLong(other._isLong) {
				other._isLong = false;
				other._storage._short._size = 0;
				other._storage._short._buffer[0] = value_type(0);
			}

			constexpr auto operator=(const BasicString &other) -> BasicString & {
				if (this != &other) {
					clear();
					const auto n = other.size();
					if (n <= capacity()) {
						t_Traits::copy(data(), other.data(), n);
						data()[n] = value_type(0);
						if (_isLong)
							_storage._long._size = n;
						else
							_storage._short._size = n;
					} else {
						if (_isLong)
							std::allocator_traits<t_Allocator>::deallocate(
								_alloc, _storage._long._data, _storage._long._cap + 1);
						_storage._long._data
							= std::allocator_traits<t_Allocator>::allocate(_alloc, n + 1);
						t_Traits::copy(_storage._long._data, other.data(), n);
						_storage._long._data[n] = value_type(0);
						_storage._long._size = n;
						_storage._long._cap = n;
						_isLong = true;
					}
				}
				return *this;
			}

			constexpr auto operator=(BasicString &&other) noexcept -> BasicString & {
				if (this != &other) {
					if (_isLong)
						std::allocator_traits<t_Allocator>::deallocate(
							_alloc, _storage._long._data, _storage._long._cap + 1);
					_storage = other._storage;
					_alloc = std::move(other._alloc);
					_isLong = other._isLong;
					other._isLong = false;
					other._storage._short._size = 0;
					other._storage._short._buffer[0] = value_type(0);
				}
				return *this;
			}

			[[nodiscard]] constexpr auto operator==(const BasicString &other) const noexcept
				-> bool {
				return size() == other.size()
					   && t_Traits::compare(data(), other.data(), size()) == 0;
			}
	};

	template <class t_Char, class t_Traits, class t_Alloc>
	auto operator<<(std::basic_ostream<t_Char, std::char_traits<t_Char>> &os,
					const BasicString<t_Char, t_Traits, t_Alloc> &s)
		-> std::basic_ostream<t_Char, std::char_traits<t_Char>> & {
		os.write(s.data(), static_cast<std::streamsize>(s.size()));
		return os;
	}

	using String = BasicString<char>;
	using U8String = BasicString<char8_t>;
} // namespace base

template <class t_Traits, class t_Alloc>
struct std::formatter<base::BasicString<char, t_Traits, t_Alloc>> : formatter<std::string_view> {
		auto format(const base::BasicString<char, t_Traits, t_Alloc> &s,
					format_context &ctx) const {
			return formatter<std::string_view>::format({s.data(), s.size()}, ctx);
		}
};

template <class t_Traits, class t_Alloc>
struct std::formatter<base::BasicString<char8_t, t_Traits, t_Alloc>> : formatter<std::string_view> {
		auto format(const base::BasicString<char8_t, t_Traits, t_Alloc> &s,
					format_context &ctx) const {
			return formatter<std::string_view>::format(
				{reinterpret_cast<const char *>(s.data()), s.size()}, ctx);
		}
};
