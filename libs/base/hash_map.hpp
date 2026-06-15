#pragma once

#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "hash_table.hpp"
#include "types.hpp"

namespace base {

	template <class t_Key, class t_Value, class t_Hash = hash<t_Key>, class t_KeyEqual = std::equal_to<t_Key>,
			  class t_Allocator = std::allocator<std::pair<const t_Key, t_Value>>>
	struct HashMap {
			// --- Types ---
			using key_type		  = t_Key;
			using mapped_type	  = t_Value;
			using value_type	  = std::pair<const t_Key, t_Value>;
			using size_type		  = usize;
			using difference_type = isize;
			using hasher		  = t_Hash;
			using key_equal		  = t_KeyEqual;
			using allocator_type  = t_Allocator;
			using reference		  = value_type&;
			using const_reference = const value_type&;
			using pointer		  = value_type*;
			using const_pointer	  = const value_type*;

		private:
			using _base = HashTable<t_Key, t_Value, t_Hash, t_KeyEqual, t_Allocator>;
			_base _table;

		public:
			// --- Iterators ---
			using iterator			   = typename _base::iterator;
			using const_iterator	   = typename _base::const_iterator;
			using local_iterator	   = typename _base::iterator;
			using const_local_iterator = typename _base::const_iterator;

			// --- Construction ---
			constexpr HashMap() = default;

			constexpr explicit HashMap(const t_Hash& hasher, const t_KeyEqual& equal = t_KeyEqual{},
									   const t_Allocator& alloc = t_Allocator{}) noexcept :
				_table(hasher, equal, alloc) {
			}

			constexpr explicit HashMap(const t_Allocator& alloc) noexcept : _table(alloc) {
			}

			template <std::input_iterator T_InputIt>
			constexpr HashMap(T_InputIt first, T_InputIt last, const t_Hash& hasher = t_Hash{},
							  const t_KeyEqual& equal = t_KeyEqual{}, const t_Allocator& alloc = t_Allocator{}) :
				_table(hasher, equal, alloc) {
				insert(first, last);
			}

			constexpr HashMap(std::initializer_list<value_type> init, const t_Hash& hasher = t_Hash{},
							  const t_KeyEqual& equal = t_KeyEqual{}, const t_Allocator& alloc = t_Allocator{}) :
				_table(init, hasher, equal, alloc) {
			}

			constexpr HashMap(const HashMap&) = default;
			constexpr HashMap(HashMap&&)	  = default;
			constexpr ~HashMap()			  = default;

			constexpr auto operator=(const HashMap&) -> HashMap&	 = default;
			constexpr auto operator=(HashMap&&) noexcept -> HashMap& = default;
			constexpr auto operator=(std::initializer_list<value_type> init) -> HashMap& {
				_table = init;
				return *this;
			}

			// --- Iterators ---
			constexpr auto begin() noexcept -> iterator {
				return _table.begin();
			}

			constexpr auto begin() const noexcept -> const_iterator {
				return _table.begin();
			}

			constexpr auto end() noexcept -> iterator {
				return _table.end();
			}

			constexpr auto end() const noexcept -> const_iterator {
				return _table.end();
			}

			constexpr auto cbegin() const noexcept -> const_iterator {
				return _table.cbegin();
			}

			constexpr auto cend() const noexcept -> const_iterator {
				return _table.cend();
			}

			// --- Capacity ---
			[[nodiscard]] constexpr auto empty() const noexcept -> bool {
				return _table.empty();
			}

			[[nodiscard]] constexpr auto size() const noexcept -> size_type {
				return _table.size();
			}

