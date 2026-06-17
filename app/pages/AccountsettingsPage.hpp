#pragma once

#include <ftxui/component/component.hpp>
#include <string>

class Client;
namespace ftxui {
class ScreenInteractive;
}

struct AccountSettingsPage {
		AccountSettingsPage(Client &client, ftxui::ScreenInteractive &screen,
							 const std::string &username, int &page);
		ftxui::Component build();
		void reset();

	private:
		void doChangePassword();
		void doToggleLock();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		const std::string &_username;
		int &_page;

		std::string _newPassword;
		std::string _confirmPassword;
		std::string _status;
		bool _loading = false;
};