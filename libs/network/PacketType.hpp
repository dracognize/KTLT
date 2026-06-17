#pragma once

#include <libs/base/types.hpp>

enum class PacketType : u8 {
	account_auth   = 0,
	account_create = 1,
	balance_req    = 2,
	balance_resp   = 3,
	balance_change = 4,
	balance_transfer = 5,
	ping           = 6,
	account_change_password = 7,
	account_toggle = 8,
	transaction_history = 9,
};
