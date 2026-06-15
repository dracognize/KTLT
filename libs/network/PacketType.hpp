#pragma once

#include <libs/base/types.hpp>

enum class PacketType : u8 {
    account_auth  = 0,
    ping          = 1,
    balance_req   = 2,
    balance_resp  = 3,
};
