#include "app/pages/AccountSettingsPage.hpp"
#include "app/client/Client.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

AccountSettingsPage::AccountSettingsPage(Client &client, ftxui::ScreenInteractive &screen,
										  const std::string &username, int &page)
	: _client(client), _screen(screen), _username(username), _page(page) {
}

void AccountSettingsPage::doChangePassword() {
	if (_newPassword.empty()) {
		_status = "Please enter a new password";
		return;
	}
	if (_newPassword != _confirmPassword) {
		_status = "Passwords do not match";
		return;
	}

	_loading = true;
	_status = "";
	_client.changePassword(_username, _newPassword, [this](bool ok) {
		_screen.Post([this, ok] {
			_loading = false;
			_status = ok ? "Password changed successfully" : "Failed to change password";
			if (ok) {
				_newPassword.clear();
				_confirmPassword.clear();
			}
			_screen.RequestAnimationFrame();
		});
	});
}

void AccountSettingsPage::doToggleLock() {
	_loading = true;
	_status = "";
	_client.toggleAccount(_username, [this](bool ok) {
		_screen.Post([this, ok] {
			_loading = false;
			_status = ok ? "Account lock status toggled" : "Failed to toggle account";
			_screen.RequestAnimationFrame();
		});
	});
}

void AccountSettingsPage::reset() {
	_newPassword.clear();
	_confirmPassword.clear();
	_status.clear();
	_loading = false;
}

ftxui::Component AccountSettingsPage::build() {
	auto newPassOpt = ftxui::InputOption{};
	newPassOpt.password = true;
	newPassOpt.multiline = false;

	auto confirmOpt = ftxui::InputOption{};
	confirmOpt.password = true;
	confirmOpt.multiline = false;
	confirmOpt.on_enter = [this] { doChangePassword(); };

	auto newPassInput = ftxui::Input(&_newPassword, "new password", newPassOpt);
	auto confirmInput = ftxui::Input(&_confirmPassword, "confirm new password", confirmOpt);
	auto changePassBtn = ftxui::Button("Change Password", [this] { doChangePassword(); });
	auto toggleLockBtn = ftxui::Button("Lock/Unlock My Account", [this] { doToggleLock(); });
	auto backBtn = ftxui::Button("Back", [this] { _page = 1; });

	auto container = ftxui::Container::Vertical({
		newPassInput,
		confirmInput,
		changePassBtn,
		toggleLockBtn,
		backBtn,
	});

	return ftxui::Renderer(container, [this, newPassInput, confirmInput, changePassBtn, toggleLockBtn, backBtn] {
		return ftxui::vbox({
				   ftxui::text("Account Settings") | ftxui::bold | ftxui::center,
				   ftxui::text(" User: " + _username),
				   ftxui::separator(),
				   ftxui::hbox({
					   ftxui::text(" New password: "),
					   newPassInput->Render() | ftxui::flex,
				   }),
				   ftxui::hbox({
					   ftxui::text(" Confirm:      "),
					   confirmInput->Render() | ftxui::flex,
				   }),
				   changePassBtn->Render() | ftxui::center,
				   ftxui::separator(),
				   toggleLockBtn->Render() | ftxui::center,
				   ftxui::separator(),
				   backBtn->Render() | ftxui::center,
				   ftxui::text(_status) | ftxui::center,
			   })
			   | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32);
	});
}