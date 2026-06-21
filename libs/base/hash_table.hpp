#pragma once

#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>

#include "algorithm.hpp"
#include "hash.hpp"
#include "types.hpp"

namespace base {

	template <class t_Key,
			  class t_Value,
			  class t_Hash = hash<t_Key>,
			  class t_KeyEqual = std::equal_to<t_Key>,
			  class t_Allocator = std::allocator<std::pair<const t_Key, t_Value>>>
	struct HashTable {
			
			using key_type = t_Key;
			using mapped_type = t_Value;
			using value_type = std::pair<const t_Key, t_Value>;
			using size_type = usize;
			using difference_type = isize;
			using hasher = t_Hash;
			using key_equal = t_KeyEqual;
			using allocator_type = t_Allocator;
			using reference = value_type &;
			using const_reference = const value_type &;
			using pointer = value_type *;
			using const_pointer = const value_type *;

		private:
			struct Node {
					value_type _value;
					Node *_next;

					template <class... T_Args>
					constexpr explicit Node(T_Args &&...args)
						: _value(std::forward<T_Args>(args)...), _next(nullptr) {
					}
			};

			using node_allocator =
				typename std::allocator_traits<t_Allocator>::template rebind_alloc<Node>;
			using alloc_traits = std::allocator_traits<node_allocator>;

			static constexpr size_type DEFAULT_BUCKET_COUNT = 8;
			static constexpr size_type REHASH_GROWTH_FACTOR = 2;
			static constexpr float DEFAULT_MAX_LOAD = 1.0f;

			node_allocator _node_alloc;
			Node **_buckets = nullptr;
			size_type _bucket_count = 0;
			size_type _size = 0;
			float _max_load_factor_v = DEFAULT_MAX_LOAD;
			t_Hash _hasher;
			t_KeyEqual _key_equal;

			
		public:
			template <class T_Node, class T_Value> struct Iterator {
					using iterator_category = std::forward_iterator_tag;
					using value_type = T_Value;
					using difference_type = isize;
					using pointer = T_Value *;
					using reference = T_Value &;

					
					
					
					Node **_bucket = nullptr;
					Node **_buckets_end = nullptr;
					T_Node *_node = nullptr;

					constexpr Iterator() = default;

					constexpr Iterator(Node **bucket, Node **buckets_end, T_Node *node) noexcept
						: _bucket(bucket), _buckets_end(buckets_end), _node(node) {
					}

					constexpr auto operator*() const -> T_Value & {
						return _node->_value;
					}

					constexpr auto operator->() const -> T_Value * {
						return &_node->_value;
					}

					constexpr auto operator++() -> Iterator & {
						if (_node)
							_node = _node->_next;
						while (!_node && _bucket != _buckets_end) {
							++_bucket;
							if (_bucket != _buckets_end)
								_node = static_cast<T_Node *>(*_bucket);
						}
						return *this;
					}

					constexpr auto operator++(int) -> Iterator {
						auto tmp = *this;
						++(*this);
						return tmp;
					}

					template <class T_UNode, class T_UValue>
						requires std::convertible_to<T_UNode *, T_Node *>
					constexpr Iterator(const Iterator<T_UNode, T_UValue> &other) noexcept
						: _bucket(other._bucket), _buckets_end(other._buckets_end),
						  _node(other._node) {
					}

					friend constexpr auto operator==(const Iterator &a, const Iterator &b) -> bool
						= default;
			};

			using iterator = Iterator<Node, value_type>;
			using const_iterator = Iterator<const Node, const value_type>;

			
			constexpr HashTable() : _bucket_count(DEFAULT_BUCKET_COUNT) {
				_buckets = _alloc_buckets(_bucket_count);
			}

			constexpr explicit HashTable(const t_Hash &hasher,
										 const t_KeyEqual &equal,
										 const t_Allocator &alloc = t_Allocator{}) noexcept
				: _node_alloc(alloc), _bucket_count(DEFAULT_BUCKET_COUNT), _hasher(hasher),
				  _key_equal(equal) {
				_buckets = _alloc_buckets(_bucket_count);
			}

			constexpr explicit HashTable(const t_Allocator &alloc) noexcept
				: _node_alloc(alloc), _bucket_count(DEFAULT_BUCKET_COUNT) {
				_buckets = _alloc_buckets(_bucket_count);
			}