			[[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
				return _table.max_size();
			}

			// --- Modifiers ---
			constexpr auto clear() noexcept -> void {
				_table.clear();
			}

			constexpr auto insert(const value_type& value) -> std::pair<iterator, bool> {
				return _table.insert(value);
			}

			constexpr auto insert(value_type&& value) -> std::pair<iterator, bool> {
				return _table.insert(std::move(value));
			}

			template <std::input_iterator T_InputIt>
			constexpr auto insert(T_InputIt first, T_InputIt last) -> void {
				for (; first != last; ++first)
					_table.insert(*first);
			}

			constexpr auto insert(std::initializer_list<value_type> init) -> void {
				for (const auto& val : init)
					_table.insert(val);
			}

			template <class... T_Args>
			constexpr auto emplace(T_Args&&... args) -> std::pair<iterator, bool> {
				return _table.emplace(std::forward<T_Args>(args)...);
			}

			constexpr auto erase(const_iterator pos) -> iterator {
				return _table.erase(pos);
			}

			constexpr auto erase(const_iterator first, const_iterator last) -> iterator {
				while (first != last)
					first = erase(first);
				return iterator(_table.end());
			}

			constexpr auto erase(const key_type& key) -> size_type {
				return _table.erase(key);
			}

			constexpr auto swap(HashMap& other) noexcept -> void {
				_table.swap(other._table);
			}

			// --- Element access ---
			constexpr auto at(const key_type& key) -> mapped_type& {
				auto it = _table.find(key);
				if (it == _table.end())
					throw std::out_of_range("HashMap::at");
				return it->second;
			}

			constexpr auto at(const key_type& key) const -> const mapped_type& {
				auto it = _table.find(key);
				if (it == _table.end())
					throw std::out_of_range("HashMap::at");
				return it->second;
			}

			constexpr auto operator[](const key_type& key) -> mapped_type& {
				auto result =
					_table.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				return result.first->second;
			}

			constexpr auto operator[](key_type&& key) -> mapped_type& {
				auto result = _table.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)),
											 std::forward_as_tuple());
				return result.first->second;
			}

			// --- Lookup ---
			constexpr auto find(const key_type& key) -> iterator {
				return _table.find(key);
			}

			constexpr auto find(const key_type& key) const -> const_iterator {
				return _table.find(key);
			}

			constexpr auto contains(const key_type& key) const -> bool {
				return _table.contains(key);
			}

			constexpr auto count(const key_type& key) const -> size_type {
				return _table.contains(key) ? 1 : 0;
			}

			// --- Convenience modifiers ---
			template <class... T_Args>
			constexpr auto try_emplace(const key_type& key, T_Args&&... args) -> std::pair<iterator, bool> {
				auto it = _table.find(key);
				if (it != _table.end())
					return {it, false};
				return _table.emplace(std::piecewise_construct, std::forward_as_tuple(key),
									  std::forward_as_tuple(std::forward<T_Args>(args)...));
			}

			template <class... T_Args>
			constexpr auto try_emplace(key_type&& key, T_Args&&... args) -> std::pair<iterator, bool> {
				auto it = _table.find(key);
				if (it != _table.end())
					return {it, false};
				return _table.emplace(std::piecewise_construct, std::forward_as_tuple(std::move(key)),
									  std::forward_as_tuple(std::forward<T_Args>(args)...));
			}

			constexpr auto insert_or_assign(const key_type& key, mapped_type&& val) -> std::pair<iterator, bool> {
				auto it = _table.find(key);
				if (it != _table.end()) {
					it->second = std::move(val);
					return {it, false};
				}
				return _table.insert(value_type{key, std::move(val)});
			}

			constexpr auto insert_or_assign(const key_type& key, const mapped_type& val) -> std::pair<iterator, bool> {
				auto it = _table.find(key);
				if (it != _table.end()) {
					it->second = val;
					return {it, false};
				}
				return _table.insert(value_type{key, val});
			}

			// --- Bucket interface ---
			constexpr auto bucket_count() const noexcept -> size_type {
				return _table.bucket_count();
			}

			constexpr auto bucket_size(size_type n) const noexcept -> size_type {
				return _table.bucket_size(n);
			}

			constexpr auto bucket(const key_type& key) const -> size_type {
				return _table.bucket(key);
			}

			// --- Hash policy ---
			constexpr auto load_factor() const noexcept -> float {
				return _table.load_factor();
			}

			constexpr auto max_load_factor() const noexcept -> float {
				return _table.max_load_factor();
			}

			constexpr auto max_load_factor(float ml) noexcept -> void {
				_table.max_load_factor(ml);
			}

			constexpr auto rehash(size_type n) -> void {
				_table.rehash(n);
			}

			constexpr auto reserve(size_type n) -> void {
				_table.reserve(n);
			}

			// --- Observers ---
			constexpr auto hash_function() const -> hasher {
				return _table.hash_function();
			}

			constexpr auto key_eq() const -> key_equal {
				return _table.key_eq();
			}

			constexpr auto get_allocator() const noexcept -> allocator_type {
				return _table.get_allocator();
			}
	};

	template <class t_Key, class t_Value, class t_Hash, class t_KeyEqual, class t_Alloc>
	constexpr auto swap(HashMap<t_Key, t_Value, t_Hash, t_KeyEqual, t_Alloc>& a,
						HashMap<t_Key, t_Value, t_Hash, t_KeyEqual, t_Alloc>& b) noexcept(noexcept(a.swap(b))) -> void {
		a.swap(b);
	}

} // namespace base
