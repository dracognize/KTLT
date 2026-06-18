#pragma once

#include "libs/base/types.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <functional>
#include <string>

class Client;
struct DashboardPage;

struct SettingPage {
		SettingPage(Client &client,
					ftxui::ScreenInteractive &screen,
					const std::string &username,
					int &postAuthPage,
					std::function<void()> onLogout,
					DashboardPage &dashboard);
		ftxui::Component build();

	private:
		void doChangePassword();
		void onLogout();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		DashboardPage &_dashboard;

		const std::string &_username;
		int &_postAuthPage;
		std::function<void()> _onLogout;

		std::string _currentPassword;
		ftxui::Component _currentPassInput;
		std::string _newPassword;
		ftxui::Component _newPassInput;
		std::string _confirmPassword;
		ftxui::Component _confirmInput;
		ftxui::Component _changeBtn;

		void doLockAccount();

		ftxui::Component _logoutBtn;
		ftxui::Component _lockBtn;

		ftxui::Component _logoutConfirmYes;
		ftxui::Component _lockConfirmYes;

		std::string _status;
		bool _showLockConfirm = false;
		bool _loading = false;
		int _spinnerFrame = 0;
		bool _showLogoutConfirm = false;
};
