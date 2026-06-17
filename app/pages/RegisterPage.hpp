#pragma once

#include <ftxui/component/component.hpp>
#include <string>

class Client;
namespace ftxui {
class ScreenInteractive;
}

struct RegisterPage {
		RegisterPage(Client &client, ftxui::ScreenInteractive &screen, int &page);
		ftxui::Component build();
		void reset();

	private:
		void doRegister();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		int &_page;
		std::string _username;
		std::string _password;
		std::string _confirmPassword;
		std::string _status;
		bool _loading = false;
};