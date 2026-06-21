#pragma once

#include "app/pages/DashboardPage.hpp"
#include "libs/base/string.hpp"
#include "libs/base/types.hpp"
#include "libs/base/vector.hpp"

#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>

class Client;

	struct HistoryPage {
		HistoryPage(Client &client,
					ftxui::ScreenInteractive &screen,
					const std::string &username,
					DashboardPage &dashboard);
		ftxui::Component build();
		void doRefresh();

		enum SortColumn { Time = 0, Operation = 1, Amount = 2, Balance = 3 };

	private:
		Client &_client;
		ftxui::ScreenInteractive &_screen;
		const std::string &_username;
		DashboardPage &_dashboard;

		base::Vector<DashboardPage::LogEntry> _entries;
		std::string _status;
		bool _loading = false;
		bool _pendingRefresh = false;
		int _spinnerFrame = 0;

		
		SortColumn _sortColumn = SortColumn::Time;
		bool _sortAscending = true;
		std::string _searchStr;
		ftxui::Component _searchInput;
		ftxui::Component _tableContainer;

		std::chrono::steady_clock::time_point _lastRefreshTime;
};