			constexpr HashTable(size_type bucket_hint,
								const t_Hash &hasher = t_Hash{},
								const t_KeyEqual &equal = t_KeyEqual{},
								const t_Allocator &alloc = t_Allocator{})
				: _node_alloc(alloc),
				  _bucket_count(bucket_hint > DEFAULT_BUCKET_COUNT ? bucket_hint
																   : DEFAULT_BUCKET_COUNT),
				  _hasher(hasher), _key_equal(equal) {
				_buckets = _alloc_buckets(_bucket_count);
			}

			constexpr HashTable(const HashTable &other)
				: _node_alloc(
					  alloc_traits::select_on_container_copy_construction(other._node_alloc)),
				  _bucket_count(other._bucket_count), _max_load_factor_v(other._max_load_factor_v),
				  _hasher(other._hasher), _key_equal(other._key_equal) {
				_buckets = _alloc_buckets(_bucket_count);
				for (const auto &val : other) {
					insert(val);
				}
			}

			constexpr HashTable(HashTable &&other) noexcept
				: _node_alloc(std::move(other._node_alloc)), _buckets(other._buckets),
				  _bucket_count(other._bucket_count), _size(other._size),
				  _max_load_factor_v(other._max_load_factor_v), _hasher(std::move(other._hasher)),
				  _key_equal(std::move(other._key_equal)) {
				other._buckets = nullptr;
				other._bucket_count = 0;
				other._size = 0;
			}

			constexpr HashTable(std::initializer_list<value_type> init,
								const t_Hash &hasher = t_Hash{},
								const t_KeyEqual &equal = t_KeyEqual{},
								const t_Allocator &alloc = t_Allocator{})
				: _node_alloc(alloc), _bucket_count(DEFAULT_BUCKET_COUNT), _hasher(hasher),
				  _key_equal(equal) {
				_buckets = _alloc_buckets(_bucket_count);
				reserve(init.size());
				for (const auto &val : init) {
					insert(val);
				}
			}

			constexpr ~HashTable() noexcept {
				_clear();
				_free_buckets();
			}

			
			constexpr auto operator=(const HashTable &other) -> HashTable & {
				if (this != &other) {
					_clear();
					_hasher = other._hasher;
					_key_equal = other._key_equal;
					_max_load_factor_v = other._max_load_factor_v;
					if constexpr (alloc_traits::propagate_on_container_copy_assignment::value)
						_node_alloc = other._node_alloc;
					_rehash(other._bucket_count);
					for (const auto &val : other) {
						insert(val);
					}
				}
				return *this;
			}

			constexpr auto operator=(HashTable &&other) noexcept -> HashTable & {
				if (this != &other) {
					_clear();
					_free_buckets();
					_node_alloc = std::move(other._node_alloc);
					_buckets = other._buckets;
					_bucket_count = other._bucket_count;
					_size = other._size;
					_max_load_factor_v = other._max_load_factor_v;
					_hasher = std::move(other._hasher);
					_key_equal = std::move(other._key_equal);
					other._buckets = nullptr;
					other._bucket_count = 0;
					other._size = 0;
				}
				return *this;
			}

			constexpr auto operator=(std::initializer_list<value_type> init) -> HashTable & {
				clear();
				reserve(init.size());
				for (const auto &val : init) {
					insert(val);
				}
				return *this;
			}

			
			constexpr auto begin() noexcept -> iterator {
				return _make_iterator(
					_buckets, _buckets + _bucket_count, _buckets ? *_buckets : nullptr);
			}

			constexpr auto begin() const noexcept -> const_iterator {
				return _make_const_iterator(
					_buckets, _buckets + _bucket_count, _buckets ? *_buckets : nullptr);
			}

			constexpr auto cbegin() const noexcept -> const_iterator {
				return begin();
			}

			constexpr auto end() noexcept -> iterator {
				return iterator(_buckets + _bucket_count, _buckets + _bucket_count, nullptr);
			}

			constexpr auto end() const noexcept -> const_iterator {
				return const_iterator(_buckets + _bucket_count, _buckets + _bucket_count, nullptr);
			}

			constexpr auto cend() const noexcept -> const_iterator {
				return end();
			}

			
			[[nodiscard]] constexpr auto empty() const noexcept -> bool {
				return _size == 0;
			}

			[[nodiscard]] constexpr auto size() const noexcept -> size_type {
				return _size;
			}

			[[nodiscard]] constexpr auto max_size() const noexcept -> size_type {
				return alloc_traits::max_size(_node_alloc);
			}

			
			constexpr auto clear() noexcept -> void {
				_clear();
				_size = 0;
			}

