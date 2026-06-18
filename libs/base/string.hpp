#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <ostream>
#include <string_view>
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

			constexpr BasicString(
				std::basic_string_view<value_type, traits_type> sv,
				const t_Allocator &alloc = t_Allocator())
				: BasicString(sv.data(), sv.size(), alloc) {
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

			[[nodiscard]] constexpr auto view() const noexcept
				-> std::basic_string_view<value_type, traits_type> {
				return std::basic_string_view<value_type, traits_type>(data(), size());
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

			/// Special value used to indicate "not found".
		static constexpr size_type npos = static_cast<size_type>(-1);

		/// Find first occurrence of @p str starting at @p pos.
		[[nodiscard]] constexpr auto find(const BasicString &str, size_type pos = 0) const noexcept
			-> size_type {
			if (pos > size())
				return npos;
			auto s = size();
			auto n = str.size();
			if (n == 0)
				return pos;
			if (n > s - pos)
				return npos;
			for (size_type i = pos; i <= s - n; ++i) {
				if (t_Traits::compare(data() + i, str.data(), n) == 0)
					return i;
			}
			return npos;
		}

		/// Find first occurrence of character @p c starting at @p pos.
		[[nodiscard]] constexpr auto find(value_type c, size_type pos = 0) const noexcept
			-> size_type {
			if (pos >= size())
				return npos;
			for (size_type i = pos; i < size(); ++i) {
				if (data()[i] == c)
					return i;
			}
			return npos;
		}

		/// Find first occurrence of C-string @p s starting at @p pos.
		[[nodiscard]] constexpr auto find(const value_type *s, size_type pos = 0) const noexcept
			-> size_type {
			return find(BasicString(s), pos);
		}

		/// Extract substring [pos, pos + count).  If count == npos, goes to the end.
		[[nodiscard]] constexpr auto substr(size_type pos = 0, size_type count = npos) const
			-> BasicString {
			if (pos > size())
				throw std::out_of_range("BasicString::substr");
			auto n = (std::min)(count, size() - pos);
			return BasicString(data() + pos, n, _alloc);
		}

		/// Append @p other to the end of this string.
		constexpr auto operator+=(const BasicString &other) -> BasicString & {
			auto curr_size = size();
			auto new_size = curr_size + other.size();
			if (new_size > capacity()) {
				auto new_cap = (std::max)(capacity() * GrowthFactor, new_size);
				reserve(new_cap);
			}
			auto ptr = data();
			t_Traits::copy(ptr + curr_size, other.data(), other.size());
			ptr[new_size] = value_type(0);
			if (_isLong)
				_storage._long._size = new_size;
			else
				_storage._short._size = new_size;
			return *this;
		}

		/// Append character @p c to the end of this string.
		constexpr auto operator+=(value_type c) -> BasicString & {
			push_back(c);
			return *this;
		}

		/// Append C-string @p s to the end of this string.
		constexpr auto operator+=(const value_type *s) -> BasicString & {
			return *this += BasicString(s);
		}

		/// Check if the string starts with @p prefix.
		[[nodiscard]] constexpr auto starts_with(const BasicString &prefix) const noexcept
			-> bool {
			if (prefix.size() > size())
				return false;
			return t_Traits::compare(data(), prefix.data(), prefix.size()) == 0;
		}

		/// Check if the string starts with character @p c.
		[[nodiscard]] constexpr auto starts_with(value_type c) const noexcept -> bool {
			return !empty() && front() == c;
		}

		/// Check if the string starts with C-string @p s.
		[[nodiscard]] constexpr auto starts_with(const value_type *s) const noexcept -> bool {
			return starts_with(BasicString(s));
		}

		/// Check if the string ends with @p suffix.
		[[nodiscard]] constexpr auto ends_with(const BasicString &suffix) const noexcept -> bool {
			if (suffix.size() > size())
				return false;
			return t_Traits::compare(data() + size() - suffix.size(), suffix.data(), suffix.size())
				   == 0;
		}

		/// Check if the string ends with character @p c.
		[[nodiscard]] constexpr auto ends_with(value_type c) const noexcept -> bool {
			return !empty() && back() == c;
		}

		/// Check if the string ends with C-string @p s.
		[[nodiscard]] constexpr auto ends_with(const value_type *s) const noexcept -> bool {
			return ends_with(BasicString(s));
		}

		[[nodiscard]] constexpr auto operator==(const BasicString &other) const noexcept
			-> bool {
			return size() == other.size()
				   && t_Traits::compare(data(), other.data(), size()) == 0;
		}

		[[nodiscard]] constexpr auto operator<(const BasicString &other) const noexcept
			-> bool {
			auto n = (std::min)(size(), other.size());
			auto cmp = t_Traits::compare(data(), other.data(), n);
			if (cmp != 0)
				return cmp < 0;
			return size() < other.size();
		}
	};

	/// Concatenate two BasicStrings.
	template <class t_Char, class t_Traits, class t_Alloc>
	[[nodiscard]] constexpr auto
	operator+(const BasicString<t_Char, t_Traits, t_Alloc> &a,
			  const BasicString<t_Char, t_Traits, t_Alloc> &b)
		-> BasicString<t_Char, t_Traits, t_Alloc> {
		auto result = a;
		result += b;
		return result;
	}

	/// Concatenate a BasicString with a C-string.
	template <class t_Char, class t_Traits, class t_Alloc>
	[[nodiscard]] constexpr auto operator+(const BasicString<t_Char, t_Traits, t_Alloc> &a,
										   const t_Char *b)
		-> BasicString<t_Char, t_Traits, t_Alloc> {
		auto result = a;
		result += b;
		return result;
	}

	/// Concatenate a C-string with a BasicString.
	template <class t_Char, class t_Traits, class t_Alloc>
	[[nodiscard]] constexpr auto operator+(const t_Char *a,
										   const BasicString<t_Char, t_Traits, t_Alloc> &b)
		-> BasicString<t_Char, t_Traits, t_Alloc> {
		auto result = BasicString<t_Char, t_Traits, t_Alloc>(a);
		result += b;
		return result;
	}

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
