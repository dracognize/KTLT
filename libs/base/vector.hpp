#pragma once

#include <concepts>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "algorithm.hpp"
#include "types.hpp"

namespace base {
	template <class t_Type, class t_Allocator = std::allocator<t_Type>>
		requires std::same_as<typename t_Allocator::value_type, t_Type>
	struct Vector {
			using value_type = t_Type;
			using size_type = usize;
			using difference_type = isize;

			using allocator_type = t_Allocator;
			using allocator_traits = std::allocator_traits<t_Allocator>;

			using reference = t_Type &;
			using const_reference = const t_Type &;

			using pointer = allocator_traits::pointer;
			using const_pointer = allocator_traits::const_pointer;

			using iterator = t_Type *;
			using const_iterator = const t_Type *;
			using reverse_iterator = std::reverse_iterator<iterator>;
			using const_reverse_iterator = std::reverse_iterator<const_iterator>;

			static constexpr size_type GrowthFactor = 2;
			static constexpr size_type InitialCapacity = 1;

			pointer _data = nullptr;
			size_type _size = 0;
			size_type _cap = 0;
			t_Allocator _alloc;

			constexpr auto
			destroy_range(pointer first,
						  pointer last) noexcept(std::is_nothrow_destructible_v<t_Type>) -> void {
				for (; first != last; ++first) {
					allocator_traits::destroy(_alloc, std::to_address(first));
				}
			}

			constexpr auto deallocate_all() noexcept(std::is_nothrow_destructible_v<t_Type>)
				-> void {
				if (_data) {
					destroy_range(_data, _data + _size);
					allocator_traits::deallocate(_alloc, _data, _cap);
					_data = nullptr;
					_size = 0;
					_cap = 0;
				}
			}

			constexpr auto transfer_to(pointer new_data, size_type new_cap) -> void {
				size_type idx = 0;
				try {
					for (; idx < _size; ++idx) {
						allocator_traits::construct(_alloc,
													std::to_address(new_data + idx),
													std::move_if_noexcept(_data[idx]));
					}
				} catch (...) {
					for (size_type j = 0; j < idx; ++j)
						allocator_traits::destroy(_alloc, std::to_address(new_data + j));
					allocator_traits::deallocate(_alloc, new_data, new_cap);
					throw;
				}
				destroy_range(_data, _data + _size);
				if (_data)
					allocator_traits::deallocate(_alloc, _data, _cap);
				_data = new_data;
				_cap = new_cap;
			}

			template <class T_Self> constexpr auto _ptr(this T_Self &&self) noexcept {
				if constexpr (std::is_const_v<std::remove_reference_t<T_Self>>)
					return static_cast<const t_Type *>(self._data);
				else
					return self._data;
			}

			constexpr Vector() noexcept(noexcept(t_Allocator{})) = default;

			constexpr explicit Vector(t_Allocator const &alloc) noexcept : _alloc(alloc) {
			}

			constexpr Vector(const size_type &count,
							 value_type const &value,
							 t_Allocator const &alloc = t_Allocator{})
				: _alloc(alloc) {
				reserve(count);
				for (size_type i = 0; i < count; ++i) {
					allocator_traits::construct(_alloc, std::to_address(_data + i), value);
				}
				_size = count;
			}

			constexpr explicit Vector(const size_type &count,
									  t_Allocator const &alloc = t_Allocator{})
				: _alloc(alloc) {
				reserve(count);
				for (size_type i = 0; i < count; ++i) {
					allocator_traits::construct(_alloc, std::to_address(_data + i));
				}
				_size = count;
			}

			template <std::input_iterator T_InputIt>
			constexpr Vector(T_InputIt first,
							 T_InputIt last,
							 t_Allocator const &alloc = t_Allocator{})
				: _alloc(alloc) {
				if constexpr (std::forward_iterator<T_InputIt>) {
					reserve(static_cast<size_type>(std::distance(first, last)));
				}
				for (; first != last; ++first) {
					if (_size == _cap)
						reserve(_cap == 0 ? InitialCapacity : _cap * GrowthFactor);
					allocator_traits::construct(_alloc, std::to_address(_data + _size), *first);
					++_size;
				}
			}

			constexpr Vector(Vector const &other) noexcept(
				std::is_nothrow_copy_constructible_v<t_Type>)
				: _alloc(allocator_traits::select_on_container_copy_construction(other._alloc)) {
				reserve(other._size);
				for (size_type i = 0; i < other._size; ++i) {
					allocator_traits::construct(_alloc, std::to_address(_data + i), other._data[i]);
				}
				_size = other._size;
			}

			constexpr auto
			operator=(Vector const &other) noexcept(std::is_nothrow_copy_constructible_v<t_Type>)
				-> Vector & {
				if (this != &other) {
					clear();
					reserve(other._size);
					for (size_type i = 0; i < other._size; ++i) {
						allocator_traits::construct(
							_alloc, std::to_address(_data + i), other._data[i]);
					}
					_size = other._size;
				}
				return *this;
			}

