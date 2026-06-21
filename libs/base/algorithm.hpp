#pragma once

#include <concepts>
#include <iterator>
#include <type_traits>
#include <utility>

#include "types.hpp"

namespace base {
	template <class t_Type>
	constexpr auto swap(t_Type &a, t_Type &b) noexcept(std::is_nothrow_move_constructible_v<t_Type>
													   && std::is_nothrow_move_assignable_v<t_Type>)
		-> void {
		t_Type tmp = std::move(a);
		a = std::move(b);
		b = std::move(tmp);
	}

	
	template <std::input_iterator T_InputIt, class T_Type>
		requires std::equality_comparable_with<typename std::iterator_traits<T_InputIt>::value_type,
											   const T_Type &>
	[[nodiscard]] constexpr auto find(T_InputIt first, T_InputIt last, const T_Type &value)
		-> T_InputIt {
		for (; first != last; ++first) {
			if (*first == value)
				return first;
		}
		return last;
	}

	
	template <std::input_iterator T_InputIt, std::predicate<const typename std::iterator_traits<T_InputIt>::value_type &> T_Pred>
	[[nodiscard]] constexpr auto find_if(T_InputIt first, T_InputIt last, T_Pred pred)
		-> T_InputIt {
		for (; first != last; ++first) {
			if (pred(*first))
				return first;
		}
		return last;
	}

	
	
	template <std::random_access_iterator T_RandomIt, class T_Compare = std::less<>>
	constexpr auto sort(T_RandomIt first, T_RandomIt last, T_Compare comp = {}) -> void {
		auto size = last - first;
		if (size <= 1)
			return;

		
		if (size <= 16) {
			for (auto i = first + 1; i != last; ++i) {
				for (auto j = i; j != first && comp(*j, *(j - 1)); --j) {
					base::swap(*j, *(j - 1));
				}
			}
			return;
		}

		
		auto pivot = *(first + size / 2);
		auto f = first;
		auto l = last - 1;
		while (true) {
			while (comp(*f, pivot))
				++f;
			while (comp(pivot, *l))
				--l;
			if (f >= l)
				break;
			base::swap(*f, *l);
			++f;
			--l;
		}

		if (f - first > 1)
			sort(first, f, comp);
		if (last - l > 1)
			sort(l + 1, last, comp);
	}
} 
