#pragma once

#include "libs/base/types.hpp"

#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <vector>

class Client;

struct DashboardPage {
	public:
		DashboardPage(Client &client,
					  ftxui::ScreenInteractive &screen,
					  const std::string &username);
		ftxui::Component build();
		void doRefresh();

		struct LogEntry {
				std::string time;
				std::string operation;
				std::string amount;
				u64 balance;
		};

		static void parseRawLogs(const std::string &raw,
								 std::vector<LogEntry> &parsed,
								 std::vector<u64> &history,
								 usize maxEntries = 20);

		const std::string& balanceStr() const { return _balanceStr; }

	private:
<<<<<<< HEAD
		void onLogout();
		void onExit();
=======

		void doPing();
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		const std::string &_username;
		std::string _balanceStr;
		std::string _status;

		std::vector<u64> _balanceHistory;
		std::vector<LogEntry> _parsedLogs;
		std::string _logsStatus;

		std::string _latencyStr = "?";
		std::chrono::steady_clock::time_point _pingSentAt;

		int _spinnerFrame = 0;
};
