#include "libs/base/sha256.hpp"
#include "libs/base/types.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
<<<<<<< HEAD
#include <fstream>
#include <iostream>
=======
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Must match the constants in server/Database.hpp
inline constexpr usize UsernameMaxLen = 23;
inline constexpr usize PasswordSaltLen = 16;
inline constexpr usize PasswordHashLen = 32;
inline constexpr usize PasswordStoreLen = PasswordSaltLen + PasswordHashLen; // 48
inline constexpr usize LogFileMaxLen = 31;
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

#pragma pack(push, 1)
struct Record {
		char username[UsernameMaxLen + 1]; // 24
		char password[PasswordStoreLen];   // 48 (16 salt + 32 hash)
		u64 balance;
		char logFile[LogFileMaxLen + 1]; // 32
		b8 isLocked;
};
#pragma pack(pop)

static_assert(sizeof(Record) == 24 + 48 + 8 + 32 + 1, "Record size mismatch");

namespace {

	[[nodiscard]] auto toNetwork(u64 val) noexcept -> u64 {
		if constexpr (std::endian::native == std::endian::little) {
			return std::byteswap(val);
		}
		return val;
	}

<<<<<<< HEAD
void writeRecord(std::ofstream &f, const char *user, const char *pass, u64 balance, bool locked) {
	Record r{};
	std::strncpy(r.username, user, sizeof(r.username) - 1);
	std::strncpy(r.password, pass, sizeof(r.password) - 1);
	r.balance = toNetwork(balance);
	std::strncpy(r.logFile, "", sizeof(r.logFile) - 1);
	r.isLocked = locked ? 1 : 0;
	f.write(reinterpret_cast<const char *>(&r), sizeof(r));
}

=======
	void writeRecord(std::ofstream &f,
					 const std::string &user,
					 const std::string &pass,
					 u64 balance,
					 bool locked) {
		Record r{};
		std::memset(&r, 0, sizeof(r));

		auto logName = user + ".log";

		// Generate salted SHA-256 hash matching Database::hashPasswordInto
		auto salt = base::generateSalt();
		auto rawHash = base::Sha256::hash(salt + ":" + pass);

		// Fill record fields
		std::memcpy(r.username, user.c_str(), (std::min)(user.size(), sizeof(r.username) - 1));
		std::memcpy(r.password,
					salt.c_str(),
					(std::min)(salt.size(), static_cast<size_t>(PasswordSaltLen)));
		std::memcpy(r.password + PasswordSaltLen, rawHash.data(), PasswordHashLen);
		std::memcpy(r.logFile, logName.c_str(), (std::min)(logName.size(), sizeof(r.logFile) - 1));
		r.balance = toNetwork(balance);
		r.isLocked = locked ? 1 : 0;

		f.write(reinterpret_cast<const char *>(&r), sizeof(r));

		std::cout << "  wrote user '" << user << "' (password hash: " << salt.substr(0, 4) << "..."
				  << base::Sha256::hashHex(salt + ":" + pass).substr(0, 8) << "...)\n";
	}

	void writeLog(const std::string &user, const std::vector<std::string> &lines) {
		auto path = std::string("data/") + user + ".log";
		auto f = std::ofstream(path, std::ios::trunc);
		if (f) {
			for (const auto &line : lines) {
				f << line << "\n";
			}
		}
	}

	struct Tx {
			std::string message;
			u64 balance;
	};

	void appendDeposit(std::vector<Tx> &txs, u64 amount, u64 &balance) {
		balance += amount;
		txs.push_back(
			{"Deposit +" + std::to_string(amount) + " " + std::to_string(balance), balance});
	}

	void appendWithdrawal(std::vector<Tx> &txs, u64 amount, u64 &balance) {
		balance -= amount;
		txs.push_back(
			{"Withdrawal -" + std::to_string(amount) + " " + std::to_string(balance), balance});
	}

	void appendTransferOut(std::vector<Tx> &txs, u64 amount, const std::string &to, u64 &balance) {
		balance -= amount;
		txs.push_back(
			{"Transfer to " + to + " -" + std::to_string(amount) + " " + std::to_string(balance),
			 balance});
	}

	void appendTransferIn(std::vector<Tx> &txs, u64 amount, const std::string &from, u64 &balance) {
		balance += amount;
		txs.push_back({"Transfer from " + from + " +" + std::to_string(amount) + " "
						   + std::to_string(balance),
					   balance});
	}

	auto formatLog(const std::string &msg, int seq) -> std::string {
		auto total = 8 * 3600 + 30 * 60 + seq * 12;
		auto h = total / 3600;
		auto m = (total % 3600) / 60;
		auto s = total % 60;
		return std::format("[2026-06-17 {:02}:{:02}:{:02}] {}", h, m, s, msg);
	}

