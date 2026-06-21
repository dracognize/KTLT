#pragma once

#include <functional>
#include <memory>
#include <utility>

#include "rbtree.hpp"
#include "types.hpp"

namespace base {
	template <class t_Key,
			  class t_Comparator = std::less<t_Key>,
			  class t_Allocator = std::allocator<t_Key>>
	struct Set {
		private:
			using _base = RedBlackTree<t_Key, t_Comparator, t_Allocator>;
			_base _tree;

		public:
			
			using key_type = t_Key;
			using value_type = t_Key;
			using key_compare = t_Comparator;
			using value_compare = t_Comparator;
			using allocator_type = t_Allocator;
			using size_type = usize;
			using difference_type = isize;
			using reference = value_type &;
			using const_reference = const value_type &;
			using pointer = value_type *;
			using const_pointer = const value_type *;
			using iterator = typename _base::const_iterator;
			using const_iterator = typename _base::const_iterator;
			using reverse_iterator = typename _base::const_reverse_iterator;
			using const_reverse_iterator = typename _base::const_reverse_iterator;

			
			Set() = default;

			explicit Set(const key_compare &comp, const allocator_type &alloc = allocator_type{})
				: _tree(comp, alloc) {
			}

			explicit Set(const allocator_type &alloc) : _tree(alloc) {
			}

			template <std::input_iterator T_InputIt>
			Set(T_InputIt first,
				T_InputIt last,
				const key_compare &comp = key_compare{},
				const allocator_type &alloc = allocator_type{})
				: _tree(first, last, comp, alloc) {
			}

			Set(std::initializer_list<value_type> init,
				const key_compare &comp = key_compare{},
				const allocator_type &alloc = allocator_type{})
				: _tree(init, comp, alloc) {
			}

			Set(const Set &) = default;
			Set(Set &&) = default;

			
			auto operator=(const Set &) -> Set & = default;
			auto operator=(Set &&) -> Set & = default;

			auto operator=(std::initializer_list<value_type> ilist) -> Set & {
				_tree.clear();
				_tree.insert(ilist);
				return *this;
			}

			
			auto begin() const noexcept -> const_iterator {
				return _tree.begin();
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
			auto rbegin() const noexcept -> const_reverse_iterator {
				return _tree.rbegin();
			}
			auto rend() const noexcept -> const_reverse_iterator {
				return _tree.rend();
			}
			auto crbegin() const noexcept -> const_reverse_iterator {
				return _tree.crbegin();
			}
			auto crend() const noexcept -> const_reverse_iterator {
				return _tree.crend();
			}

			
			[[nodiscard]] auto empty() const noexcept -> bool {
				return _tree.empty();
			}
			[[nodiscard]] auto size() const noexcept -> size_type {
				return _tree.size();
			}
			[[nodiscard]] auto max_size() const noexcept -> size_type {
				return _tree.max_size();
			}

			
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

			auto erase(const_iterator pos) -> iterator {
				return _tree.erase(pos);
			}

			auto erase(const_iterator first, const_iterator last) -> iterator {
				return _tree.erase(first, last);
			}

			auto erase(const key_type &val) -> size_type {
				return _tree.erase(val);
			}

			auto swap(Set &other) noexcept(noexcept(_tree.swap(other._tree))) -> void {
				_tree.swap(other._tree);
			}

			auto extract(const_iterator pos) -> value_type {
				auto val = *pos;
				_tree.erase(pos);
				return val;
			}

			
			auto find(const key_type &val) const -> const_iterator {
				return _tree.find(val);
			}
			auto contains(const key_type &val) const -> bool {
				return _tree.contains(val);
			}
			auto count(const key_type &val) const -> size_type {
				return _tree.contains(val) ? 1 : 0;
			}
			auto lower_bound(const key_type &val) const -> const_iterator {
				return _tree.lower_bound(val);
			}
			auto upper_bound(const key_type &val) const -> const_iterator {
				return _tree.upper_bound(val);
			}

			auto equal_range(const key_type &val) const
				-> std::pair<const_iterator, const_iterator> {
				return {lower_bound(val), upper_bound(val)};
			}

			
			auto key_comp() const -> key_compare {
				return _tree.value_comp();
			}
			auto value_comp() const -> value_compare {
				return _tree.value_comp();
			}
			auto get_allocator() const noexcept -> allocator_type {
				return _tree.get_allocator();
			}
	};

	template <class t_Key, class t_Comparator, class t_Allocator>
	auto swap(Set<t_Key, t_Comparator, t_Allocator> &a,
			  Set<t_Key, t_Comparator, t_Allocator> &b) noexcept(noexcept(a.swap(b))) -> void {
		a.swap(b);
	}
} 
