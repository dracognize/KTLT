#pragma once

#include "libs/base/string.hpp"
#include "libs/base/types.hpp"
#include "libs/base/vector.hpp"

#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>

class Client;

struct DashboardPage {
	public:
		DashboardPage(Client &client,
					  ftxui::ScreenInteractive &screen,
					  const std::string &username);
		ftxui::Component build();
		void doRefresh();

		struct LogEntry {
				base::String time;
				base::String operation;
				base::String amount;
				u64 balance;
		};

		static void parseTransactionHistory(const std::string &raw,
											base::Vector<LogEntry> &parsed);

		const std::string& balanceStr() const { return _balanceStr; }

	private:

		void doPing();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		const std::string &_username;
		std::string _balanceStr;
		std::string _status;

		base::Vector<u64> _balanceHistory;
		base::Vector<LogEntry> _parsedLogs;
		std::string _logsStatus;

		std::string _latencyStr = "?";
		long long _latencyVal = -1;

		std::chrono::steady_clock::time_point _pingSentAt;

		int _spinnerFrame = 0;
};
