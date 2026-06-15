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
	template <class t_Type, class t_Allocator = std::allocator<t_Type>>
	struct ForwardList {
			struct Node {
					t_Type _value{};
					Node*  _next = nullptr;

					constexpr Node() = default;

					template <class... T_Args>
					constexpr explicit Node(T_Args&&... args) : _value(std::forward<T_Args>(args)...), _next(nullptr) {
					}
			};

			// --- Type aliases ---
			using value_type	  = t_Type;
			using size_type		  = usize;
			using difference_type = isize;
			using allocator_type  = t_Allocator;
			using reference		  = value_type&;
			using const_reference = const value_type&;
			using pointer		  = value_type*;
			using const_pointer	  = const value_type*;

			using node_allocator = std::allocator_traits<t_Allocator>::template rebind_alloc<Node>;
			using alloc_traits	 = std::allocator_traits<node_allocator>;

			// --- Iterator ---
			template <class T_Node, class T_Ref, class T_Ptr>
			struct ForwardIterator {
					using iterator_category = std::forward_iterator_tag;
					using value_type		= t_Type;
					using difference_type	= isize;
					using reference			= T_Ref;
					using pointer			= T_Ptr;

					T_Node* _node = nullptr;

					constexpr ForwardIterator() = default;

					constexpr explicit ForwardIterator(T_Node* node) noexcept : _node(node) {
					}

					constexpr auto operator*() const -> T_Ref {
						return _node->_value;
					}

					constexpr auto operator->() const -> T_Ptr {
						return &_node->_value;
					}

					constexpr auto operator++() -> ForwardIterator& {
						_node = _node->_next;
						return *this;
					}

					constexpr auto operator++(int) -> ForwardIterator {
						auto tmp = *this;
						++(*this);
						return tmp;
					}

					// Implicit conversion: iterator -> const_iterator
					template <class T_UNode, class T_URef, class T_UPtr>
						requires std::convertible_to<T_UNode*, T_Node*>
					constexpr ForwardIterator(const ForwardIterator<T_UNode, T_URef, T_UPtr>& other) noexcept :
						_node(other._node) {
					}

					friend constexpr auto operator==(const ForwardIterator& a, const ForwardIterator& b)
						-> bool = default;
			};

			using iterator		 = ForwardIterator<Node, value_type&, value_type*>;
			using const_iterator = ForwardIterator<const Node, const value_type&, const value_type*>;

		private:
			Node		   _head; // sentinel; _head._next is first element or nullptr
			size_type	   _size = 0;
			node_allocator _alloc;

			constexpr auto _create_node(const value_type& val) -> Node* {
				auto* mem = alloc_traits::allocate(_alloc, 1);
				alloc_traits::construct(_alloc, mem, val);
				mem->_next = nullptr;
				return mem;
			}

			constexpr auto _create_node(value_type&& val) -> Node* {
				auto* mem = alloc_traits::allocate(_alloc, 1);
				alloc_traits::construct(_alloc, mem, std::move(val));
				mem->_next = nullptr;
				return mem;
			}

			template <class... T_Args>
			constexpr auto _emplace_create_node(T_Args&&... args) -> Node* {
				auto* mem = alloc_traits::allocate(_alloc, 1);
				alloc_traits::construct(_alloc, mem, std::forward<T_Args>(args)...);
				mem->_next = nullptr;
				return mem;
			}

			constexpr auto _destroy_node(Node* n) noexcept -> void {
				if (n) {
					alloc_traits::destroy(_alloc, n);
					alloc_traits::deallocate(_alloc, n, 1);
				}
			}

			constexpr auto _destroy_all() noexcept -> void {
				auto* n = _head._next;
				while (n) {
					auto* next = n->_next;
					_destroy_node(n);
					n = next;
				}
				_head._next = nullptr;
				_size		= 0;
			}

		public:
			// --- Construction ---
			constexpr ForwardList() noexcept(noexcept(node_allocator{})) = default;

			constexpr explicit ForwardList(const t_Allocator& alloc) noexcept : _alloc(alloc) {
			}

			constexpr explicit ForwardList(const size_type& count, const value_type& value,
										   const t_Allocator& alloc = t_Allocator{}) : _alloc(alloc) {
				Node* prev = &_head;
				for (size_type i = 0; i < count; ++i) {
					auto* n		= _create_node(value);
					prev->_next = n;
					prev		= n;
					++_size;
				}
			}

			constexpr explicit ForwardList(const size_type& count, const t_Allocator& alloc = t_Allocator{}) :
				_alloc(alloc) {
				Node* prev = &_head;
				for (size_type i = 0; i < count; ++i) {
					auto* n		= _create_node(value_type{});
					prev->_next = n;
					prev		= n;
					++_size;
				}
			}

			template <std::input_iterator T_InputIt>
			constexpr ForwardList(T_InputIt first, T_InputIt last, const t_Allocator& alloc = t_Allocator{}) :
				_alloc(alloc) {
				Node* prev = &_head;
				for (; first != last; ++first) {
					auto* n		= _create_node(*first);
					prev->_next = n;
					prev		= n;
					++_size;
				}
			}

			constexpr ForwardList(std::initializer_list<value_type> init, const t_Allocator& alloc = t_Allocator{}) :
				_alloc(alloc) {
				Node* prev = &_head;
				for (auto& val : init) {
					auto* n		= _create_node(val);
					prev->_next = n;
					prev		= n;
					++_size;
				}
			}

			constexpr ForwardList(const ForwardList& other) :
				_alloc(alloc_traits::select_on_container_copy_construction(other._alloc)) {
				Node* prev = &_head;
				for (auto* n = other._head._next; n; n = n->_next) {
					auto* copy	= _create_node(n->_value);
					prev->_next = copy;
					prev		= copy;
					++_size;
				}
			}

			constexpr ForwardList(ForwardList&& other) noexcept :
				_size(std::exchange(other._size, 0)), _alloc(std::move(other._alloc)) {
				_head._next		  = other._head._next;
				other._head._next = nullptr;
			}

			// --- Destructor ---
			constexpr ~ForwardList() noexcept {
				_destroy_all();
			}

			// --- Assignment ---
			constexpr auto operator=(const ForwardList& other) -> ForwardList& {
				if (this != &other) {
					clear();
					if constexpr (alloc_traits::propagate_on_container_copy_assignment::value)
						_alloc = other._alloc;
					Node* prev = &_head;
					for (auto* n = other._head._next; n; n = n->_next) {
						auto* copy	= _create_node(n->_value);
						prev->_next = copy;
						prev		= copy;
						++_size;
					}
				}
				return *this;
			}

			constexpr auto operator=(ForwardList&& other) noexcept -> ForwardList& {
				if (this != &other) {
					_destroy_all();
					_head._next		  = other._head._next;
					_size			  = std::exchange(other._size, 0);
					_alloc			  = std::move(other._alloc);
					other._head._next = nullptr;
				}
				return *this;
			}

			constexpr auto operator=(std::initializer_list<value_type> ilist) -> ForwardList& {
				clear();
				Node* prev = &_head;
				for (auto& val : ilist) {
					auto* n		= _create_node(val);
					prev->_next = n;
					prev		= n;
					++_size;
				}
				return *this;
			}

			// --- Iterators ---
			constexpr auto before_begin() noexcept -> iterator {
				return iterator(&_head);
			}

			constexpr auto before_begin() const noexcept -> const_iterator {
				return const_iterator(const_cast<const Node*>(&_head));
			}

			constexpr auto cbefore_begin() const noexcept -> const_iterator {
				return const_iterator(const_cast<const Node*>(&_head));
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
				return iterator(nullptr);
			}

			constexpr auto end() const noexcept -> const_iterator {
				return const_iterator(nullptr);
			}

			constexpr auto cend() const noexcept -> const_iterator {
				return const_iterator(nullptr);
			}

			// --- Capacity ---
			[[nodiscard]] constexpr auto empty() const noexcept -> bool {
				return _head._next == nullptr;
			}

			[[nodiscard]] constexpr auto size() const noexcept -> size_type {
				return _size;
			}

			[[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
				return alloc_traits::max_size(_alloc);
			}

			// --- Element access ---
			constexpr auto front(this auto&& self) noexcept -> decltype(auto) {
				return self._head._next->_value;
			}

			// --- Modifiers ---
			constexpr auto clear() noexcept -> void {
				_destroy_all();
			}

			constexpr auto push_front(const value_type& val) -> void {
				auto* n		= _create_node(val);
				n->_next	= _head._next;
				_head._next = n;
				++_size;
			}

			constexpr auto push_front(value_type&& val) -> void {
				auto* n		= _create_node(std::move(val));
				n->_next	= _head._next;
				_head._next = n;
				++_size;
			}

			constexpr auto pop_front() -> void {
				auto* n = _head._next;
				if (n) {
					_head._next = n->_next;
					_destroy_node(n);
					--_size;
				}
			}

			template <class... T_Args>
			constexpr auto emplace_front(T_Args&&... args) -> reference {
				auto* n		= _emplace_create_node(std::forward<T_Args>(args)...);
				n->_next	= _head._next;
				_head._next = n;
				++_size;
				return n->_value;
			}

			constexpr auto insert_after(const_iterator pos, const value_type& val) -> iterator {
				auto* node	= const_cast<Node*>(pos._node);
				auto* n		= _create_node(val);
				n->_next	= node->_next;
				node->_next = n;
				++_size;
				return iterator(n);
			}

			constexpr auto insert_after(const_iterator pos, value_type&& val) -> iterator {
				auto* node	= const_cast<Node*>(pos._node);
				auto* n		= _create_node(std::move(val));
				n->_next	= node->_next;
				node->_next = n;
				++_size;
				return iterator(n);
			}

			template <std::input_iterator T_InputIt>
			constexpr auto insert_after(const_iterator pos, T_InputIt first, T_InputIt last) -> iterator {
				auto* node = const_cast<Node*>(pos._node);
				for (; first != last; ++first) {
					auto* n		= _create_node(*first);
					n->_next	= node->_next;
					node->_next = n;
					node		= n;
					++_size;
				}
				return iterator(node);
			}

			constexpr auto insert_after(const_iterator pos, std::initializer_list<value_type> ilist) -> iterator {
				return insert_after(pos, ilist.begin(), ilist.end());
			}

			template <class... T_Args>
			constexpr auto emplace_after(const_iterator pos, T_Args&&... args) -> iterator {
				auto* node	= const_cast<Node*>(pos._node);
				auto* n		= _emplace_create_node(std::forward<T_Args>(args)...);
				n->_next	= node->_next;
				node->_next = n;
				++_size;
				return iterator(n);
			}

			constexpr auto erase_after(const_iterator pos) -> iterator {
				auto* node	 = const_cast<Node*>(pos._node);
				auto* target = node->_next;
				if (target) {
					node->_next = target->_next;
					_destroy_node(target);
					--_size;
				}
				return iterator(node->_next);
			}

			constexpr auto erase_after(const_iterator first, const_iterator last) -> iterator {
				auto* node	   = const_cast<Node*>(first._node);
				auto* end_node = const_cast<Node*>(last._node);
				while (node->_next != end_node) {
					auto* target = node->_next;
					node->_next	 = target->_next;
					_destroy_node(target);
					--_size;
				}
				return iterator(end_node);
			}

			constexpr auto resize(size_type count) -> void {
				if (count < _size) {
					auto* prev = &_head;
					for (size_type i = 0; i < count; ++i)
						prev = prev->_next;
					auto* n		= prev->_next;
					prev->_next = nullptr;
					while (n) {
						auto* next = n->_next;
						_destroy_node(n);
						n = next;
					}
					_size = count;
				}
				else if (count > _size) {
					auto* prev = &_head;
					while (prev->_next)
						prev = prev->_next;
					for (size_type i = _size; i < count; ++i) {
						auto* n		= _create_node(value_type{});
						prev->_next = n;
						prev		= n;
					}
					_size = count;
				}
			}

			constexpr auto resize(size_type count, const value_type& value) -> void {
				if (count < _size) {
					auto* prev = &_head;
					for (size_type i = 0; i < count; ++i)
						prev = prev->_next;
					auto* n		= prev->_next;
					prev->_next = nullptr;
					while (n) {
						auto* next = n->_next;
						_destroy_node(n);
						n = next;
					}
					_size = count;
				}
				else if (count > _size) {
					auto* prev = &_head;
					while (prev->_next)
						prev = prev->_next;
					for (size_type i = _size; i < count; ++i) {
						auto* n		= _create_node(value);
						prev->_next = n;
						prev		= n;
					}
					_size = count;
				}
			}

			constexpr auto swap(ForwardList& other) noexcept(std::is_nothrow_swappable_v<node_allocator>) -> void {
				base::swap(_head._next, other._head._next);
				base::swap(_size, other._size);
				base::swap(_alloc, other._alloc);
			}

			// --- Operations ---
			constexpr auto reverse() noexcept -> void {
				Node* prev = nullptr;
				Node* curr = _head._next;
				while (curr) {
					auto* next	= curr->_next;
					curr->_next = prev;
					prev		= curr;
					curr		= next;
				}
				_head._next = prev;
			}

			constexpr auto remove(const value_type& val) -> size_type {
				return remove_if([&](const value_type& v) { return v == val; });
			}

			template <class T_UnaryPred>
			constexpr auto remove_if(T_UnaryPred pred) -> size_type {
				size_type count = 0;
				Node*	  prev	= &_head;
				while (prev->_next) {
					if (pred(prev->_next->_value)) {
						auto* target = prev->_next;
						prev->_next	 = target->_next;
						_destroy_node(target);
						--_size;
						++count;
					}
					else {
						prev = prev->_next;
					}
				}
				return count;
			}

			constexpr auto unique() -> size_type {
				return unique([](const value_type& a, const value_type& b) { return a == b; });
			}

			template <class T_BinaryPred>
			constexpr auto unique(T_BinaryPred pred) -> size_type {
				size_type count = 0;
				Node*	  curr	= _head._next;
				while (curr && curr->_next) {
					if (pred(curr->_value, curr->_next->_value)) {
						auto* target = curr->_next;
						curr->_next	 = target->_next;
						_destroy_node(target);
						--_size;
						++count;
					}
					else {
						curr = curr->_next;
					}
				}
				return count;
			}

			constexpr auto merge(ForwardList& other) -> void {
				merge(other, [](const value_type& a, const value_type& b) { return a < b; });
			}

			template <class T_Compare>
			constexpr auto merge(ForwardList& other, T_Compare comp) -> void {
				Node*  a	= _head._next;
				Node** tail = &_head._next;
				Node*  b	= other._head._next;

				while (a && b) {
					if (comp(a->_value, b->_value)) {
						*tail = a;
						tail  = &a->_next;
						a	  = a->_next;
					}
					else {
						*tail = b;
						tail  = &b->_next;
						b	  = b->_next;
					}
				}
				*tail = a ? a : b;
				_size += other._size;
				other._head._next = nullptr;
				other._size		  = 0;
			}

			constexpr auto merge(ForwardList&& other) -> void {
				merge(other);
			}

			template <class T_Compare>
			constexpr auto merge(ForwardList&& other, T_Compare comp) -> void {
				merge(other, std::move(comp));
			}
	};

	template <class t_Type, class t_Allocator>
	constexpr auto swap(ForwardList<t_Type, t_Allocator>& a,
						ForwardList<t_Type, t_Allocator>& b) noexcept(noexcept(a.swap(b))) -> void {
		a.swap(b);
	}
} // namespace base
