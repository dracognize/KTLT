# Base Library Reference

The `libs/base/` directory contains a high-performance, header-only implementation of core data structures and algorithms. These are designed to be modern C++26 replacements for standard library components, optimized for the specific requirements of the Banking Terminal project.

## Core Principles
- **C++26 Standards**: Utilizes advanced features like "Deducing this", concepts, and constexpr everything.
- **STL Compatibility**: Follows standard naming conventions and iterator requirements to be a drop-in replacement where possible.
- **Minimal Dependencies**: Only depends on standard headers like `<concepts>`, `<memory>`, and `<utility>`.

## Data Structures

### `base::Vector<T>`
A dynamic array implementation similar to `std::vector`.
- **Header**: `libs/base/vector.hpp`
- **Growth Strategy**: Default growth factor of 2.
- **Key Features**:
  - `constexpr` support for all operations.
  - Exception-safe memory management.
  - Supports `emplace_back` and `push_back` with move semantics.
  - Uses `std::allocator_traits` for custom allocator support.

### `base::List<T>`
A doubly-linked list implementation similar to `std::list`.
- **Header**: `libs/base/list.hpp`
- **Key Features**:
  - `O(1)` insertion and deletion at any point.
  - Bidirectional iterators.
  - Built-in `sort()` using Merge Sort algorithm.
  - Circularly linked list with a sentinel header node for efficient operations.

### `base::HashTable<Key, Value>`
A high-performance hash map implementation using separate chaining.
- **Header**: `libs/base/hash_table.hpp`
- **Key Features**:
  - Dynamic rehashing based on a configurable load factor.
  - Supports custom hash and equality functions.
  - `O(1)` average case lookup, insertion, and deletion.

### `base::RedBlackTree<T>`
A balanced binary search tree implementation.
- **Header**: `libs/base/rbtree.hpp`
- **Key Features**:
  - Guarantees `O(log n)` operations for lookup, insertion, and deletion.
  - Used as the underlying structure for `base::Map` and `base::Set`.
  - Implements standard CLRS (Cormen et al.) Red-Black Tree algorithms.
  - Efficient iterator traversal.

### `base::String`
A custom string implementation with support for common string operations.
- **Header**: `libs/base/string.hpp`
- **Key Features**:
  - Optimized for small string optimizations (SSO).
  - Integrates with the custom `Vector` and `Algorithm` utilities.

## Utilities & Algorithms

### `algorithm.hpp`
A collection of generic algorithms similar to `<algorithm>`.
- Includes: `copy`, `move`, `fill`, `find`, `find_if`, `swap`, etc.
- Leverages C++26 concepts for compile-time type checking.

### `hash.hpp`
Generic hash functions for basic types and strings.
- Used by `HashTable`, `Map`, and `Set`.

### `sha256.hpp`
A standalone implementation of the SHA-256 cryptographic hash algorithm.
- Used for secure password hashing in the banking terminal.

### `types.hpp`
Standardized type aliases for consistent sizing across platforms.
- `u8`, `u16`, `u32`, `u64`: Unsigned integers.
- `i8`, `i16`, `i32`, `i64`: Signed integers.
- `f32`, `f64`: Floating point numbers.
- `usize`, `isize`: Platform-dependent size types.
- `b1`, `b8`: Boolean types.
