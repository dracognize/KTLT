#pragma once

#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>

#include "algorithm.hpp"
#include "types.hpp"

namespace base {
	template <class t_ValueTypes, class t_ComparatorTypes = std::less<t_ValueTypes>,
			  class t_Alloc = std::allocator<t_ValueTypes>>
		requires std::default_initializable<t_ValueTypes> && std::copy_constructible<t_ValueTypes>
	struct RedBlackTree {
			struct Node {
					t_ValueTypes _value{};
					Node*		 _parent  = nullptr;
					Node*		 _left	  = nullptr;
					Node*		 _right	  = nullptr;
					bool		 _isBlack = false;

					constexpr Node() = default;

					constexpr explicit Node(const t_ValueTypes& val) :
						_value(val), _parent(nullptr), _left(nullptr), _right(nullptr), _isBlack(false) {};
			};

			// --- Type aliases ---
			using value_type	  = t_ValueTypes;
			using comparator_type = t_ComparatorTypes;
			using allocator_type  = t_Alloc;
			using size_type		  = usize;
			using difference_type = isize;
			using reference		  = value_type&;
			using const_reference = const value_type&;
			using pointer		  = value_type*;
			using const_pointer	  = const value_type*;

			using node_allocator = std::allocator_traits<t_Alloc>::template rebind_alloc<Node>;
			using alloc_traits	 = std::allocator_traits<node_allocator>;

			// --- Iterator ---
			template <class T_Node, class T_Ref, class T_Ptr>
			struct TreeIterator {
					using iterator_category = std::bidirectional_iterator_tag;
					using value_type		= t_ValueTypes;
					using difference_type	= isize;
					using reference			= T_Ref;
					using pointer			= T_Ptr;

					T_Node* _node = nullptr;
					T_Node* _nil  = nullptr;

					constexpr TreeIterator() = default;

					constexpr TreeIterator(T_Node* n, T_Node* nil_ptr) noexcept : _node(n), _nil(nil_ptr) {
					}

					template <class U_Node, class U_Ref, class U_Ptr>
						requires std::convertible_to<U_Node*, T_Node*>
					constexpr TreeIterator(const TreeIterator<U_Node, U_Ref, U_Ptr>& other) noexcept :
						_node(other._node), _nil(other._nil) {
					}

					constexpr auto operator*() const noexcept -> T_Ref {
						return _node->_value;
					}

					constexpr auto operator->() const noexcept -> T_Ptr {
						return &_node->_value;
					}

					constexpr auto operator++() -> TreeIterator& {
						if (_node != _nil) {
							if (_node->_right != _nil) {
								_node = _node->_right;
								while (_node->_left != _nil)
									_node = _node->_left;
							}
							else {
								auto* p = _node->_parent;
								while (p != _nil && _node == p->_right) {
									_node = p;
									p	  = p->_parent;
								}
								_node = p;
							}
						}
						return *this;
					}

					constexpr auto operator++(int) -> TreeIterator {
						auto tmp = *this;
						++(*this);
						return tmp;
					}

					constexpr auto operator--() -> TreeIterator& {
						if (_node == _nil) {
							_node = _nil->_parent;
							if (_node != _nil) {
								while (_node->_right != _nil)
									_node = _node->_right;
							}
						}
						else {
							if (_node->_left != _nil) {
								_node = _node->_left;
								while (_node->_right != _nil)
									_node = _node->_right;
							}
							else {
								auto* p = _node->_parent;
								while (p != _nil && _node == p->_left) {
									_node = p;
									p	  = p->_parent;
								}
								_node = p;
							}
						}
						return *this;
					}

					constexpr auto operator--(int) -> TreeIterator {
						auto tmp = *this;
						--(*this);
						return tmp;
					}

					friend constexpr auto operator==(const TreeIterator& a, const TreeIterator& b) -> bool = default;
			};

