#pragma once

#include <string>

struct ServerLog {
		static void info(const std::string &msg);
		static void warn(const std::string &msg);
		static void auth(const std::string &msg);
		static void create(const std::string &msg);
		static void balance(const std::string &msg);
		static void transfer(const std::string &msg);
		static void password(const std::string &msg);
		static void toggle(const std::string &msg);
};
