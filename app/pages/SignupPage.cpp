#include "app/pages/SignupPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

SignupPage::SignupPage(Client &client,
					   ftxui::ScreenInteractive &screen,
					   std::string &appUsername,
					   int &page)
	: _client(client), _screen(screen), _appUsername(appUsername), _page(page) {
}

void SignupPage::setDashboard(DashboardPage &dashboard) {
	_dashboard = &dashboard;
}

void SignupPage::doCreateAccount() {
	if (_username.empty() || _password.empty() || _confirmPassword.empty()) {
		_status = "Please fill in all fields";
		return;
	}
	if (_password != _confirmPassword) {
		_status = "Passwords do not match";
		return;
	}
	if (_username.size() > 23) {
		_status = "Username must be at most 23 characters";
		return;
	}
	if (_password.size() > 23) {
		_status = "Password must be at most 23 characters";
		return;
	}
	_loading = true;
	_status.clear();
	_client.createAccount(_username, _password, [this](bool ok) {
		_screen.Post([this, ok] {
			_loading = false;
			if (ok) {
				_appUsername = _username;
				onSuccess();
			} else {
				_status = "Account creation failed (username may exist)";
			}
			_screen.RequestAnimationFrame();
		});
	});
}

void SignupPage::onSuccess() {
	if (_dashboard)
		_dashboard->doRefresh();
	_page = 1;
}

ftxui::Component SignupPage::build() {
	_userInput = ftxui::Input(&_username,
							  "username",
							  ftxui::InputOption{
								  .multiline = false,
								  .on_enter = [this] { _passInput->TakeFocus(); },
							  });

	_passInput = ftxui::Input(&_password,
							  "password",
							  ftxui::InputOption{
								  .password = true,
								  .multiline = false,
								  .on_enter = [this] { _confirmInput->TakeFocus(); },
							  });

	_confirmInput = ftxui::Input(&_confirmPassword,
								 "confirm password",
								 ftxui::InputOption{
									 .password = true,
									 .multiline = false,
									 .on_enter = [this] { doCreateAccount(); },
								 });

	_createBtn = ftxui::Button("Create Account", [this] { doCreateAccount(); });

	auto container = ftxui::Container::Vertical({
		_userInput,
		_passInput,
		_confirmInput,
		ftxui::Container::Horizontal({
			_createBtn,
		}),
	});

	return ftxui::Renderer(container, [this] {
		return ftxui::vbox({
				   ftxui::text("Create Account") | ftxui::bold | ftxui::center,
				   ftxui::separator(),
				   ftxui::hbox({
					   ftxui::text(" Username: "),
					   _userInput->Render() | ftxui::flex,
				   }) | ftxui::border,
				   ftxui::hbox({
					   ftxui::text(" Password: "),
					   _passInput->Render() | ftxui::flex,
				   }) | ftxui::border,
				   ftxui::hbox({
					   ftxui::text(" Confirm:  "),
					   _confirmInput->Render() | ftxui::flex,
				   }) | ftxui::border,
				   ftxui::separator(),
				   ftxui::hbox({
					   _createBtn->Render(),
				   }) | ftxui::center,
				   _status.empty() ? ftxui::emptyElement() : ftxui::text(_status) | ftxui::center,
			   })
			   | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 28) | ftxui::center;
	});
}
