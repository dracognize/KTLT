#include "app/pages/LoginPage.hpp"
#include "app/client/Client.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

LoginPage::LoginPage(Client &client, ftxui::ScreenInteractive &screen,
					std::string &appUsername, int &page)
	: _client(client), _screen(screen), _appUsername(appUsername), _page(page) {
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
	if (onEnterDashboard)
		onEnterDashboard();
	_page = 1;
}

void LoginPage::reset() {
	_username.clear();
	_password.clear();
	_status.clear();
	_loading = false;
}

ftxui::Component LoginPage::build() {
	auto passwordOpt = ftxui::InputOption{};
	passwordOpt.password = true;
	passwordOpt.multiline = false;
	passwordOpt.on_enter = [this] { doLogin(); };

	auto userInput = ftxui::Input(&_username, "username");
	auto passInput = ftxui::Input(&_password, "password", passwordOpt);
	auto loginBtn = ftxui::Button("Login", [this] { doLogin(); });
	auto registerBtn = ftxui::Button("Register", [this] { _page = 2; });
	
	auto container = ftxui::Container::Vertical({
		userInput,
		passInput,
		loginBtn,
		registerBtn,
	});

	return ftxui::Renderer(container, [this, userInput, passInput, loginBtn] {
		return ftxui::vbox({
				   ftxui::text("Login") | ftxui::bold | ftxui::center,
				   ftxui::separator(),
				   ftxui::hbox({
					   ftxui::text(" Username: "),
					   userInput->Render() | ftxui::flex,
				   }),
				   ftxui::hbox({
					   ftxui::text(" Password: "),
					   passInput->Render() | ftxui::flex,
				   }),
				   ftxui::separator(),
				   loginBtn->Render() | ftxui::center,
				   ftxui::text(_status) | ftxui::center,
			   })
			   | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 24);
	});
}