			using iterator				 = TreeIterator<Node, value_type&, value_type*>;
			using const_iterator		 = TreeIterator<const Node, const value_type&, const value_type*>;
			using reverse_iterator		 = std::reverse_iterator<iterator>;
			using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		private:
			// --- Internal members ---
			Node			_header;
			Node*			_nil;
			size_type		_size = 0;
			comparator_type _comp;
			node_allocator	_alloc;

			// --- Internal helpers ---

			static constexpr auto _minimum(Node* n, Node* nil) noexcept -> Node* {
				if (n == nil)
					return nil;
				while (n->_left != nil)
					n = n->_left;
				return n;
			}

			static constexpr auto _maximum(Node* n, Node* nil) noexcept -> Node* {
				if (n == nil)
					return nil;
				while (n->_right != nil)
					n = n->_right;
				return n;
			}

			constexpr auto _root() const noexcept -> Node* {
				return _header._parent;
			}

			constexpr auto _root_ref() noexcept -> Node*& {
				return _header._parent;
			}

			constexpr auto _update_extremes() noexcept -> void {
				auto* r = _root();
				if (r == _nil) {
					_header._left  = _nil;
					_header._right = _nil;
				}
				else {
					_header._left  = _minimum(r, _nil);
					_header._right = _maximum(r, _nil);
				}
			}

			constexpr auto _create_node(const value_type& val) -> Node* {
				auto* mem = alloc_traits::allocate(_alloc, 1);
				alloc_traits::construct(_alloc, mem, val);
				mem->_left	  = _nil;
				mem->_right	  = _nil;
				mem->_parent  = _nil;
				mem->_isBlack = false;
				return mem;
			}

			constexpr auto _destroy_node(Node* n) noexcept -> void {
				if (n && n != _nil) {
					alloc_traits::destroy(_alloc, n);
					alloc_traits::deallocate(_alloc, n, 1);
				}
			}

			constexpr auto _destroy_subtree(Node* n) noexcept -> void {
				if (n == _nil)
					return;
				_destroy_subtree(n->_left);
				_destroy_subtree(n->_right);
				_destroy_node(n);
			}

			constexpr auto _init_header() noexcept -> void {
				_header._isBlack = true;
				_header._parent	 = _nil;
				_header._left	 = _nil;
				_header._right	 = _nil;
			}

			// --- Rotations ---
			constexpr auto _left_rotate(Node* x) noexcept -> void {
				auto* y	  = x->_right;
				x->_right = y->_left;
				if (y->_left != _nil)
					y->_left->_parent = x;
				y->_parent = x->_parent;
				if (x->_parent == _nil)
					_root_ref() = y;
				else if (x == x->_parent->_left)
					x->_parent->_left = y;
				else
					x->_parent->_right = y;
				y->_left   = x;
				x->_parent = y;
			}

			constexpr auto _right_rotate(Node* x) noexcept -> void {
				auto* y	 = x->_left;
				x->_left = y->_right;
				if (y->_right != _nil)
					y->_right->_parent = x;
				y->_parent = x->_parent;
				if (x->_parent == _nil)
					_root_ref() = y;
				else if (x == x->_parent->_right)
					x->_parent->_right = y;
				else
					x->_parent->_left = y;
				y->_right  = x;
				x->_parent = y;
			}

			// --- Insert fixup (CLRS) ---
			constexpr auto _insert_fixup(Node* z) noexcept -> void {
				while (!z->_parent->_isBlack) {
					if (z->_parent == z->_parent->_parent->_left) {
						auto* y = z->_parent->_parent->_right;
						if (!y->_isBlack) {
							z->_parent->_isBlack		  = true;
							y->_isBlack					  = true;
							z->_parent->_parent->_isBlack = false;
							z							  = z->_parent->_parent;
						}
						else {
							if (z == z->_parent->_right) {
								z = z->_parent;
								_left_rotate(z);
							}
							z->_parent->_isBlack		  = true;
							z->_parent->_parent->_isBlack = false;
							_right_rotate(z->_parent->_parent);
						}
					}
					else {
						auto* y = z->_parent->_parent->_left;
						if (!y->_isBlack) {
							z->_parent->_isBlack		  = true;
							y->_isBlack					  = true;
							z->_parent->_parent->_isBlack = false;
							z							  = z->_parent->_parent;
						}
						else {
							if (z == z->_parent->_left) {
								z = z->_parent;
								_right_rotate(z);
							}
							z->_parent->_isBlack		  = true;
							z->_parent->_parent->_isBlack = false;
							_left_rotate(z->_parent->_parent);
						}
					}
				}
				_root()->_isBlack = true;
			}

