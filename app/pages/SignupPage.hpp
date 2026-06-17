#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <string>

class Client;
struct DashboardPage;
namespace ftxui {
	class ScreenInteractive;
}

struct SignupPage {
		SignupPage(Client &client,
				   ftxui::ScreenInteractive &screen,
				   std::string &appUsername,
				   int &page);
		ftxui::Component build();
		void setDashboard(DashboardPage &dashboard);

	private:
		void doCreateAccount();
		void onSuccess();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		DashboardPage *_dashboard = nullptr;

		std::string &_appUsername;
		int &_page;

		std::string _username;
		ftxui::Component _userInput;

		std::string _password;
		ftxui::Component _passInput;

		std::string _confirmPassword;
		ftxui::Component _confirmInput;

		ftxui::Component _createBtn;

		std::string _status;
		bool _loading = false;
};
