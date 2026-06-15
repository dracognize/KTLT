#pragma once

#include <libs/base/types.hpp>

enum class PacketType : u8 {
	AccountAuth = 0,
	AccountChangePassword = 1,
	AccountCreate = 2,
	AccountToggle = 3,
	BalanceGet = 4,
	BalanceChange = 5,
	BalanceTransfer = 6,
};