			// --- Transplant (CLRS) ---
			// NOTE: does NOT set v->_parent when v == _nil
			// because _nil == &_header and _header._parent == root
			constexpr auto _rb_transplant(Node* u, Node* v) noexcept -> void {
				if (u->_parent == _nil)
					_root_ref() = v;
				else if (u == u->_parent->_left)
					u->_parent->_left = v;
				else
					u->_parent->_right = v;
				if (v != _nil)
					v->_parent = u->_parent;
			}

			// --- Erase fixup (CLRS) ---
			// Takes explicit x_parent because x may be _nil (sentinel),
			// and _nil->_parent == _header._parent (root), not x's parent.
			constexpr auto _erase_fixup(Node* x, Node* x_parent) noexcept -> void {
				while (x != _root() && x->_isBlack) {
					auto* p = (x == _nil) ? x_parent : x->_parent;
					if (x == p->_left) {
						auto* w = p->_right;
						if (!w->_isBlack) {
							w->_isBlack = true;
							p->_isBlack = false;
							_left_rotate(p);
							w = p->_right;
						}
						if (w->_left->_isBlack && w->_right->_isBlack) {
							w->_isBlack = false;
							x			= p;
							x_parent	= p->_parent;
						}
						else {
							if (w->_right->_isBlack) {
								w->_left->_isBlack = true;
								w->_isBlack		   = false;
								_right_rotate(w);
								w = p->_right;
							}
							w->_isBlack			= p->_isBlack;
							p->_isBlack			= true;
							w->_right->_isBlack = true;
							_left_rotate(p);
							x = _root();
						}
					}
					else {
						auto* w = p->_left;
						if (!w->_isBlack) {
							w->_isBlack = true;
							p->_isBlack = false;
							_right_rotate(p);
							w = p->_left;
						}
						if (w->_right->_isBlack && w->_left->_isBlack) {
							w->_isBlack = false;
							x			= p;
							x_parent	= p->_parent;
						}
						else {
							if (w->_left->_isBlack) {
								w->_right->_isBlack = true;
								w->_isBlack			= false;
								_left_rotate(w);
								w = p->_left;
							}
							w->_isBlack		   = p->_isBlack;
							p->_isBlack		   = true;
							w->_left->_isBlack = true;
							_right_rotate(p);
							x = _root();
						}
					}
				}
				if (x != _nil)
					x->_isBlack = true;
			}

			constexpr auto _find_node(const value_type& val) const noexcept -> Node* {
				auto* n = _root();
				while (n != _nil) {
					if (_comp(val, n->_value))
						n = n->_left;
					else if (_comp(n->_value, val))
						n = n->_right;
					else
						return n;
				}
				return _nil;
			}

			constexpr auto _lower_bound(const value_type& val) const noexcept -> Node* {
				auto* n	  = _root();
				auto* res = _nil;
				while (n != _nil) {
					if (!_comp(n->_value, val)) {
						res = n;
						n	= n->_left;
					}
					else {
						n = n->_right;
					}
				}
				return res;
			}

			constexpr auto _upper_bound(const value_type& val) const noexcept -> Node* {
				auto* n	  = _root();
				auto* res = _nil;
				while (n != _nil) {
					if (_comp(val, n->_value)) {
						res = n;
						n	= n->_left;
					}
					else {
						n = n->_right;
					}
				}
				return res;
			}

