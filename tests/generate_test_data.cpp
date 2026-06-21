#include "libs/base/sha256.hpp"
#include "libs/base/types.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>


inline constexpr usize UsernameMaxLen = 23;
inline constexpr usize PasswordSaltLen = 16;
inline constexpr usize PasswordHashLen = 32;
inline constexpr usize PasswordStoreLen = PasswordSaltLen + PasswordHashLen; 
inline constexpr usize LogFileMaxLen = 31;

#pragma pack(push, 1)
struct Record {
		char username[UsernameMaxLen + 1]; 
		char password[PasswordStoreLen];   
		u64 balance;
		char logFile[LogFileMaxLen + 1]; 
		b8 isLocked;
};
#pragma pack(pop)

static_assert(sizeof(Record) == 24 + 48 + 8 + 32 + 1, "Record size mismatch");


#pragma pack(push, 1)
struct TxnRecord {
		char username[24];
		char counterparty[24];
		u64 amount;
		u64 balanceAfter;
		u64 timestamp;
		u8 type;
};
#pragma pack(pop)

namespace {

	[[nodiscard]] auto toNetwork(u64 val) noexcept -> u64 {
		if constexpr (std::endian::native == std::endian::little) {
			return std::byteswap(val);
		}
		return val;
	}

	void writeRecord(std::ofstream &f,
					 const std::string &user,
					 const std::string &pass,
					 u64 balance,
					 bool locked) {
		Record r{};
		std::memset(&r, 0, sizeof(r));

		auto logName = user + ".log";

		
		auto salt = base::generateSalt();
		auto rawHash = base::Sha256::hash(salt + ":" + pass);

		
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

	
	enum TxnType : u8 {
		deposit = 0,
		withdraw = 1,
		transfer_out = 2,
		transfer_in = 3,
	};

	struct Tx {
			std::string message;
			u64 balance;
			u8 type = 0;
			u64 amount = 0;
			std::string counterparty;
	};

	void appendDeposit(std::vector<Tx> &txs, u64 amount, u64 &balance) {
		balance += amount;
		txs.push_back(
			{"Deposit +" + std::to_string(amount) + " " + std::to_string(balance), balance,
			 deposit, amount, ""});
	}

	void appendWithdrawal(std::vector<Tx> &txs, u64 amount, u64 &balance) {
		balance -= amount;
		txs.push_back(
			{"Withdrawal -" + std::to_string(amount) + " " + std::to_string(balance), balance,
			 withdraw, amount, ""});
	}

	void appendTransferOut(std::vector<Tx> &txs, u64 amount, const std::string &to, u64 &balance) {
		balance -= amount;
		txs.push_back(
			{"Transfer to " + to + " -" + std::to_string(amount) + " " + std::to_string(balance),
			 balance, transfer_out, amount, to});
	}

	void appendTransferIn(std::vector<Tx> &txs, u64 amount, const std::string &from, u64 &balance) {
		balance += amount;
		txs.push_back({"Transfer from " + from + " +" + std::to_string(amount) + " "
						   + std::to_string(balance),
					   balance, transfer_in, amount, from});
	}

	auto formatLog(const std::string &msg, int seq) -> std::string {
		auto total = 8 * 3600 + 30 * 60 + seq * 12;
		auto h = total / 3600;
		auto m = (total % 3600) / 60;
		auto s = total % 60;
		return std::format("[2026-06-17 {:02}:{:02}:{:02}] {}", h, m, s, msg);
	}

	
	inline constexpr u64 BaseEpoch = 1781654400ULL;

	
	[[nodiscard]] auto txnTimestamp(int seq) -> u64 {
		auto total = 8 * 3600 + 30 * 60 + seq * 12; 
		return BaseEpoch + static_cast<u64>(total);
	}

	void generateAdminTxs(std::vector<Tx> &txs, u64 &finalBalance) {
		u64 bal = 999'999;

		txs.push_back({"Account created with balance " + std::to_string(bal), bal,
					   deposit, bal, ""});

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
	}

	void generateUser1Txs(std::vector<Tx> &txs, u64 &finalBalance) {
		u64 bal = 50'000;

		txs.push_back({"Account created with balance " + std::to_string(bal), bal,
					   deposit, bal, ""});

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
	}

	void writeTxnFile(const std::string &user, const std::vector<Tx> &txs) {
		auto path = std::string("data/") + user + ".txn";
		auto f = std::ofstream(path, std::ios::binary | std::ios::trunc);
		if (!f) {
			std::cerr << "  failed to write " << path << "\n";
			return;
		}
		for (std::size_t i = 0; i < txs.size(); ++i) {
			const auto &tx = txs[i];
			TxnRecord rec{};
			std::memset(&rec, 0, sizeof(rec));
			std::memcpy(rec.username, user.c_str(),
						(std::min)(user.size(), sizeof(rec.username) - 1));
			if (!tx.counterparty.empty()) {
				std::memcpy(rec.counterparty, tx.counterparty.c_str(),
							(std::min)(tx.counterparty.size(), sizeof(rec.counterparty) - 1));
			}
			rec.amount = toNetwork(tx.amount);
			rec.balanceAfter = toNetwork(tx.balance);
			rec.timestamp = toNetwork(txnTimestamp(static_cast<int>(i)));
			rec.type = tx.type;
			f.write(reinterpret_cast<const char *>(&rec), sizeof(rec));
		}
		std::cout << "  wrote " << txs.size() << " txn records to " << path << "\n";
	}

} // namespace

int main() {
	std::ofstream f("data.db", std::ios::binary);
	if (!f) {
		std::cerr << "failed to open data.db for writing\n";
		return 1;
	}

	u64 adminBalance = 0;
	u64 user1Balance = 0;

	std::vector<Tx> adminTxs;
	std::vector<Tx> user1Txs;
	generateAdminTxs(adminTxs, adminBalance);
	generateUser1Txs(user1Txs, user1Balance);

	// Build log strings from Txs
	auto buildLog = [](const std::vector<Tx> &txs) {
		std::vector<std::string> result;
		for (std::size_t i = 0; i < txs.size(); ++i) {
			result.push_back(formatLog(txs[i].message, static_cast<int>(i)));
		}
		return result;
	};
	auto adminLog = buildLog(adminTxs);
	auto user1Log = buildLog(user1Txs);

	std::cout << "Generating test data...\n";

	// Write records
	writeRecord(f, "admin", "admin123", adminBalance, false);
	writeLog("admin", adminLog);
	writeTxnFile("admin", adminTxs);

	writeRecord(f, "user1", "password1", user1Balance, false);
	writeLog("user1", user1Log);
	writeTxnFile("user1", user1Txs);

	writeRecord(f, "locked", "lockedpw", 1'000, true);
	writeLog("locked", {"[2026-06-17 08:30:00] Account created with balance 1000"});
	

	std::cout << "wrote 3 test accounts to data.db\n";
	std::cout << "  admin:   balance " << adminBalance << " (" << (adminLog.size())
			  << " log entries, " << adminTxs.size() << " txn records)\n";
	std::cout << "  user1:   balance " << user1Balance << " (" << (user1Log.size())
			  << " log entries, " << user1Txs.size() << " txn records)\n";
	std::cout << "  locked:  balance 1,000 (account locked)\n";
	return 0;
}
