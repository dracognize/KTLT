#pragma once

#include <libs/base/types.hpp>

enum class PacketType : u8 {
	account_auth = 0,
	account_create = 1,
<<<<<<< HEAD
	balance_req    = 2,
	balance_resp   = 3,
	balance_change = 4,
	balance_transfer = 5,
	ping           = 6,
	account_change_password = 7,
	account_toggle = 8,
	transaction_history = 9,
=======
	balance_req = 2,
	balance_change = 3,
	balance_transfer = 4,
	ping = 5,
	account_change_password = 6,
	account_toggle = 7,
	log_req = 8,
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
};
