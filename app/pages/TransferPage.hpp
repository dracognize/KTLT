#pragma once

<<<<<<< HEAD
#include <ftxui/component/component.hpp>
#include <string>

class Client;
namespace ftxui {
class ScreenInteractive;
}

struct TransferPage {
		TransferPage(Client &client, ftxui::ScreenInteractive &screen,
					 const std::string &username, int &page);
		ftxui::Component build();
		void reset();
=======
#include "libs/base/types.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>

class Client;
struct DashboardPage;

struct TransferPage {
		TransferPage(Client &client,
					 ftxui::ScreenInteractive &screen,
					 const std::string &username,
					 int &postAuthPage,
					 DashboardPage &dashboard);
		ftxui::Component build();
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

	private:
		void doTransfer();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
<<<<<<< HEAD
		const std::string &_username; // sender = currently logged in user
		int &_page;

		std::string _recipient;
		std::string _amountStr;
		std::string _status;
		bool _loading = false;
};
=======
		DashboardPage &_dashboard;

		const std::string &_username;
		int &_postAuthPage;

		std::string _recipient;
		ftxui::Component _recipientInput;
		std::string _amountStr;
		ftxui::Component _amountInput;
		ftxui::Component _transferBtn;
		ftxui::Component _backBtn;

		std::string _status;
		bool _loading = false;
		int _spinnerFrame = 0;
};
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