			constexpr Vector(Vector &&other) noexcept(
				std::is_nothrow_move_constructible_v<t_Allocator>)
				: _data(other._data), _size(other._size), _cap(other._cap),
				  _alloc(std::move(other._alloc)) {
				other._data = nullptr;
				other._size = 0;
				other._cap = 0;
			}

			constexpr Vector(Vector &&other, t_Allocator const &alloc) : _alloc(alloc) {
				if constexpr (allocator_traits::is_always_equal::value || _alloc == other._alloc) {
					_data = other._data;
					_size = other._size;
					_cap = other._cap;
					other._data = nullptr;
					other._size = 0;
					other._cap = 0;
				} else {
					reserve(other._size);
					for (size_type i = 0; i < other._size; ++i) {
						allocator_traits::construct(
							_alloc, std::to_address(_data + i), std::move(other._data[i]));
					}
					_size = other._size;
				}
			}

			constexpr auto
			operator=(Vector &&other) noexcept(std::is_nothrow_move_assignable_v<t_Allocator>)
				-> Vector & {
				if (this != &other) {
					deallocate_all();
					_data = other._data;
					_size = other._size;
					_cap = other._cap;
					_alloc = std::move(other._alloc);
					other._data = nullptr;
					other._size = 0;
					other._cap = 0;
				}
				return *this;
			}

			constexpr Vector(std::initializer_list<t_Type> init,
							 t_Allocator const &alloc = t_Allocator{})
				: _alloc(alloc) {
				reserve(init.size());
				for (auto &val : init) {
					allocator_traits::construct(_alloc, std::to_address(_data + _size), val);
					++_size;
				}
			}

			constexpr ~Vector() {
				deallocate_all();
			}

			constexpr auto assign(const size_type &count, value_type const &value) -> void {
				clear();
				reserve(count);
				for (size_type i = 0; i < count; ++i) {
					allocator_traits::construct(_alloc, std::to_address(_data + i), value);
				}
				_size = count;
			}

			template <std::input_iterator T_InputIt>
			constexpr auto assign(T_InputIt first, T_InputIt last) -> void {
				clear();
				for (; first != last; ++first) {
					if (_size == _cap)
						reserve(_cap == 0 ? InitialCapacity : _cap * GrowthFactor);
					allocator_traits::construct(_alloc, std::to_address(_data + _size), *first);
					++_size;
				}
			}

			constexpr auto assign(std::initializer_list<t_Type> ilist) -> void {
				assign(ilist.begin(), ilist.end());
			}

			constexpr auto operator[](this auto &&self, size_type pos) noexcept -> decltype(auto) {
				return (self._ptr()[pos]);
			}

			constexpr auto at(this auto &&self, size_type pos) -> decltype(auto) {
				if (pos >= self._size)
					throw std::out_of_range("Vector::at");
				return (self._ptr()[pos]);
			}

			constexpr auto front(this auto &&self) noexcept -> decltype(auto) {
				return (self._ptr()[0]);
			}

			constexpr auto back(this auto &&self) noexcept -> decltype(auto) {
				return (self._ptr()[self._size - 1]);
			}

			constexpr auto data(this auto &&self) noexcept {
				return self._ptr();
			}

			constexpr auto begin(this auto &&self) noexcept {
				return self._ptr();
			}

			constexpr auto end(this auto &&self) noexcept {
				return self._ptr() + self._size;
			}

			constexpr auto cbegin() const noexcept -> const_iterator {
				return static_cast<const t_Type *>(_data);
			}

