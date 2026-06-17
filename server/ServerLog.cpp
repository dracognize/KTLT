#include "server/ServerLog.hpp"

#include <chrono>
#include <format>
#include <iostream>

namespace {

	void log(const std::string &level, const std::string &msg) {
		auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
		std::cout << std::format("[{:%T}] [{}] {}\n", now, level, msg);
	}

} // namespace

void ServerLog::info(const std::string &msg) {
	log("INFO", msg);
}
void ServerLog::warn(const std::string &msg) {
	log("WARN", msg);
}
void ServerLog::auth(const std::string &msg) {
	log("AUTH", msg);
}
void ServerLog::create(const std::string &msg) {
	log("CREATE", msg);
}
void ServerLog::balance(const std::string &msg) {
	log("BALANCE", msg);
}
void ServerLog::transfer(const std::string &msg) {
	log("TRANSFER", msg);
}
void ServerLog::password(const std::string &msg) {
	log("PASSWORD", msg);
}
void ServerLog::toggle(const std::string &msg) {
	log("TOGGLE", msg);
}
