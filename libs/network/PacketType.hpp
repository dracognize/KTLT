#pragma once

#include <libs/base/types.hpp>

enum class PacketType : u8 {
<<<<<<< HEAD
	AccountAuth = 0,
	AccountChangePassword = 1,
	AccountCreate = 2,
	AccountToggle = 3,
	BalanceGet = 4,
	BalanceChange = 5,
	BalanceTransfer = 6,
=======
    account_auth  = 0,
    ping          = 1,
    balance_req   = 2,
    balance_resp  = 3,
>>>>>>> origin/main
};