			template <class... T_Args>
			constexpr auto emplace(T_Args &&...args) -> std::pair<iterator, bool> {
				auto node_owner = _create_node(std::forward<T_Args>(args)...);
				const auto &key = node_owner->_value.first;
				auto [bucket, exists] = _find_insert_bucket(key);
				if (exists) {
					_destroy_node(node_owner);
					return {_make_iterator_from_node(bucket, *bucket), false};
				}
				node_owner->_next = *bucket;
				*bucket = node_owner;
				++_size;
				_rehash_if_needed();
				return {_make_iterator_from_node(bucket, node_owner), true};
			}

			constexpr auto insert(const value_type &value) -> std::pair<iterator, bool> {
				return emplace(value);
			}

			constexpr auto insert(value_type &&value) -> std::pair<iterator, bool> {
				return emplace(std::move(value));
			}

			constexpr auto erase(const_iterator pos) -> iterator {
				auto *node = const_cast<Node *>(pos._node);
				if (!node)
					return end();
				const auto &key = node->_value.first;
				auto idx = _bucket_index(key);
				auto *cur = _buckets[idx];
				Node *prev = nullptr;
				while (cur) {
					if (cur == node) {
						if (prev)
							prev->_next = cur->_next;
						else
							_buckets[idx] = cur->_next;
						auto *next = cur->_next;
						_destroy_node(cur);
						--_size;
						return iterator(_buckets + idx, _buckets + _bucket_count, next);
					}
					prev = cur;
					cur = cur->_next;
				}
				return end();
			}

			constexpr auto erase(const key_type &key) -> size_type {
				auto idx = _bucket_index(key);
				auto *cur = _buckets[idx];
				Node *prev = nullptr;
				while (cur) {
					if (_key_equal(cur->_value.first, key)) {
						if (prev)
							prev->_next = cur->_next;
						else
							_buckets[idx] = cur->_next;
						_destroy_node(cur);
						--_size;
						return 1;
					}
					prev = cur;
					cur = cur->_next;
				}
				return 0;
			}

			
			constexpr auto find(const key_type &key) -> iterator {
				auto idx = _bucket_index(key);
				auto cur = _buckets[idx];
				while (cur) {
					if (_key_equal(cur->_value.first, key))
						return iterator(_buckets + idx, _buckets + _bucket_count, cur);
					cur = cur->_next;
				}
				return end();
			}

			constexpr auto find(const key_type &key) const -> const_iterator {
				auto idx = _bucket_index(key);
				auto cur = _buckets[idx];
				while (cur) {
					if (_key_equal(cur->_value.first, key))
						return const_iterator(_buckets + idx,
											  _buckets + _bucket_count,
											  static_cast<const Node *>(cur));
					cur = cur->_next;
				}
				return end();
			}

			constexpr auto contains(const key_type &key) const -> bool {
				return find(key) != end();
			}

			
			constexpr auto bucket_count() const noexcept -> size_type {
				return _bucket_count;
			}

			constexpr auto bucket_size(size_type n) const noexcept -> size_type {
				size_type count = 0;
				auto *cur = _buckets[n];
				while (cur) {
					++count;
					cur = cur->_next;
				}
				return count;
			}

			constexpr auto bucket(const key_type &key) const -> size_type {
				return _bucket_index(key);
			}

			
			constexpr auto load_factor() const noexcept -> float {
				return _bucket_count ? static_cast<float>(_size) / static_cast<float>(_bucket_count)
									 : 0.0f;
			}

			constexpr auto max_load_factor() const noexcept -> float {
				return _max_load_factor_v;
			}

			constexpr auto max_load_factor(float ml) noexcept -> void {
				_max_load_factor_v = ml;
			}

			constexpr auto rehash(size_type new_bucket_count) -> void {
				if (new_bucket_count < DEFAULT_BUCKET_COUNT)
					new_bucket_count = DEFAULT_BUCKET_COUNT;
				if (load_factor() > _max_load_factor_v || new_bucket_count > _bucket_count)
					_rehash(new_bucket_count);
			}

			constexpr auto reserve(size_type n) -> void {
				if (n > 0)
					rehash(static_cast<size_type>(static_cast<float>(n) / _max_load_factor_v) + 1);
			}

			
			constexpr auto swap(HashTable &other) noexcept -> void {
				base::swap(_node_alloc, other._node_alloc);
				base::swap(_buckets, other._buckets);
				base::swap(_bucket_count, other._bucket_count);
				base::swap(_size, other._size);
				base::swap(_max_load_factor_v, other._max_load_factor_v);
				base::swap(_hasher, other._hasher);
				base::swap(_key_equal, other._key_equal);
			}

			
			constexpr auto hash_function() const -> hasher {
				return _hasher;
			}

