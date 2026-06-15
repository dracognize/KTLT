#pragma once

#include <type_traits>
#include <utility>

#include "types.hpp"

namespace base {
	template <class t_Type>
	constexpr auto swap(t_Type& a, t_Type& b) noexcept(std::is_nothrow_move_constructible_v<t_Type> &&
													   std::is_nothrow_move_assignable_v<t_Type>) -> void {
		t_Type tmp = std::move(a);
		a		   = std::move(b);
		b		   = std::move(tmp);
	}
} // namespace base
