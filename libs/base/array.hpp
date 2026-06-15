#pragma once

#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "types.hpp"
#include "algorithm.hpp"
#include "vector.hpp"

namespace base {
	namespace detail {
		template <class t_Type, usize t_Size>
		struct array_traits {
			using _storage_type         = t_Type[t_Size];
			using _is_swappable         = std::is_swappable<t_Type>;
			using _is_nothrow_swappable = std::is_nothrow_swappable<t_Type>;
		};

		template <class t_Type>
		struct array_traits<t_Type, 0> {
			struct _storage_type {
				[[noreturn]] constexpr t_Type& operator[](usize) const {
					std::unreachable();
				}

				constexpr explicit operator t_Type*() const {
					return nullptr;
				}
			};

			using _is_swappable         = std::true_type;
			using _is_nothrow_swappable = std::true_type;
		};
	}

	template <class t_Type, usize t_Size>
	struct Array {
		using value_type      = t_Type;
		using size_type       = usize;
		using difference_type = isize;

		using reference       = t_Type&;
		using const_reference = const t_Type&;

		using pointer       = t_Type*;
		using const_pointer = const t_Type*;

		using iterator       = t_Type*;
		using const_iterator = const t_Type*;

		using reverse_iterator       = std::reverse_iterator<t_Type*>;
		using const_reverse_iterator = std::reverse_iterator<const t_Type*>;

		detail::array_traits<t_Type, t_Size>::_storage_type _data;

		constexpr auto fill(const value_type& value) noexcept -> void {
			if constexpr (t_Size > 0) {
				for (auto& it : _data) {
					it = value;
				}
			}
		}

		constexpr auto swap(Array& other) noexcept -> void {
			for (usize i{}; i < t_Size; ++i) {
				base::swap(_data[i], other._data[i]);
			}
		}

		[[nodiscard]] constexpr auto size() const noexcept -> usize {
			return t_Size;
		}

		[[nodiscard]] constexpr auto max_size() const noexcept -> usize {
			return t_Size;
		}

		[[nodiscard]] constexpr auto empty() const noexcept -> bool {
			return t_Size == 0;
		}

		constexpr auto begin(this auto&& self) noexcept {
			using ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<decltype(self)>>,
				const_pointer, pointer>;
			return static_cast<ptr>(self._data);
		}

		constexpr auto end(this auto&& self) noexcept {
			using ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<decltype(self)>>,
				const_pointer, pointer>;
			return static_cast<ptr>(self._data) + t_Size;
		}

		constexpr auto cbegin() const noexcept -> const_iterator {
			return static_cast<const_iterator>(_data);
		}

		constexpr auto cend() const noexcept -> const_iterator {
			return static_cast<const_iterator>(_data) + t_Size;
		}

		constexpr auto rbegin(this auto&& self) noexcept {
			using ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<decltype(self)>>,
				const_pointer, pointer>;
			return std::make_reverse_iterator(static_cast<ptr>(self._data) + t_Size);
		}

		constexpr auto rend(this auto&& self) noexcept {
			using ptr = std::conditional_t<
				std::is_const_v<std::remove_reference_t<decltype(self)>>,
				const_pointer, pointer>;
			return std::make_reverse_iterator(static_cast<ptr>(self._data));
		}

		constexpr auto crbegin() const noexcept -> const_reverse_iterator {
			return std::make_reverse_iterator(static_cast<const_iterator>(_data) + t_Size);
		}

		constexpr auto crend() const noexcept -> const_reverse_iterator {
			return std::make_reverse_iterator(static_cast<const_iterator>(_data));
		}

		constexpr auto operator[](this auto&& self, usize index) noexcept -> decltype(auto) {
			return self._data[index];
		}

		constexpr auto at(this auto&& self, usize index) -> decltype(auto) {
			if (index >= t_Size) {
				throw std::out_of_range{"index out of bounds"};
			}
			return self._data[index];
		}

		constexpr auto front(this auto&& self) noexcept -> decltype(auto) {
			return self._data[0];
		}

		constexpr auto back(this auto&& self) noexcept -> decltype(auto) {
			return self._data[t_Size - 1];
		}

		constexpr auto data() noexcept -> pointer {
			return static_cast<pointer>(_data);
		}

		constexpr auto data() const noexcept -> const_pointer {
			return static_cast<const_pointer>(_data);
		}

		constexpr auto operator==(const Array&) const -> bool = default;

		constexpr auto operator<=>(const Array&) const = default;
	};

	template <class t_Type, class... t_Types>
		requires (std::is_same_v<t_Type, t_Types> && ...)
	Array(t_Type, t_Types...) -> Array<t_Type, 1 + sizeof...(t_Types)>;

	template <class t_Type, usize t_Size>
	constexpr auto swap(Array<t_Type, t_Size>& lhs, Array<t_Type, t_Size>& rhs) noexcept -> void {
		lhs.swap(rhs);
	}

	template <usize t_Index, class t_Type, usize t_Size>
		requires (t_Index < t_Size)
	constexpr auto get(Array<t_Type, t_Size>& arr) noexcept -> t_Type& {
		return arr._data[t_Index];
	}

	template <usize t_Index, class t_Type, usize t_Size>
		requires(t_Index < t_Size)
	constexpr auto get(const Array<t_Type, t_Size>& arr) -> const t_Type& {
		return arr._data[t_Index];
	}

	template <usize t_Index, class t_Type, usize t_Size>
		requires(t_Index < t_Size)
	constexpr auto get(Array<t_Type, t_Size>&& arr) -> t_Type&& {
		return static_cast<t_Type&&>(arr._data[t_Index]);
	}

	template <usize t_Index, class t_Type, usize t_Size>
		requires(t_Index < t_Size)
	constexpr auto get(const Array<t_Type, t_Size>&& arr) -> const t_Type&& {
		return static_cast<const t_Type&&>(arr._data[t_Index]);
	}

	namespace detail {
		template <class t_Type, std::size_t t_Size, std::size_t... t_Is>
		constexpr auto to_array_copy_impl(t_Type (&_src)[t_Size], std::index_sequence<t_Is...>)
			-> Array<std::remove_cv_t<t_Type>, t_Size> {
			return {{_src[t_Is]...}};
		}

		template <class t_Type, std::size_t t_Size, std::size_t... t_Is>
		constexpr auto to_array_move_impl(t_Type (&&_src)[t_Size], std::index_sequence<t_Is...>)
			-> Array<std::remove_cv_t<t_Type>, t_Size> {
			return {{std::move(_src[t_Is])...}};
		}
	}

	template <class t_Type, std::size_t t_Size>
	constexpr auto to_array(t_Type (&_src)[t_Size]) -> Array<std::remove_cv_t<t_Type>, t_Size> {
		return detail::to_array_copy_impl(_src, std::make_index_sequence<t_Size>());
	}

	template <class t_Type, std::size_t t_Size>
	constexpr auto to_array(t_Type (&&_src)[t_Size]) -> Array<std::remove_cv_t<t_Type>, t_Size> {
		return detail::to_array_move_impl(_src, std::make_index_sequence<t_Size>());
	}
}

namespace std {
	template <class t_Type, std::size_t t_Size>
	struct tuple_size<base::Array<t_Type, t_Size>> : integral_constant<std::size_t, t_Size> {};

	template <std::size_t t_Index, class t_Type, std::size_t t_Size>
	struct tuple_element<t_Index, base::Array<t_Type, t_Size>> {
		using type = t_Type;
	};
}