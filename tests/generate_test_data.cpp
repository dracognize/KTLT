#include "libs/base/types.hpp"

#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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

	auto logName = std::string(user) + ".log";
	std::strncpy(r.logFile, logName.c_str(), sizeof(r.logFile) - 1);

	r.balance = toNetwork(balance);
	r.isLocked = locked ? 1 : 0;
	f.write(reinterpret_cast<const char *>(&r), sizeof(r));
}

void writeLog(const char *user, u64 balance) {
	auto path = std::string("data/") + user + ".log";
	auto f = std::ofstream(path, std::ios::trunc);
	if (f) {
		f << "account created with balance " << balance << "\n";
	}
}

} // namespace

int main() {
	std::filesystem::create_directories("data");

	std::ofstream f("data.db", std::ios::binary);
	if (!f) {
		std::cerr << "failed to open data.db for writing\n";
		return 1;
	}

	writeRecord(f, "admin", "admin123", 999'999, false);
	writeLog("admin", 999'999);

	writeRecord(f, "user1", "password1", 50'000, false);
	writeLog("user1", 50'000);

	writeRecord(f, "locked", "lockedpw", 1'000, true);
	writeLog("locked", 1'000);

	std::cout << "wrote 3 test accounts to data.db\n";
	std::cout << "wrote log files to data/*.log\n";
	return 0;
}
