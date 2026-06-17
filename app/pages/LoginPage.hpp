#pragma once

#include <ftxui/component/component.hpp>
<<<<<<< HEAD
#include <functional>
#include <string>

class Client;
namespace ftxui {
class ScreenInteractive;
}
=======
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>

class Client;
struct DashboardPage;
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

struct LoginPage {
		LoginPage(Client &client, ftxui::ScreenInteractive &screen,
				  std::string &appUsername, int &page);
		ftxui::Component build();
		void reset();
<<<<<<< HEAD

		std::function<void()> onEnterDashboard;
=======
		void setStatus(std::string status);
		void setDashboard(DashboardPage &dashboard);
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

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
		bool _pendingFocus = true;
		int _spinnerFrame = 0;
};