	std::vector<std::string> generateAdminLog(u64 &finalBalance) {
		std::vector<Tx> txs;
		u64 bal = 999'999;

		txs.push_back({"Account created with balance " + std::to_string(bal), bal});

		appendDeposit(txs, 500'000, bal);
		appendDeposit(txs, 200'000, bal);
		appendWithdrawal(txs, 100'000, bal);
		appendTransferOut(txs, 50'000, "user1", bal);
		appendDeposit(txs, 1'000'000, bal);
		appendWithdrawal(txs, 300'000, bal);
		appendTransferIn(txs, 25'000, "user1", bal);
		appendDeposit(txs, 800'000, bal);
		appendWithdrawal(txs, 75'000, bal);
		appendTransferOut(txs, 100'000, "user1", bal);
		appendDeposit(txs, 50'000, bal);
		appendWithdrawal(txs, 200'000, bal);
		appendTransferIn(txs, 150'000, "user1", bal);
		appendDeposit(txs, 400'000, bal);
		appendWithdrawal(txs, 500'000, bal);
		appendDeposit(txs, 900'000, bal);
		appendTransferOut(txs, 75'000, "user1", bal);
		appendWithdrawal(txs, 25'000, bal);
		appendDeposit(txs, 100'000, bal);
		appendTransferIn(txs, 50'000, "user1", bal);
		appendWithdrawal(txs, 150'000, bal);
		appendDeposit(txs, 600'000, bal);
		appendTransferOut(txs, 200'000, "user1", bal);
		appendWithdrawal(txs, 100'000, bal);
		appendDeposit(txs, 50'000, bal);
		appendTransferIn(txs, 300'000, "user1", bal);
		appendWithdrawal(txs, 400'000, bal);
		appendDeposit(txs, 750'000, bal);
		appendTransferOut(txs, 25'000, "user1", bal);

		finalBalance = bal;
		std::vector<std::string> result;
		for (std::size_t i = 0; i < txs.size(); ++i) {
			result.push_back(formatLog(txs[i].message, static_cast<int>(i)));
		}
		return result;
	}

	std::vector<std::string> generateUser1Log(u64 &finalBalance) {
		std::vector<Tx> txs;
		u64 bal = 50'000;

		txs.push_back({"Account created with balance " + std::to_string(bal), bal});

		appendDeposit(txs, 100'000, bal);
		appendWithdrawal(txs, 10'000, bal);
		appendTransferIn(txs, 50'000, "admin", bal);
		appendDeposit(txs, 25'000, bal);
		appendWithdrawal(txs, 5'000, bal);
		appendTransferOut(txs, 25'000, "admin", bal);
		appendDeposit(txs, 200'000, bal);
		appendWithdrawal(txs, 30'000, bal);
		appendDeposit(txs, 15'000, bal);
		appendTransferIn(txs, 100'000, "admin", bal);
		appendWithdrawal(txs, 50'000, bal);
		appendDeposit(txs, 80'000, bal);
		appendTransferOut(txs, 150'000, "admin", bal);
		appendWithdrawal(txs, 20'000, bal);
		appendDeposit(txs, 40'000, bal);
		appendTransferIn(txs, 75'000, "admin", bal);
		appendWithdrawal(txs, 15'000, bal);
		appendDeposit(txs, 60'000, bal);
		appendTransferOut(txs, 50'000, "admin", bal);
		appendWithdrawal(txs, 25'000, bal);
		appendDeposit(txs, 90'000, bal);
		appendTransferIn(txs, 200'000, "admin", bal);
		appendWithdrawal(txs, 35'000, bal);
		appendDeposit(txs, 10'000, bal);
		appendTransferOut(txs, 300'000, "admin", bal);
		appendDeposit(txs, 45'000, bal);
		appendWithdrawal(txs, 8'000, bal);
		appendTransferIn(txs, 25'000, "admin", bal);
		appendDeposit(txs, 70'000, bal);

		finalBalance = bal;
		std::vector<std::string> result;
		for (std::size_t i = 0; i < txs.size(); ++i) {
			result.push_back(formatLog(txs[i].message, static_cast<int>(i)));
		}
		return result;
	}

>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
} // namespace

int main() {
	std::ofstream f("data.db", std::ios::binary);
	if (!f) {
		std::cerr << "failed to open data.db for writing\n";
		return 1;
	}

<<<<<<< HEAD
	writeRecord(f, "admin", "admin123", 999'999, false);
	writeRecord(f, "user1", "password1", 50'000, false);
	writeRecord(f, "locked", "lockedpw", 1'000, true);

	std::cout << "wrote 3 test accounts to data.db\n";
=======
	u64 adminBalance = 0;
	u64 user1Balance = 0;
	auto adminLog = generateAdminLog(adminBalance);
	auto user1Log = generateUser1Log(user1Balance);

	std::cout << "Generating test data...\n";
	writeRecord(f, "admin", "admin123", adminBalance, false);
	writeLog("admin", adminLog);

	writeRecord(f, "user1", "password1", user1Balance, false);
	writeLog("user1", user1Log);

	writeRecord(f, "locked", "lockedpw", 1'000, true);
	writeLog("locked", {"[2026-06-17 08:30:00] Account created with balance 1000"});

	std::cout << "wrote 3 test accounts to data.db\n";
	std::cout << "  admin:   balance " << adminBalance << " (" << (adminLog.size())
			  << " log entries)\n";
	std::cout << "  user1:   balance " << user1Balance << " (" << (user1Log.size())
			  << " log entries)\n";
	std::cout << "  locked:  balance 1,000 (account locked)\n";
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
	return 0;
}
