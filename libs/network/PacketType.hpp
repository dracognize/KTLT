#pragma once

#include <libs/base/types.hpp>

enum class PacketType : u8 {
	account_auth   = 0,
	account_create = 1,
	balance_req    = 2,
	balance_change = 3,
	balance_transfer = 4,
	ping           = 5,
	account_change_password = 6,
	account_toggle = 7,
};
