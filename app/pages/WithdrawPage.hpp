#pragma once

#include "libs/base/types.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>

class Client;
struct DashboardPage;

struct WithdrawPage {
		WithdrawPage(Client &client,
					 ftxui::ScreenInteractive &screen,
					 const std::string &username,
					 int &postAuthPage,
					 DashboardPage &dashboard);
		ftxui::Component build();

	private:
		void doWithdraw();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		DashboardPage &_dashboard;

		const std::string &_username;
		int &_postAuthPage;

		std::string _amountStr;
		ftxui::Component _amountInput;
		ftxui::Component _withdrawBtn;
		ftxui::Component _backBtn;

		std::string _status;
		bool _loading = false;
		int _spinnerFrame = 0;
		bool _showConfirm = false;
};
