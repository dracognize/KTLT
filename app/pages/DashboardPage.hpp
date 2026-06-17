#pragma once

#include <ftxui/component/component.hpp>
#include <string>

class Client;
struct LoginPage;
namespace ftxui {
	class ScreenInteractive;
}

struct DashboardPage {
		DashboardPage(Client &client,
					  ftxui::ScreenInteractive &screen,
					  const std::string &username,
					  int &page,
					  LoginPage &loginPage);
		ftxui::Component build();
		void doRefresh();

	private:
		void onLogout();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		const std::string &_username;
		int &_page;
		LoginPage &_loginPage;
		std::string _balanceStr;
		std::string _status;
};
