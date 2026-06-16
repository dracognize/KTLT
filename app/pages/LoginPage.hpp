#pragma once

#include <ftxui/component/component.hpp>
#include <functional>
#include <string>

class Client;
namespace ftxui {
class ScreenInteractive;
}

struct LoginPage {
		LoginPage(Client &client, ftxui::ScreenInteractive &screen,
				  std::string &appUsername, int &page);
		ftxui::Component build();
		void reset();

		std::function<void()> onEnterDashboard;

	private:
		void doLogin();
		void onLogin();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		std::string &_appUsername;
		int &_page;
		std::string _username;
		std::string _password;
		std::string _status;
		bool _loading = false;
};
