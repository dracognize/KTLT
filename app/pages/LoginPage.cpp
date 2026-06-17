#include "app/pages/LoginPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

LoginPage::LoginPage(Client &client,
					 ftxui::ScreenInteractive &screen,
					 std::string &appUsername,
					 int &section)
	: _client(client), _screen(screen), _appUsername(appUsername), _section(section) {
}

void LoginPage::setDashboard(DashboardPage &dashboard) {
	_dashboard = &dashboard;
}

void LoginPage::doLogin() {
	if (_username.empty() || _password.empty()) {
		_status = "Please enter username and password";
		return;
	}
	_loading = true;
	_status = "";
	_client.authenticate(_username, _password, [this](bool ok) {
		_screen.Post([this, ok] {
			_loading = false;
			if (ok) {
				_appUsername = _username;
				onLogin();
			} else {
				_status = "Authentication failed";
			}
			_screen.RequestAnimationFrame();
		});
	});
}

void LoginPage::onLogin() {
	if (_dashboard)
		_dashboard->doRefresh();
	_section = 1;
}

void LoginPage::reset() {
	_username.clear();
	_password.clear();
	_status.clear();
	_loading = false;
}

ftxui::Component LoginPage::build() {

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
								  .on_enter =
									  [this] {
										  if (_username.empty()) {
											  _userInput->TakeFocus();
											  return;
										  }
										  doLogin();
									  },
							  });

	_loginBtn = ftxui::Button("Login", [this] { doLogin(); }, ftxui::ButtonOption::Border());

	auto container = ftxui::Container::Vertical({
		_userInput,
		_passInput,
		_loginBtn,
	});

	return ftxui::Renderer(container, [this] {
		return ftxui::vbox({
				   ftxui::text("Login") | ftxui::bold | ftxui::center,
				   ftxui::separator(),
				   ftxui::hbox({
					   ftxui::text(" Username: "),
					   _userInput->Render() | ftxui::flex,
				   }) | ftxui::border,
				   ftxui::hbox({
					   ftxui::text(" Password: "),
					   _passInput->Render() | ftxui::flex,
				   }) | ftxui::border,
				   ftxui::separator(),
				   _loginBtn->Render() | ftxui::center,
				   _status.empty() ? ftxui::emptyElement() : ftxui::text(_status) | ftxui::center,
			   })
			   | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24) | ftxui::center;
	});
}
