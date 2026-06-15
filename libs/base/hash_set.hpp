#pragma once

#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#include "hash_table.hpp"
#include "types.hpp"

namespace base {

	template <class t_Key, class t_Hash = hash<t_Key>, class t_KeyEqual = std::equal_to<t_Key>,
			  class t_Allocator = std::allocator<t_Key>>
	struct HashSet {
			// --- Types ---
			using key_type		  = t_Key;
			using value_type	  = t_Key;
			using size_type		  = usize;
			using difference_type = isize;
			using hasher		  = t_Hash;
			using key_equal		  = t_KeyEqual;
			using allocator_type  = t_Allocator;
			using reference		  = const value_type&;
			using const_reference = const value_type&;
			using pointer		  = const value_type*;
			using const_pointer	  = const value_type*;

		private:
			using _base = HashTable<t_Key, t_Key, t_Hash, t_KeyEqual, std::allocator<std::pair<const t_Key, t_Key>>>;
			_base _table;

		public:
			// --- Iterators ---
			using iterator			   = typename _base::const_iterator;
			using const_iterator	   = typename _base::const_iterator;
			using local_iterator	   = typename _base::const_iterator;
			using const_local_iterator = typename _base::const_iterator;

			// --- Construction ---
			constexpr HashSet() = default;

			constexpr explicit HashSet(const t_Hash& hasher, const t_KeyEqual& equal = t_KeyEqual{},
									   const t_Allocator& alloc = t_Allocator{}) :
				_table(hasher, equal, std::allocator<std::pair<const t_Key, t_Key>>(alloc)) {
			}

			constexpr explicit HashSet(const t_Allocator& alloc) noexcept : _table(alloc) {
			}

			template <std::input_iterator T_InputIt>
			constexpr HashSet(T_InputIt first, T_InputIt last, const t_Hash& hasher = t_Hash{},
							  const t_KeyEqual& equal = t_KeyEqual{}, const t_Allocator& alloc = t_Allocator{}) :
				_table(hasher, equal, std::allocator<std::pair<const t_Key, t_Key>>(alloc)) {
				insert(first, last);
			}

			constexpr HashSet(std::initializer_list<value_type> init, const t_Hash& hasher = t_Hash{},
							  const t_KeyEqual& equal = t_KeyEqual{}, const t_Allocator& alloc = t_Allocator{}) :
				_table(init.size(), hasher, equal, std::allocator<std::pair<const t_Key, t_Key>>(alloc)) {
				insert(init);
			}

			constexpr HashSet(const HashSet&) = default;
			constexpr HashSet(HashSet&&)	  = default;
			constexpr ~HashSet()			  = default;

			constexpr auto operator=(const HashSet&) -> HashSet&	 = default;
			constexpr auto operator=(HashSet&&) noexcept -> HashSet& = default;
			constexpr auto operator=(std::initializer_list<value_type> init) -> HashSet& {
				_table = init;
				return *this;
			}

			// --- Iterators (always const) ---
			constexpr auto begin() const noexcept -> const_iterator {
				return _table.cbegin();
			}

			constexpr auto end() const noexcept -> const_iterator {
				return _table.cend();
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
				auto result = _table.insert({value, value});
				return {iterator(result.first._bucket, result.first._buckets_end, result.first._node), result.second};
			}

			constexpr auto insert(value_type&& value) -> std::pair<iterator, bool> {
				auto result = _table.insert({std::move(value), t_Key{}});
				return {iterator(result.first._bucket, result.first._buckets_end, result.first._node), result.second};
			}

			template <std::input_iterator T_InputIt>
			constexpr auto insert(T_InputIt first, T_InputIt last) -> void {
				for (; first != last; ++first)
					insert(*first);
			}

			constexpr auto insert(std::initializer_list<value_type> init) -> void {
				insert(init.begin(), init.end());
			}

			template <class... T_Args>
			constexpr auto emplace(T_Args&&... args) -> std::pair<iterator, bool> {
				return insert(value_type(std::forward<T_Args>(args)...));
			}

			constexpr auto erase(const_iterator pos) -> iterator {
				return _table.erase(pos);
			}

			constexpr auto erase(const key_type& key) -> size_type {
				return _table.erase(key);
			}

			constexpr auto swap(HashSet& other) noexcept -> void {
				_table.swap(other._table);
			}

			constexpr auto extract(const_iterator pos) -> value_type {
				auto val = pos->first;
				_table.erase(pos);
				return val;
			}

			// --- Lookup ---
			constexpr auto find(const key_type& key) const -> const_iterator {
				return _table.find(key);
			}

			constexpr auto contains(const key_type& key) const -> bool {
				return _table.contains(key);
			}

			constexpr auto count(const key_type& key) const -> size_type {
				return _table.contains(key) ? 1 : 0;
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
				return allocator_type(_table.get_allocator());
			}
	};

	template <class t_Key, class t_Hash, class t_KeyEqual, class t_Alloc>
	constexpr auto swap(HashSet<t_Key, t_Hash, t_KeyEqual, t_Alloc>& a,
						HashSet<t_Key, t_Hash, t_KeyEqual, t_Alloc>& b) noexcept(noexcept(a.swap(b))) -> void {
		a.swap(b);
	}

} // namespace base
