#include "app/pages/RegisterPage.hpp"
#include "app/client/Client.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

RegisterPage::RegisterPage(Client &client, ftxui::ScreenInteractive &screen, int &page)
	: _client(client), _screen(screen), _page(page) {
}

void RegisterPage::doRegister() {
	if (_username.empty() || _password.empty()) {
		_status = "Please enter username and password";
		return;
	}
	if (_password != _confirmPassword) {
		_status = "Passwords do not match";
		return;
	}

	_loading = true;
	_status = "";
	_client.createAccount(_username, _password, [this](bool ok) {
		_screen.Post([this, ok] {
			_loading = false;
			if (ok) {
				_status = "Account created. You can log in now.";
				_username.clear();
				_password.clear();
				_confirmPassword.clear();
				_page = 0; // back to Login
			} else {
				_status = "Registration failed";
			}
			_screen.RequestAnimationFrame();
		});
	});
}

void RegisterPage::reset() {
	_username.clear();
	_password.clear();
	_confirmPassword.clear();
	_status.clear();
	_loading = false;
}

ftxui::Component RegisterPage::build() {
	auto passwordOpt = ftxui::InputOption{};
	passwordOpt.password = true;
	passwordOpt.multiline = false;

	auto confirmOpt = ftxui::InputOption{};
	confirmOpt.password = true;
	confirmOpt.multiline = false;
	confirmOpt.on_enter = [this] { doRegister(); };

	auto userInput = ftxui::Input(&_username, "username");
	auto passInput = ftxui::Input(&_password, "password", passwordOpt);
	auto confirmInput = ftxui::Input(&_confirmPassword, "confirm password", confirmOpt);
	auto registerBtn = ftxui::Button("Register", [this] { doRegister(); });
	auto backBtn = ftxui::Button("Back to Login", [this] { _page = 0; });

	auto container = ftxui::Container::Vertical({
		userInput,
		passInput,
		confirmInput,
		registerBtn,
		backBtn,
	});

	return ftxui::Renderer(container, [this, userInput, passInput, confirmInput, registerBtn, backBtn] {
		return ftxui::vbox({
				   ftxui::text("Register") | ftxui::bold | ftxui::center,
				   ftxui::separator(),
				   ftxui::hbox({
					   ftxui::text(" Username: "),
					   userInput->Render() | ftxui::flex,
				   }),
				   ftxui::hbox({
					   ftxui::text(" Password: "),
					   passInput->Render() | ftxui::flex,
				   }),
				   ftxui::hbox({
					   ftxui::text(" Confirm:  "),
					   confirmInput->Render() | ftxui::flex,
				   }),
				   ftxui::separator(),
				   registerBtn->Render() | ftxui::center,
				   backBtn->Render() | ftxui::center,
				   ftxui::text(_status) | ftxui::center,
			   })
			   | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 28);
	});
}