			constexpr auto _successor(Node* n) const noexcept -> Node* {
				if (n == _nil)
					return _nil;
				if (n->_right != _nil)
					return _minimum(n->_right, _nil);
				auto* p = n->_parent;
				while (p != _nil && n == p->_right) {
					n = p;
					p = p->_parent;
				}
				return p;
			}

			constexpr auto _erase_node(Node* z) -> void {
				auto* y				   = z;
				auto* x				   = _nil;
				Node* x_parent		   = _nil;
				bool  y_original_color = y->_isBlack;

				if (z->_left == _nil) {
					x		 = z->_right;
					x_parent = z->_parent;
					_rb_transplant(z, z->_right);
				}
				else if (z->_right == _nil) {
					x		 = z->_left;
					x_parent = z->_parent;
					_rb_transplant(z, z->_left);
				}
				else {
					y				 = _minimum(z->_right, _nil);
					y_original_color = y->_isBlack;
					x				 = y->_right;
					if (y->_parent == z) {
						x_parent = y;
						if (x != _nil)
							x->_parent = y;
					}
					else {
						x_parent = y->_parent;
						_rb_transplant(y, y->_right);
						y->_right		   = z->_right;
						y->_right->_parent = y;
					}
					_rb_transplant(z, y);
					y->_left		  = z->_left;
					y->_left->_parent = y;
					y->_isBlack		  = z->_isBlack;
				}

				if (y_original_color)
					_erase_fixup(x, x_parent);

				_destroy_node(z);
				--_size;
				_update_extremes();
			}

			constexpr auto _relink_nil(Node* n, Node* old_nil, Node* new_nil) noexcept -> void {
				if (n == old_nil)
					return;
				if (n->_left == old_nil)
					n->_left = new_nil;
				else
					_relink_nil(n->_left, old_nil, new_nil);
				if (n->_right == old_nil)
					n->_right = new_nil;
				else
					_relink_nil(n->_right, old_nil, new_nil);
				if (n->_parent == old_nil)
					n->_parent = new_nil;
			}

		public:
			// --- Construction / Destruction ---

			constexpr RedBlackTree() noexcept(noexcept(node_allocator{})) : _nil(&_header), _size(0) {
				_init_header();
			}

			constexpr explicit RedBlackTree(const comparator_type& comp, const t_Alloc& alloc = t_Alloc{}) :
				_nil(&_header), _size(0), _comp(comp), _alloc(node_allocator(alloc)) {
				_init_header();
			}

			constexpr explicit RedBlackTree(const t_Alloc& alloc) :
				_nil(&_header), _size(0), _alloc(node_allocator(alloc)) {
				_init_header();
			}

			template <std::input_iterator T_InputIt>
			constexpr RedBlackTree(T_InputIt first, T_InputIt last, const comparator_type& comp = comparator_type{},
								   const t_Alloc& alloc = t_Alloc{}) :
				_nil(&_header), _size(0), _comp(comp), _alloc(node_allocator(alloc)) {
				_init_header();
				for (; first != last; ++first)
					insert(*first);
			}

			constexpr RedBlackTree(std::initializer_list<value_type> init,
								   const comparator_type& comp = comparator_type{}, const t_Alloc& alloc = t_Alloc{}) :
				_nil(&_header), _size(0), _comp(comp), _alloc(node_allocator(alloc)) {
				_init_header();
				for (auto& val : init)
					insert(val);
			}

			constexpr RedBlackTree(const RedBlackTree& other) :
				_nil(&_header), _size(0), _comp(other._comp),
				_alloc(alloc_traits::select_on_container_copy_construction(other._alloc)) {
				_init_header();
				for (auto& val : other)
					insert(val);
			}

