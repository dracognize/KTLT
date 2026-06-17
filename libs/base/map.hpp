#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include "rbtree.hpp"
#include "types.hpp"

namespace base {
	template <class t_Key,
			  class t_Value,
			  class t_Comparator = std::less<t_Key>,
			  class t_Allocator = std::allocator<std::pair<const t_Key, t_Value>>>
	struct Map {
			// --- Types ---
			using key_type = t_Key;
			using mapped_type = t_Value;
			using value_type = std::pair<const key_type, mapped_type>;
			using key_compare = t_Comparator;
			using allocator_type = t_Allocator;
			using size_type = usize;
			using difference_type = isize;
			using reference = value_type &;
			using const_reference = const value_type &;
			using pointer = value_type *;
			using const_pointer = const value_type *;

			// --- Comparator adapter ---
			struct value_compare {
					key_compare _comp;

					constexpr value_compare() = default;

					constexpr explicit value_compare(key_compare comp) : _comp(std::move(comp)) {
					}

					constexpr auto operator()(const value_type &a, const value_type &b) const
						-> bool {
						return _comp(a.first, b.first);
					}
			};

		private:
			using _base = RedBlackTree<value_type, value_compare, t_Allocator>;
			_base _tree;

		public:
			// --- Iterators ---
			using iterator = typename _base::iterator;
			using const_iterator = typename _base::const_iterator;
			using reverse_iterator = typename _base::reverse_iterator;
			using const_reverse_iterator = typename _base::const_reverse_iterator;

			// --- Construction ---
			Map() = default;

			explicit Map(const key_compare &comp, const allocator_type &alloc = allocator_type{})
				: _tree(value_compare(comp), alloc) {
			}

			explicit Map(const allocator_type &alloc) : _tree(alloc) {
			}

			template <std::input_iterator T_InputIt>
			Map(T_InputIt first,
				T_InputIt last,
				const key_compare &comp = key_compare{},
				const allocator_type &alloc = allocator_type{})
				: _tree(first, last, value_compare(comp), alloc) {
			}

			Map(std::initializer_list<value_type> init,
				const key_compare &comp = key_compare{},
				const allocator_type &alloc = allocator_type{})
				: _tree(init, value_compare(comp), alloc) {
			}

			Map(const Map &) = default;
			Map(Map &&) = default;

			// --- Assignment ---
			auto operator=(const Map &) -> Map & = default;
			auto operator=(Map &&) -> Map & = default;

			auto operator=(std::initializer_list<value_type> ilist) -> Map & {
				_tree.clear();
				_tree.insert(ilist);
				return *this;
			}

			// --- Iterators ---
			auto begin() noexcept -> iterator {
				return _tree.begin();
			}
			auto begin() const noexcept -> const_iterator {
				return _tree.begin();
			}
			auto end() noexcept -> iterator {
				return _tree.end();
			}
			auto end() const noexcept -> const_iterator {
				return _tree.end();
			}
			auto cbegin() const noexcept -> const_iterator {
				return _tree.cbegin();
			}
			auto cend() const noexcept -> const_iterator {
				return _tree.cend();
			}
			auto rbegin() noexcept -> reverse_iterator {
				return _tree.rbegin();
			}
			auto rend() noexcept -> reverse_iterator {
				return _tree.rend();
			}
			auto crbegin() const noexcept -> const_reverse_iterator {
				return _tree.crbegin();
			}
			auto crend() const noexcept -> const_reverse_iterator {
				return _tree.crend();
			}

			// --- Capacity ---
			[[nodiscard]] auto empty() const noexcept -> bool {
				return _tree.empty();
			}
			[[nodiscard]] auto size() const noexcept -> size_type {
				return _tree.size();
			}
			[[nodiscard]] auto max_size() const noexcept -> size_type {
				return _tree.max_size();
			}

			// --- Element access ---
			auto at(const key_type &key) -> mapped_type & {
				auto it = find(key);
				if (it == end())
					throw std::out_of_range("Map::at");
				return it->second;
			}

			auto at(const key_type &key) const -> const mapped_type & {
				auto it = find(key);
				if (it == end())
					throw std::out_of_range("Map::at");
				return it->second;
			}

			auto operator[](const key_type &key) -> mapped_type & {
				return _tree
					.insert(value_type(std::piecewise_construct,
									   std::forward_as_tuple(key),
									   std::forward_as_tuple()))
					.first->second;
			}

			// --- Modifiers ---
			auto clear() noexcept -> void {
				_tree.clear();
			}