			constexpr auto cend() const noexcept -> const_iterator {
				return static_cast<const t_Type *>(_data) + _size;
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

			[[nodiscard]] constexpr auto empty() const noexcept -> bool {
				return _size == 0;
			}

			[[nodiscard]] constexpr auto size() const noexcept -> size_type {
				return _size;
			}

			[[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
				return allocator_traits::max_size(_alloc);
			}

			[[nodiscard]] constexpr auto capacity() const noexcept -> size_type {
				return _cap;
			}

			constexpr auto reserve(size_type new_cap) -> void {
				if (new_cap <= _cap)
					return;
				transfer_to(allocator_traits::allocate(_alloc, new_cap), new_cap);
			}

			constexpr auto shrink_to_fit() -> void {
				if (_size == _cap)
					return;
				if (_size == 0) {
					deallocate_all();
					return;
				}
				transfer_to(allocator_traits::allocate(_alloc, _size), _size);
			}

			constexpr auto clear() noexcept -> void {
				destroy_range(_data, _data + _size);
				_size = 0;
			}

			template <class... T_Args>
			constexpr auto emplace(const_iterator pos, T_Args &&...args) -> iterator {
				auto const idx = static_cast<size_type>(pos - _data);
				if (_size == _cap) {
					auto const new_cap
						= _cap == 0 ? size_type{InitialCapacity} : _cap * GrowthFactor;
					auto new_data = allocator_traits::allocate(_alloc, new_cap);
					size_type i = 0;
					try {
						for (; i < idx; ++i)
							allocator_traits::construct(_alloc,
														std::to_address(new_data + i),
														std::move_if_noexcept(_data[i]));
						allocator_traits::construct(
							_alloc, std::to_address(new_data + idx), std::forward<T_Args>(args)...);
						++i;
						for (; i <= _size; ++i)
							allocator_traits::construct(_alloc,
														std::to_address(new_data + i),
														std::move_if_noexcept(_data[i - 1]));
					} catch (...) {
						for (size_type j = 0; j < i; ++j)
							allocator_traits::destroy(_alloc, std::to_address(new_data + j));
						allocator_traits::deallocate(_alloc, new_data, new_cap);
						throw;
					}
					destroy_range(_data, _data + _size);
					allocator_traits::deallocate(_alloc, _data, _cap);
					_data = new_data;
					_cap = new_cap;
					++_size;
					return _data + idx;
				}
				auto const dest = _data + idx;
				allocator_traits::construct(
					_alloc, std::to_address(_data + _size), std::move(_data[_size - 1]));
				++_size;
				for (auto p = _data + _size - 1; p != dest; --p)
					*p = std::move(p[-1]);
				allocator_traits::destroy(_alloc, std::to_address(dest));
				allocator_traits::construct(
					_alloc, std::to_address(dest), std::forward<T_Args>(args)...);
				return dest;
			}

			constexpr auto insert(const_iterator pos, value_type const &value) -> iterator {
				return emplace(pos, value);
			}

			constexpr auto insert(const_iterator pos, value_type &&value) -> iterator {
				return emplace(pos, std::move(value));
			}

			constexpr auto erase(const_iterator pos) -> iterator {
				auto const idx = static_cast<size_type>(pos - _data);
				auto const p = _data + idx;
				allocator_traits::destroy(_alloc, std::to_address(p));
				for (auto q = p + 1; q != _data + _size; ++q) {
					allocator_traits::construct(_alloc, std::to_address(q - 1), std::move(*q));
					allocator_traits::destroy(_alloc, std::to_address(q));
				}
				--_size;
				return p;
			}

			constexpr auto erase(const_iterator first, const_iterator last) -> iterator {
				if (first == last)
					return _data + (first - begin());
				auto const count = static_cast<size_type>(last - first);
				auto const idx = static_cast<size_type>(first - _data);
				auto const dest = _data + idx;
				destroy_range(const_cast<pointer>(first), const_cast<pointer>(last));
				auto const src = dest + count;
				auto const end_ptr = _data + _size;
				for (auto p = src; p != end_ptr; ++p) {
					allocator_traits::construct(_alloc, std::to_address(p - count), std::move(*p));
					allocator_traits::destroy(_alloc, std::to_address(p));
				}
				_size -= count;
				return dest;
			}

			constexpr auto push_back(value_type const &value) -> void {
				if (_size == _cap)
					reserve(_cap == 0 ? InitialCapacity : _cap * GrowthFactor);
				allocator_traits::construct(_alloc, std::to_address(_data + _size), value);
				++_size;
			}

			constexpr auto push_back(value_type &&value) -> void {
				if (_size == _cap)
					reserve(_cap == 0 ? InitialCapacity : _cap * GrowthFactor);
				allocator_traits::construct(
					_alloc, std::to_address(_data + _size), std::move(value));
				++_size;
			}

			template <class... T_Args> constexpr auto emplace_back(T_Args &&...args) -> reference {
				if (_size == _cap)
					reserve(_cap == 0 ? InitialCapacity : _cap * GrowthFactor);
				allocator_traits::construct(
					_alloc, std::to_address(_data + _size), std::forward<T_Args>(args)...);
				++_size;
				return _data[_size - 1];
			}

			constexpr auto pop_back() -> void {
				allocator_traits::destroy(_alloc, std::to_address(_data + _size - 1));
				--_size;
			}

			constexpr auto resize(size_type count) -> void {
				if (count < _size) {
					destroy_range(_data + count, _data + _size);
				} else if (count > _size) {
					reserve(count);
					for (size_type i = _size; i < count; ++i)
						allocator_traits::construct(_alloc, std::to_address(_data + i));
				}
				_size = count;
			}

			constexpr auto resize(size_type count, value_type const &value) -> void {
				if (count < _size) {
					destroy_range(_data + count, _data + _size);
				} else if (count > _size) {
					reserve(count);
					for (size_type i = _size; i < count; ++i)
						allocator_traits::construct(_alloc, std::to_address(_data + i), value);
				}
				_size = count;
			}

			constexpr auto swap(Vector &other) noexcept(std::is_nothrow_swappable_v<t_Allocator>)
				-> void {
				base::swap(_data, other._data);
				base::swap(_size, other._size);
				base::swap(_cap, other._cap);
				base::swap(_alloc, other._alloc);
			}

			constexpr auto get_allocator() const noexcept -> allocator_type {
				return _alloc;
			}
	};

	template <class t_Type, class t_Allocator>
	constexpr auto swap(Vector<t_Type, t_Allocator> &a,
						Vector<t_Type, t_Allocator> &b) noexcept(noexcept(a.swap(b))) -> void {
		a.swap(b);
	}
} 
