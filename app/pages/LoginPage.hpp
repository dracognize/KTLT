#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>

class Client;
struct DashboardPage;

struct LoginPage {
		LoginPage(Client &client, ftxui::ScreenInteractive &screen,
				  std::string &appUsername, int &section);
		ftxui::Component build();
		void reset();
		void setStatus(std::string status);
		void setDashboard(DashboardPage &dashboard);

	private:
		void doLogin();
		void onLogin();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		std::string &_appUsername;
		int &_section;
		DashboardPage *_dashboard = nullptr;
		std::string _username;
		std::string _password;
		std::string _status;
		bool _loading = false;
		bool _pendingFocus = true;
		int _spinnerFrame = 0;
		ftxui::Component _userInput;
		ftxui::Component _passInput;
		ftxui::Component _loginBtn;
};