			auto insert(const value_type &val) -> std::pair<iterator, bool> {
				auto r = _tree.insert(val);
				return {iterator(r.first), r.second};
			}

			auto insert(const_iterator hint, const value_type &val) -> iterator {
				return _tree.insert(hint, val);
			}

			template <std::input_iterator T_InputIt>
			auto insert(T_InputIt first, T_InputIt last) -> void {
				_tree.insert(first, last);
			}

			auto insert(std::initializer_list<value_type> ilist) -> void {
				_tree.insert(ilist);
			}

			template <class... Args>
			auto try_emplace(const key_type &key, Args &&...args) -> std::pair<iterator, bool> {
				// Avoid constructing the mapped value if key already exists
				auto it = find(key);
				if (it != end())
					return {it, false};
				return _tree.insert(value_type(std::piecewise_construct,
											   std::forward_as_tuple(key),
											   std::forward_as_tuple(std::forward<Args>(args)...)));
			}

			auto insert_or_assign(const key_type &key, mapped_type &&val)
				-> std::pair<iterator, bool> {
				auto it = find(key);
				if (it != end()) {
					it->second = std::move(val);
					return {it, false};
				}
				return _tree.insert(value_type(std::piecewise_construct,
											   std::forward_as_tuple(key),
											   std::forward_as_tuple(std::move(val))));
			}

			auto insert_or_assign(const key_type &key, const mapped_type &val)
				-> std::pair<iterator, bool> {
				auto it = find(key);
				if (it != end()) {
					it->second = val;
					return {it, false};
				}
				return _tree.insert(value_type(std::piecewise_construct,
											   std::forward_as_tuple(key),
											   std::forward_as_tuple(val)));
			}

			auto erase(const_iterator pos) -> iterator {
				return _tree.erase(pos);
			}

			auto erase(const_iterator first, const_iterator last) -> iterator {
				return _tree.erase(first, last);
			}

			auto erase(const key_type &key) -> size_type {
				value_type probe(
					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				return _tree.erase(probe);
			}

			auto swap(Map &other) noexcept(noexcept(_tree.swap(other._tree))) -> void {
				_tree.swap(other._tree);
			}

			auto extract(const_iterator pos) -> value_type {
				auto val = *pos; // copy (const_iterator can't move)
				_tree.erase(pos);
				return val;
			}

			// --- Lookup ---
			auto find(const key_type &key) -> iterator {
				value_type probe(
					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				auto it = _tree.find(probe);
				return iterator(it._node, it._nil);
			}

			auto find(const key_type &key) const -> const_iterator {
				value_type probe(
					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				auto it = _tree.find(probe);
				return const_iterator(it._node, it._nil);
			}

			auto contains(const key_type &key) const -> bool {
				value_type probe(
					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				return _tree.contains(probe);
			}

			auto count(const key_type &key) const -> size_type {
				return contains(key) ? 1 : 0;
			}

			auto lower_bound(const key_type &key) -> iterator {
				value_type probe(
					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				auto it = _tree.lower_bound(probe);
				return iterator(it._node, it._nil);
			}

			auto lower_bound(const key_type &key) const -> const_iterator {
				value_type probe(
					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				auto it = _tree.lower_bound(probe);
				return const_iterator(it._node, it._nil);
			}

			auto upper_bound(const key_type &key) -> iterator {
				value_type probe(
					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				auto it = _tree.upper_bound(probe);
				return iterator(it._node, it._nil);
			}

			auto upper_bound(const key_type &key) const -> const_iterator {
				value_type probe(
					std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple());
				auto it = _tree.upper_bound(probe);
				return const_iterator(it._node, it._nil);
			}

			auto equal_range(const key_type &key) -> std::pair<iterator, iterator> {
				return {lower_bound(key), upper_bound(key)};
			}

			auto equal_range(const key_type &key) const
				-> std::pair<const_iterator, const_iterator> {
				return {lower_bound(key), upper_bound(key)};
			}

			// --- Observers ---
			auto key_comp() const -> key_compare {
				return _tree.value_comp()._comp;
			}
			auto value_comp() const -> value_compare {
				return _tree.value_comp();
			}
			auto get_allocator() const noexcept -> allocator_type {
				return _tree.get_allocator();
			}
	};

	template <class t_Key, class t_Value, class t_Comparator, class t_Allocator>
	auto swap(Map<t_Key, t_Value, t_Comparator, t_Allocator> &a,
			  Map<t_Key, t_Value, t_Comparator, t_Allocator> &b) noexcept(noexcept(a.swap(b)))
		-> void {
		a.swap(b);
	}
} // namespace base
