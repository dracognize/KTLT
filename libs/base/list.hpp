#pragma once

#include <concepts>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "algorithm.hpp"
#include "types.hpp"

namespace base {
	template <class t_Type, class t_Allocator = std::allocator<t_Type>> struct List {
			struct Node {
					t_Type _value{};
					Node *_prev = nullptr;
					Node *_next = nullptr;

					constexpr Node() = default;

					template <class... T_Args>
					constexpr explicit Node(T_Args &&...args)
						: _value(std::forward<T_Args>(args)...), _prev(nullptr), _next(nullptr) {
					}
			};

			
			using value_type = t_Type;
			using size_type = usize;
			using difference_type = isize;
			using allocator_type = t_Allocator;
			using reference = value_type &;
			using const_reference = const value_type &;
			using pointer = value_type *;
			using const_pointer = const value_type *;

			using node_allocator = std::allocator_traits<t_Allocator>::template rebind_alloc<Node>;
			using alloc_traits = std::allocator_traits<node_allocator>;

			
			template <class T_Node, class T_Ref, class T_Ptr> struct ListIterator {
					using iterator_category = std::bidirectional_iterator_tag;
					using value_type = t_Type;
					using difference_type = isize;
					using reference = T_Ref;
					using pointer = T_Ptr;

					T_Node *_node = nullptr;

					constexpr ListIterator() = default;

					constexpr explicit ListIterator(T_Node *node) noexcept : _node(node) {
					}

					constexpr auto operator*() const -> T_Ref {
						return _node->_value;
					}

					constexpr auto operator->() const -> T_Ptr {
						return &_node->_value;
					}

					constexpr auto operator++() -> ListIterator & {
						_node = _node->_next;
						return *this;
					}

					constexpr auto operator++(int) -> ListIterator {
						auto tmp = *this;
						++(*this);
						return tmp;
					}

					constexpr auto operator--() -> ListIterator & {
						_node = _node->_prev;
						return *this;
					}

					constexpr auto operator--(int) -> ListIterator {
						auto tmp = *this;
						--(*this);
						return tmp;
					}

					template <class T_UNode, class T_URef, class T_UPtr>
						requires std::convertible_to<T_UNode *, T_Node *>
					constexpr ListIterator(
						const ListIterator<T_UNode, T_URef, T_UPtr> &other) noexcept
						: _node(other._node) {
					}

					friend constexpr auto operator==(const ListIterator &a, const ListIterator &b)
						-> bool = default;
			};

			using iterator = ListIterator<Node, value_type &, value_type *>;
			using const_iterator = ListIterator<const Node, const value_type &, const value_type *>;
			using reverse_iterator = std::reverse_iterator<iterator>;
			using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		private:
			Node _head;
			size_type _size = 0;
			node_allocator _alloc;

			constexpr auto _create_node(const value_type &val) -> Node * {
				auto *mem = alloc_traits::allocate(_alloc, 1);
				alloc_traits::construct(_alloc, mem, val);
				mem->_prev = nullptr;
				mem->_next = nullptr;
				return mem;
			}

			constexpr auto _create_node(value_type &&val) -> Node * {
				auto *mem = alloc_traits::allocate(_alloc, 1);
				alloc_traits::construct(_alloc, mem, std::move(val));
				mem->_prev = nullptr;
				mem->_next = nullptr;
				return mem;
			}

			template <class... T_Args>
			constexpr auto _emplace_create_node(T_Args &&...args) -> Node * {
				auto *mem = alloc_traits::allocate(_alloc, 1);
				alloc_traits::construct(_alloc, mem, std::forward<T_Args>(args)...);
				mem->_prev = nullptr;
				mem->_next = nullptr;
				return mem;
			}

			constexpr auto _destroy_node(Node *n) noexcept -> void {
				if (n && n != &_head) {
					alloc_traits::destroy(_alloc, n);
					alloc_traits::deallocate(_alloc, n, 1);
				}
			}

			constexpr auto _destroy_all() noexcept -> void {
				auto *n = _head._next;
				while (n != &_head) {
					auto *next = n->_next;
					_destroy_node(n);
					n = next;
				}
				_head._prev = &_head;
				_head._next = &_head;
				_size = 0;
			}

			constexpr auto _link_before(Node *pos, Node *n) noexcept -> void {
				n->_prev = pos->_prev;
				n->_next = pos;
				pos->_prev->_next = n;
				pos->_prev = n;
			}

			constexpr auto _link_block_before(Node *pos, Node *first, Node *last) noexcept -> void {
				first->_prev = pos->_prev;
				last->_next = pos;
				pos->_prev->_next = first;
				pos->_prev = last;
			}

			constexpr auto _unlink(Node *n) noexcept -> void {
				n->_prev->_next = n->_next;
				n->_next->_prev = n->_prev;
			}

			

			constexpr auto _split(Node *head, Node **a, Node **b) noexcept -> void {
				Node *slow = head;
				Node *fast = head->_next;
				while (fast != &_head && fast->_next != &_head) {
					slow = slow->_next;
					fast = fast->_next->_next;
				}
				*b = slow->_next;
				slow->_next = &_head;
				(*b)->_prev = &_head;
				*a = head;
			}

			template <class T_Compare>
			constexpr auto _sorted_merge(Node *a, Node *b, T_Compare comp) noexcept -> Node * {
				Node result;
				result._prev = const_cast<Node *>(&_head);
				result._next = const_cast<Node *>(&_head);
				Node *tail = &result;

				while (a != &_head && b != &_head) {
					if (comp(a->_value, b->_value)) {
						tail->_next = a;
						a->_prev = tail;
						tail = a;
						a = a->_next;
					} else {
						tail->_next = b;
						b->_prev = tail;
						tail = b;
						b = b->_next;
					}
				}
				auto *remainder = (a != &_head) ? a : b;
				tail->_next = remainder;
				remainder->_prev = tail;

				while (tail->_next != &_head) {
					tail = tail->_next;
				}
				tail->_next = const_cast<Node *>(&_head);
				_head._prev = tail;
				return result._next;
			}

			constexpr auto _merge_sort(Node *head) noexcept -> Node * {
				if (head == &_head || head->_next == &_head)
					return head;
				Node *a, *b;
				_split(head, &a, &b);
				a = _merge_sort(a);
				b = _merge_sort(b);
				return _sorted_merge(a, b, std::less<value_type>{});
			}

			template <class T_Compare>
			constexpr auto _merge_sort(Node *head, T_Compare comp) noexcept -> Node * {
				if (head == &_head || head->_next == &_head)
					return head;
				Node *a, *b;
				_split(head, &a, &b);
				a = _merge_sort(a, comp);
				b = _merge_sort(b, comp);
				return _sorted_merge(a, b, comp);
			}

		public:
			
			constexpr List() noexcept(noexcept(node_allocator{})) {
				_head._prev = &_head;
				_head._next = &_head;
			}

			constexpr explicit List(const t_Allocator &alloc) noexcept : _alloc(alloc) {
				_head._prev = &_head;
				_head._next = &_head;
			}

			constexpr explicit List(const size_type &count,
									const value_type &value,
									const t_Allocator &alloc = t_Allocator{})
				: _alloc(alloc) {
				_head._prev = &_head;
				_head._next = &_head;
				Node *tail = &_head;
				for (size_type i = 0; i < count; ++i) {
					auto *n = _create_node(value);
					tail->_next = n;
					n->_prev = tail;
					tail = n;
					++_size;
				}
				tail->_next = &_head;
				_head._prev = tail;
			}

			constexpr explicit List(const size_type &count,
									const t_Allocator &alloc = t_Allocator{})
				: _alloc(alloc) {
				_head._prev = &_head;
				_head._next = &_head;
				Node *tail = &_head;
				for (size_type i = 0; i < count; ++i) {
					auto *n = _create_node(value_type{});
					tail->_next = n;
					n->_prev = tail;
					tail = n;
					++_size;
				}
				tail->_next = &_head;
				_head._prev = tail;
			}

			template <std::input_iterator T_InputIt>
			constexpr List(T_InputIt first,
						   T_InputIt last,
						   const t_Allocator &alloc = t_Allocator{})
				: _alloc(alloc) {
				_head._prev = &_head;
				_head._next = &_head;
				Node *tail = &_head;
				for (; first != last; ++first) {
					auto *n = _create_node(*first);
					tail->_next = n;
					n->_prev = tail;
					tail = n;
					++_size;
				}
				tail->_next = &_head;
				_head._prev = tail;
			}

			constexpr List(std::initializer_list<value_type> init,
						   const t_Allocator &alloc = t_Allocator{})
				: _alloc(alloc) {
				_head._prev = &_head;
				_head._next = &_head;
				Node *tail = &_head;
				for (auto &val : init) {
					auto *n = _create_node(val);
					tail->_next = n;
					n->_prev = tail;
					tail = n;
					++_size;
				}
				tail->_next = &_head;
				_head._prev = tail;
			}

			constexpr List(const List &other)
				: _alloc(alloc_traits::select_on_container_copy_construction(other._alloc)) {
				_head._prev = &_head;
				_head._next = &_head;
				Node *tail = &_head;
				for (auto *n = other._head._next; n != &other._head; n = n->_next) {
					auto *copy = _create_node(n->_value);
					tail->_next = copy;
					copy->_prev = tail;
					tail = copy;
					++_size;
				}
				tail->_next = &_head;
				_head._prev = tail;
			}

			constexpr List(List &&other) noexcept
				: _size(std::exchange(other._size, 0)), _alloc(std::move(other._alloc)) {
				if (other._head._next != &other._head) {
					_head._next = other._head._next;
					_head._prev = other._head._prev;
					_head._next->_prev = &_head;
					_head._prev->_next = &_head;
					other._head._prev = &other._head;
					other._head._next = &other._head;
				} else {
					_head._prev = &_head;
					_head._next = &_head;
				}
			}

			
			constexpr ~List() noexcept {
				_destroy_all();
			}

			
			constexpr auto operator=(const List &other) -> List & {
				if (this != &other) {
					_destroy_all();
					if constexpr (alloc_traits::propagate_on_container_copy_assignment::value)
						_alloc = other._alloc;
					Node *tail = &_head;
					for (auto *n = other._head._next; n != &other._head; n = n->_next) {
						auto *copy = _create_node(n->_value);
						tail->_next = copy;
						copy->_prev = tail;
						tail = copy;
						++_size;
					}
					tail->_next = &_head;
					_head._prev = tail;
				}
				return *this;
			}

			constexpr auto operator=(List &&other) noexcept -> List & {
				if (this != &other) {
					_destroy_all();
					if (other._head._next != &other._head) {
						_head._next = other._head._next;
						_head._prev = other._head._prev;
						_head._next->_prev = &_head;
						_head._prev->_next = &_head;
						other._head._prev = &other._head;
						other._head._next = &other._head;
					} else {
						_head._prev = &_head;
						_head._next = &_head;
					}
					_size = std::exchange(other._size, 0);
					_alloc = std::move(other._alloc);
				}
				return *this;
			}

			constexpr auto operator=(std::initializer_list<value_type> ilist) -> List & {
				assign(ilist);
				return *this;
			}

			
			constexpr auto assign(const size_type &count, const value_type &value) -> void {
				clear();
				if (count == 0)
					return;
				Node *tail = &_head;
				for (size_type i = 0; i < count; ++i) {
					auto *n = _create_node(value);
					tail->_next = n;
					n->_prev = tail;
					tail = n;
				}
				tail->_next = &_head;
				_head._prev = tail;
				_size = count;
			}

			template <std::input_iterator T_InputIt>
			constexpr auto assign(T_InputIt first, T_InputIt last) -> void {
				clear();
				if (first == last)
					return;
				Node *tail = &_head;
				for (; first != last; ++first) {
					auto *n = _create_node(*first);
					tail->_next = n;
					n->_prev = tail;
					tail = n;
					++_size;
				}
				tail->_next = &_head;
				_head._prev = tail;
			}

			constexpr auto assign(std::initializer_list<value_type> ilist) -> void {
				assign(ilist.begin(), ilist.end());
			}

			
			constexpr auto begin() noexcept -> iterator {
				return iterator(_head._next);
			}

			constexpr auto begin() const noexcept -> const_iterator {
				return const_iterator(_head._next);
			}

			constexpr auto cbegin() const noexcept -> const_iterator {
				return const_iterator(_head._next);
			}

			constexpr auto end() noexcept -> iterator {
				return iterator(const_cast<Node *>(&_head));
			}

			constexpr auto end() const noexcept -> const_iterator {
				return const_iterator(&_head);
			}

			constexpr auto cend() const noexcept -> const_iterator {
				return const_iterator(&_head);
			}

			constexpr auto rbegin() noexcept -> reverse_iterator {
				return reverse_iterator(end());
			}

			constexpr auto rbegin() const noexcept -> const_reverse_iterator {
				return const_reverse_iterator(end());
			}

			constexpr auto crbegin() const noexcept -> const_reverse_iterator {
				return const_reverse_iterator(cend());
			}

			constexpr auto rend() noexcept -> reverse_iterator {
				return reverse_iterator(begin());
			}

			constexpr auto rend() const noexcept -> const_reverse_iterator {
				return const_reverse_iterator(begin());
			}

			constexpr auto crend() const noexcept -> const_reverse_iterator {
				return const_reverse_iterator(cbegin());
			}

			
			[[nodiscard]] constexpr auto empty() const noexcept -> bool {
				return _head._next == &_head;
			}

			[[nodiscard]] constexpr auto size() const noexcept -> size_type {
				return _size;
			}

			[[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
				return alloc_traits::max_size(_alloc);
			}

			
			constexpr auto front(this auto &&self) noexcept -> decltype(auto) {
				return self._head._next->_value;
			}

			constexpr auto back(this auto &&self) noexcept -> decltype(auto) {
				return self._head._prev->_value;
			}

			
			constexpr auto clear() noexcept -> void {
				_destroy_all();
			}

			
			constexpr auto insert(const_iterator pos, const value_type &val) -> iterator {
				auto *n = _create_node(val);
				_link_before(const_cast<Node *>(pos._node), n);
				++_size;
				return iterator(n);
			}

			constexpr auto insert(const_iterator pos, value_type &&val) -> iterator {
				auto *n = _create_node(std::move(val));
				_link_before(const_cast<Node *>(pos._node), n);
				++_size;
				return iterator(n);
			}

			constexpr auto insert(const_iterator pos,
								  const size_type &count,
								  const value_type &value) -> iterator {
				if (count == 0)
					return iterator(const_cast<Node *>(pos._node));
				auto *first_node = _create_node(value);
				Node *tail = first_node;
				for (size_type i = 1; i < count; ++i) {
					auto *n = _create_node(value);
					tail->_next = n;
					n->_prev = tail;
					tail = n;
				}
				_link_block_before(const_cast<Node *>(pos._node), first_node, tail);
				_size += count;
				return iterator(first_node);
			}

			template <std::input_iterator T_InputIt>
			constexpr auto insert(const_iterator pos, T_InputIt first, T_InputIt last) -> iterator {
				if (first == last)
					return iterator(const_cast<Node *>(pos._node));

				size_type count = 1;
				auto *first_node = _create_node(*first);
				++first;
				Node *prev_node = first_node;
				for (; first != last; ++first) {
					auto *n = _create_node(*first);
					prev_node->_next = n;
					n->_prev = prev_node;
					prev_node = n;
					++count;
				}
				_link_block_before(const_cast<Node *>(pos._node), first_node, prev_node);
				_size += count;
				return iterator(first_node);
			}

			constexpr auto insert(const_iterator pos, std::initializer_list<value_type> ilist)
				-> iterator {
				return insert(pos, ilist.begin(), ilist.end());
			}

			
			template <class... T_Args>
			constexpr auto emplace(const_iterator pos, T_Args &&...args) -> iterator {
				auto *n = _emplace_create_node(std::forward<T_Args>(args)...);
				_link_before(const_cast<Node *>(pos._node), n);
				++_size;
				return iterator(n);
			}

			template <class... T_Args> constexpr auto emplace_back(T_Args &&...args) -> reference {
				auto *n = _emplace_create_node(std::forward<T_Args>(args)...);
				_link_before(&_head, n);
				++_size;
				return n->_value;
			}

			template <class... T_Args> constexpr auto emplace_front(T_Args &&...args) -> reference {
				auto *n = _emplace_create_node(std::forward<T_Args>(args)...);
				_link_before(_head._next, n);
				++_size;
				return n->_value;
			}

			
			constexpr auto push_front(const value_type &val) -> void {
				insert(cbegin(), val);
			}

			constexpr auto push_front(value_type &&val) -> void {
				insert(cbegin(), std::move(val));
			}

			constexpr auto push_back(const value_type &val) -> void {
				insert(cend(), val);
			}

			constexpr auto push_back(value_type &&val) -> void {
				insert(cend(), std::move(val));
			}

			constexpr auto pop_front() -> void {
				erase(cbegin());
			}

			constexpr auto pop_back() -> void {
				erase(std::prev(cend()));
			}

			
			constexpr auto erase(const_iterator pos) -> iterator {
				auto *node = const_cast<Node *>(pos._node);
				auto *next = node->_next;
				_unlink(node);
				_destroy_node(node);
				--_size;
				return iterator(next);
			}

			constexpr auto erase(const_iterator first, const_iterator last) -> iterator {
				if (first == last)
					return iterator(const_cast<Node *>(last._node));

				auto *first_node = const_cast<Node *>(first._node);
				auto *last_node = const_cast<Node *>(last._node);

				first_node->_prev->_next = last_node;
				last_node->_prev = first_node->_prev;

				size_type count = 0;
				auto *n = first_node;
				while (n != last_node) {
					auto *next = n->_next;
					_destroy_node(n);
					n = next;
					++count;
				}
				_size -= count;
				return iterator(last_node);
			}

			
			constexpr auto resize(size_type count) -> void {
				if (count < _size) {
					auto *n = _head._next;
					for (size_type i = 0; i < count; ++i)
						n = n->_next;
					auto *last_kept = n->_prev;
					last_kept->_next = &_head;
					_head._prev = last_kept;
					auto *curr = n;
					while (curr != &_head) {
						auto *next = curr->_next;
						_destroy_node(curr);
						curr = next;
					}
					_size = count;
				} else if (count > _size) {
					Node *tail = _head._prev;
					for (size_type i = _size; i < count; ++i) {
						auto *n = _create_node(value_type{});
						tail->_next = n;
						n->_prev = tail;
						tail = n;
					}
					tail->_next = &_head;
					_head._prev = tail;
					_size = count;
				}
			}

			constexpr auto resize(size_type count, const value_type &value) -> void {
				if (count < _size) {
					auto *n = _head._next;
					for (size_type i = 0; i < count; ++i)
						n = n->_next;
					auto *last_kept = n->_prev;
					last_kept->_next = &_head;
					_head._prev = last_kept;
					auto *curr = n;
					while (curr != &_head) {
						auto *next = curr->_next;
						_destroy_node(curr);
						curr = next;
					}
					_size = count;
				} else if (count > _size) {
					Node *tail = _head._prev;
					for (size_type i = _size; i < count; ++i) {
						auto *n = _create_node(value);
						tail->_next = n;
						n->_prev = tail;
						tail = n;
					}
					tail->_next = &_head;
					_head._prev = tail;
					_size = count;
				}
			}

			
			constexpr auto swap(List &other) noexcept(std::is_nothrow_swappable_v<node_allocator>)
				-> void {
				if (this == &other)
					return;
				base::swap(_size, other._size);
				base::swap(_alloc, other._alloc);

				bool this_empty = _head._next == &_head;
				bool other_empty = other._head._next == &other._head;

				if (this_empty && other_empty) {
				} else if (this_empty) {
					_head._next = other._head._next;
					_head._prev = other._head._prev;
					_head._next->_prev = &_head;
					_head._prev->_next = &_head;
					other._head._prev = &other._head;
					other._head._next = &other._head;
				} else if (other_empty) {
					other._head._next = _head._next;
					other._head._prev = _head._prev;
					other._head._next->_prev = &other._head;
					other._head._prev->_next = &other._head;
					_head._prev = &_head;
					_head._next = &_head;
				} else {
					base::swap(_head._next, other._head._next);
					base::swap(_head._prev, other._head._prev);
					_head._next->_prev = &_head;
					_head._prev->_next = &_head;
					other._head._next->_prev = &other._head;
					other._head._prev->_next = &other._head;
				}
			}

			
			constexpr auto reverse() noexcept -> void {
				if (_size <= 1)
					return;
				auto *curr = _head._next;
				while (curr != &_head) {
					base::swap(curr->_prev, curr->_next);
					curr = curr->_prev;
				}
				base::swap(_head._prev, _head._next);
			}

			
			constexpr auto remove(const value_type &val) -> size_type {
				return remove_if([&](const value_type &v) { return v == val; });
			}

			template <class T_UnaryPred> constexpr auto remove_if(T_UnaryPred pred) -> size_type {
				size_type count = 0;
				auto *curr = _head._next;
				while (curr != &_head) {
					auto *next = curr->_next;
					if (pred(curr->_value)) {
						_unlink(curr);
						_destroy_node(curr);
						--_size;
						++count;
					}
					curr = next;
				}
				return count;
			}

			
			constexpr auto unique() -> size_type {
				return unique([](const value_type &a, const value_type &b) { return a == b; });
			}

			template <class T_BinaryPred> constexpr auto unique(T_BinaryPred pred) -> size_type {
				size_type count = 0;
				auto *curr = _head._next;
				while (curr != &_head && curr->_next != &_head) {
					if (pred(curr->_value, curr->_next->_value)) {
						auto *target = curr->_next;
						_unlink(target);
						_destroy_node(target);
						--_size;
						++count;
					} else {
						curr = curr->_next;
					}
				}
				return count;
			}

			
			constexpr auto merge(List &other) -> void {
				merge(other, [](const value_type &a, const value_type &b) { return a < b; });
			}

			template <class T_Compare> constexpr auto merge(List &other, T_Compare comp) -> void {
				if (this == &other || other.empty())
					return;

				Node *a = _head._next;
				Node *b = other._head._next;
				Node *tail = &_head;

				while (a != &_head && b != &other._head) {
					if (comp(a->_value, b->_value)) {
						tail->_next = a;
						a->_prev = tail;
						tail = a;
						a = a->_next;
					} else {
						tail->_next = b;
						b->_prev = tail;
						tail = b;
						b = b->_next;
					}
				}
				auto *remainder = (a != &_head) ? a : b;
				tail->_next = remainder;
				remainder->_prev = tail;

				_rebuild_tail();
				_size += other._size;
				other._head._prev = &other._head;
				other._head._next = &other._head;
				other._size = 0;
			}

			constexpr auto merge(List &&other) -> void {
				merge(other);
			}

			template <class T_Compare> constexpr auto merge(List &&other, T_Compare comp) -> void {
				merge(other, std::move(comp));
			}

			
			constexpr auto sort() -> void {
				if (_size <= 1)
					return;
				auto *new_head = _merge_sort(_head._next);
				_head._next = new_head;
				new_head->_prev = const_cast<Node *>(&_head);
				_rebuild_tail();
			}

			template <class T_Compare> constexpr auto sort(T_Compare comp) -> void {
				if (_size <= 1)
					return;
				auto *new_head = _merge_sort(_head._next, comp);
				_head._next = new_head;
				new_head->_prev = const_cast<Node *>(&_head);
				_rebuild_tail();
			}

			
			constexpr auto splice(const_iterator pos, List &other) -> void {
				if (other.empty())
					return;
				auto *before = const_cast<Node *>(pos._node);
				auto *first = other._head._next;
				auto *last = other._head._prev;

				other._head._next = &other._head;
				other._head._prev = &other._head;

				_link_block_before(before, first, last);
				_size += other._size;
				other._size = 0;
			}

			constexpr auto splice(const_iterator pos, List &&other) -> void {
				splice(pos, other);
			}

			constexpr auto splice(const_iterator pos, List &other, const_iterator it) -> void {
				auto *node = const_cast<Node *>(it._node);
				if (node == &other._head)
					return;
				other._unlink(node);
				--other._size;
				_link_before(const_cast<Node *>(pos._node), node);
				++_size;
			}

			constexpr auto splice(const_iterator pos, List &&other, const_iterator it) -> void {
				splice(pos, other, it);
			}

			constexpr auto
			splice(const_iterator pos, List &other, const_iterator first, const_iterator last)
				-> void {
				auto *first_node = const_cast<Node *>(first._node);
				auto *last_node = const_cast<Node *>(last._node);
				if (first_node == last_node)
					return;

				size_type count = 0;
				auto *n = first_node;
				while (n != last_node) {
					n = n->_next;
					++count;
				}
				auto *block_last = n->_prev;

				first_node->_prev->_next = last_node;
				last_node->_prev = first_node->_prev;

				_link_block_before(const_cast<Node *>(pos._node), first_node, block_last);
				_size += count;
				other._size -= count;
			}

			constexpr auto
			splice(const_iterator pos, List &&other, const_iterator first, const_iterator last)
				-> void {
				splice(pos, other, first, last);
			}

			
			constexpr auto get_allocator() const noexcept -> allocator_type {
				return _alloc;
			}

		private:
			constexpr auto _rebuild_tail() noexcept -> void {
				auto *tail = _head._next;
				while (tail->_next != &_head)
					tail = tail->_next;
				tail->_next = const_cast<Node *>(&_head);
				_head._prev = tail;
			}
	};

	template <class t_Type, class t_Allocator>
	constexpr auto swap(List<t_Type, t_Allocator> &a,
						List<t_Type, t_Allocator> &b) noexcept(noexcept(a.swap(b))) -> void {
		a.swap(b);
	}
} 