			constexpr RedBlackTree(RedBlackTree&& other) noexcept :
				_nil(&_header), _size(std::exchange(other._size, 0)), _comp(std::move(other._comp)),
				_alloc(std::move(other._alloc)) {
				if (other._root() != other._nil) {
					_root_ref() = other._root();
					_relink_nil(_root(), other._nil, _nil);
					_update_extremes();
					other._root_ref()	 = other._nil;
					other._header._left	 = other._nil;
					other._header._right = other._nil;
				}
				else {
					_init_header();
				}
			}

			constexpr ~RedBlackTree() noexcept {
				_destroy_subtree(_root());
			}

			constexpr auto operator=(const RedBlackTree& other) -> RedBlackTree& {
				if (this != &other) {
					clear();
					_comp = other._comp;
					if constexpr (alloc_traits::propagate_on_container_copy_assignment::value) {
						_alloc = other._alloc;
					}
					for (auto& val : other)
						insert(val);
				}
				return *this;
			}

			constexpr auto
			operator=(RedBlackTree&& other) noexcept(std::is_nothrow_move_assignable_v<comparator_type> &&
													 std::is_nothrow_move_assignable_v<node_allocator>)
				-> RedBlackTree& {
				if (this != &other) {
					clear();
					_size = std::exchange(other._size, 0);
					_comp = std::move(other._comp);
					if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
						_alloc = std::move(other._alloc);
					}
					if (other._root() != other._nil) {
						_root_ref() = other._root();
						_relink_nil(_root(), other._nil, _nil);
						_update_extremes();
					}
					else {
						_init_header();
					}
					other._root_ref()	 = other._nil;
					other._header._left	 = other._nil;
					other._header._right = other._nil;
				}
				return *this;
			}

			// --- Iterators ---
			constexpr auto begin() noexcept -> iterator {
				return iterator(_header._left, _nil);
			}

			constexpr auto begin() const noexcept -> const_iterator {
				return const_iterator(_header._left, _nil);
			}

			constexpr auto end() noexcept -> iterator {
				return iterator(_nil, _nil);
			}

			constexpr auto end() const noexcept -> const_iterator {
				return const_iterator(_nil, _nil);
			}

			constexpr auto cbegin() const noexcept -> const_iterator {
				return begin();
			}

			constexpr auto cend() const noexcept -> const_iterator {
				return end();
			}

			constexpr auto rbegin() noexcept -> reverse_iterator {
				return reverse_iterator{end()};
			}

			constexpr auto rend() noexcept -> reverse_iterator {
				return reverse_iterator{begin()};
			}

			constexpr auto crbegin() const noexcept -> const_reverse_iterator {
				return const_reverse_iterator{cend()};
			}

			constexpr auto crend() const noexcept -> const_reverse_iterator {
				return const_reverse_iterator{cbegin()};
			}

			// --- Capacity ---
			[[nodiscard]] constexpr auto empty() const noexcept -> bool {
				return _size == 0;
			}

			[[nodiscard]] constexpr auto size() const noexcept -> size_type {
				return _size;
			}

