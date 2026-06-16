#include "libs/base/types.hpp"

#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

#pragma pack(push, 1)
struct Record {
		char username[24];
		char password[24];
		u64 balance;
		char logFile[32];
		b8 isLocked;
};
#pragma pack(pop)

static_assert(sizeof(Record) == 89);

namespace {

[[nodiscard]] auto toNetwork(u64 val) noexcept -> u64 {
	if constexpr (std::endian::native == std::endian::little) {
		return std::byteswap(val);
	}
	return val;
}

void writeRecord(std::ofstream &f, const char *user, const char *pass, u64 balance, bool locked) {
	Record r{};
	std::strncpy(r.username, user, sizeof(r.username) - 1);
	std::strncpy(r.password, pass, sizeof(r.password) - 1);
	r.balance = toNetwork(balance);
	std::strncpy(r.logFile, "", sizeof(r.logFile) - 1);
	r.isLocked = locked ? 1 : 0;
	f.write(reinterpret_cast<const char *>(&r), sizeof(r));
}

} // namespace

int main() {
	std::ofstream f("data.db", std::ios::binary);
	if (!f) {
		std::cerr << "failed to open data.db for writing\n";
		return 1;
	}

	writeRecord(f, "admin", "admin123", 999'999, false);
	writeRecord(f, "user1", "password1", 50'000, false);
	writeRecord(f, "locked", "lockedpw", 1'000, true);

	std::cout << "wrote 3 test accounts to data.db\n";
	return 0;
}
