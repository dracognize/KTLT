#pragma once

#include "app/pages/DashboardPage.hpp"
#include "libs/base/types.hpp"

#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <vector>

class Client;

	struct HistoryPage {
		HistoryPage(Client &client,
					ftxui::ScreenInteractive &screen,
					const std::string &username,
					DashboardPage &dashboard);
		ftxui::Component build();
		void doRefresh();

	private:
		Client &_client;
		ftxui::ScreenInteractive &_screen;
		const std::string &_username;
		DashboardPage &_dashboard;

		std::vector<DashboardPage::LogEntry> _entries;
		std::string _status;
		bool _loading = false;
		bool _pendingRefresh = false;
		int _spinnerFrame = 0;

		std::chrono::steady_clock::time_point _lastRefreshTime;
};