			[[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
				return alloc_traits::max_size(_alloc);
			}

			// --- Modifiers ---
			constexpr auto clear() noexcept -> void {
				_destroy_subtree(_root());
				_root_ref()	   = _nil;
				_header._left  = _nil;
				_header._right = _nil;
				_size		   = 0;
			}

			constexpr auto insert(const value_type& val) -> std::pair<iterator, bool> {
				auto* y = _nil;
				auto* x = _root();

				while (x != _nil) {
					y = x;
					if (_comp(val, x->_value))
						x = x->_left;
					else if (_comp(x->_value, val))
						x = x->_right;
					else
						return {iterator(x, _nil), false};
				}

				auto* z	   = _create_node(val);
				z->_parent = y;

				if (y == _nil)
					_root_ref() = z;
				else if (_comp(val, y->_value))
					y->_left = z;
				else
					y->_right = z;

				_insert_fixup(z);
				++_size;
				_update_extremes();
				return {iterator(z, _nil), true};
			}

			constexpr auto insert(const_iterator hint, const value_type& val) -> iterator {
				(void)hint;
				return insert(val).first;
			}

			template <std::input_iterator T_InputIt>
			constexpr auto insert(T_InputIt first, T_InputIt last) -> void {
				for (; first != last; ++first)
					insert(*first);
			}

			constexpr auto insert(std::initializer_list<value_type> ilist) -> void {
				for (auto& val : ilist)
					insert(val);
			}

			constexpr auto erase(const_iterator pos) -> iterator {
				auto* z = const_cast<Node*>(pos._node);
				if (z == _nil)
					return end();

				auto* next = _successor(z);
				_erase_node(z);
				return iterator(next, _nil);
			}

			constexpr auto erase(const_iterator first, const_iterator last) -> iterator {
				auto it = first;
				while (it != last)
					it = erase(it);
				return iterator(const_cast<Node*>(last._node), _nil);
			}

			constexpr auto erase(const value_type& val) -> size_type {
				auto* z = _find_node(val);
				if (z == _nil)
					return 0;
				_erase_node(z);
				return 1;
			}

			constexpr auto swap(RedBlackTree& other) noexcept(std::is_nothrow_swappable_v<comparator_type> &&
															  std::is_nothrow_swappable_v<node_allocator>) -> void {
				if (this == &other)
					return;

				auto* this_old_nil	= _nil;
				auto* other_old_nil = other._nil;

				base::swap(_header._parent, other._header._parent);
				base::swap(_header._left, other._header._left);
				base::swap(_header._right, other._header._right);
				base::swap(_size, other._size);
				base::swap(_comp, other._comp);
				if constexpr (alloc_traits::propagate_on_container_swap::value) {
					base::swap(_alloc, other._alloc);
				}

				auto* r = _root();
				if (r == other_old_nil) {
					_root_ref()	   = _nil;
					_header._left  = _nil;
					_header._right = _nil;
				}
				else if (r != _nil) {
					_relink_nil(r, other_old_nil, _nil);
					_update_extremes();
				}

				r = other._root();
				if (r == this_old_nil) {
					other._root_ref()	 = other._nil;
					other._header._left	 = other._nil;
					other._header._right = other._nil;
				}
				else if (r != other._nil) {
					other._relink_nil(r, this_old_nil, other._nil);
					other._update_extremes();
				}
			}

			// --- Lookup ---
			constexpr auto find(this auto&& self, const value_type& val) -> decltype(auto) {
				auto* n			= self._root();
				auto* nil		= self._nil;
				using iter_type = std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>,
													 const_iterator, iterator>;
				while (n != nil) {
					if (self._comp(val, n->_value))
						n = n->_left;
					else if (self._comp(n->_value, val))
						n = n->_right;
					else
						return iter_type(n, nil);
				}
				return iter_type(nil, nil);
			}

			constexpr auto contains(const value_type& val) const -> bool {
				return _find_node(val) != _nil;
			}

			constexpr auto lower_bound(this auto&& self, const value_type& val) -> decltype(auto) {
				using iter_type = std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>,
													 const_iterator, iterator>;
				return iter_type(self._lower_bound(val), self._nil);
			}

			constexpr auto upper_bound(this auto&& self, const value_type& val) -> decltype(auto) {
				using iter_type = std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(self)>>,
													 const_iterator, iterator>;
				return iter_type(self._upper_bound(val), self._nil);
			}

			// --- Observers ---
			constexpr auto value_comp() const -> comparator_type {
				return _comp;
			}

			constexpr auto get_allocator() const noexcept -> allocator_type {
				return t_Alloc(_alloc);
			}
	};

	template <class t_ValueTypes, class t_ComparatorTypes, class t_Alloc>
	constexpr auto swap(RedBlackTree<t_ValueTypes, t_ComparatorTypes, t_Alloc>& a,
						RedBlackTree<t_ValueTypes, t_ComparatorTypes, t_Alloc>& b) noexcept(noexcept(a.swap(b)))
		-> void {
		a.swap(b);
	}
} // namespace base