			constexpr auto key_eq() const -> key_equal {
				return _key_equal;
			}

			constexpr auto get_allocator() const noexcept -> allocator_type {
				return allocator_type(_node_alloc);
			}

		private:
			constexpr auto _alloc_buckets(size_type count) -> Node ** {
				auto *mem = std::allocator<Node *>().allocate(count);
				for (size_type i = 0; i < count; ++i)
					mem[i] = nullptr;
				return mem;
			}

			constexpr auto _free_buckets() noexcept -> void {
				if (_buckets) {
					std::allocator<Node *>().deallocate(_buckets, _bucket_count);
					_buckets = nullptr;
				}
			}

			constexpr auto _bucket_index(const key_type &key) const -> size_type {
				return _hasher(key) % _bucket_count;
			}

			template <class... T_Args> constexpr auto _create_node(T_Args &&...args) -> Node * {
				auto *mem = alloc_traits::allocate(_node_alloc, 1);
				alloc_traits::construct(_node_alloc, mem, std::forward<T_Args>(args)...);
				return mem;
			}

			constexpr auto _destroy_node(Node *n) noexcept -> void {
				if (n) {
					alloc_traits::destroy(_node_alloc, n);
					alloc_traits::deallocate(_node_alloc, n, 1);
				}
			}

			constexpr auto _clear() noexcept -> void {
				for (size_type i = 0; i < _bucket_count; ++i) {
					auto *cur = _buckets[i];
					while (cur) {
						auto *next = cur->_next;
						_destroy_node(cur);
						cur = next;
					}
					_buckets[i] = nullptr;
				}
			}

			
			
			constexpr auto _find_insert_bucket(const key_type &key) -> std::pair<Node **, bool> {
				auto idx = _bucket_index(key);
				auto *cur = _buckets[idx];
				while (cur) {
					if (_key_equal(cur->_value.first, key))
						return {_buckets + idx, true};
					cur = cur->_next;
				}
				return {_buckets + idx, false};
			}

			constexpr auto _rehash(size_type new_bucket_count) -> void {
				auto **old_buckets = _buckets;
				auto old_bucket_count = _bucket_count;

				_buckets = _alloc_buckets(new_bucket_count);
				_bucket_count = new_bucket_count;

				for (size_type i = 0; i < old_bucket_count; ++i) {
					auto *cur = old_buckets[i];
					while (cur) {
						auto *next = cur->_next;
						auto idx = _bucket_index(cur->_value.first);
						cur->_next = _buckets[idx];
						_buckets[idx] = cur;
						cur = next;
					}
				}

				std::allocator<Node *>().deallocate(old_buckets, old_bucket_count);
			}

			constexpr auto _rehash_if_needed() -> void {
				if (load_factor() > _max_load_factor_v)
					_rehash(_bucket_count * REHASH_GROWTH_FACTOR);
			}

			constexpr auto _make_iterator(Node **bucket, Node **end, Node *first_node) -> iterator {
				if (first_node)
					return iterator(bucket, end, first_node);
				while (bucket != end) {
					if (*bucket)
						return iterator(bucket, end, *bucket);
					++bucket;
				}
				return iterator(end, end, nullptr);
			}

			constexpr auto _make_const_iterator(Node **bucket, Node **end, Node *first_node) const
				-> const_iterator {
				if (first_node)
					return const_iterator(bucket, end, static_cast<const Node *>(first_node));
				while (bucket != end) {
					if (*bucket)
						return const_iterator(bucket, end, static_cast<const Node *>(*bucket));
					++bucket;
				}
				return const_iterator(end, end, nullptr);
			}

			constexpr auto _make_iterator_from_node(Node **bucket, Node *node) -> iterator {
				return iterator(bucket, _buckets + _bucket_count, node);
			}
	};

	template <class t_Key, class t_Value, class t_Hash, class t_KeyEqual, class t_Alloc>
	constexpr auto
	swap(HashTable<t_Key, t_Value, t_Hash, t_KeyEqual, t_Alloc> &a,
		 HashTable<t_Key, t_Value, t_Hash, t_KeyEqual, t_Alloc> &b) noexcept(noexcept(a.swap(b)))
		-> void {
		a.swap(b);
	}

} 
