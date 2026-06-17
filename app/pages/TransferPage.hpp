#pragma once

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

	private:
		void doTransfer();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		const std::string &_username; // sender = currently logged in user
		int &_page;

		std::string _recipient;
		std::string _amountStr;
		std::string _status;
		bool _loading = false;
};