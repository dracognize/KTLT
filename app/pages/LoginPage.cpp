#include "app/pages/LoginPage.hpp"
#include "app/client/Client.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

<<<<<<< HEAD
LoginPage::LoginPage(Client &client, ftxui::ScreenInteractive &screen,
					std::string &appUsername, int &page)
	: _client(client), _screen(screen), _appUsername(appUsername), _page(page) {
=======
LoginPage::LoginPage(Client &client,
                     ftxui::ScreenInteractive &screen,
                     std::string &appUsername,
                     int &section)
    : _client(client), _screen(screen), _appUsername(appUsername), _section(section) {
}

void LoginPage::setDashboard(DashboardPage &dashboard) {
    _dashboard = &dashboard;
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
}

void LoginPage::doLogin() {
    if (_username.empty() || _password.empty()) {
        _status = "Please enter username and password";
        return;
    }
    if (_username.size() > 23) {
        _status = "Username too long (max 23)";
        return;
    }
    if (_password.size() > 23) {
        _status = "Password too long (max 23)";
        return;
    }
    _loading = true;
    _status = "";
    _spinnerFrame = 0;
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
<<<<<<< HEAD
	if (onEnterDashboard)
		onEnterDashboard();
	_page = 1;
=======
    if (_dashboard)
        _dashboard->doRefresh();
    _section = 1;
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
}

void LoginPage::reset() {
    _username.clear();
    _password.clear();
    _status.clear();
    _loading = false;
    _pendingFocus = true;
}

void LoginPage::setStatus(std::string status) {
    _status = std::move(status);
}

ftxui::Component LoginPage::build() {
	auto passwordOpt = ftxui::InputOption{};
	passwordOpt.password = true;
	passwordOpt.multiline = false;
	passwordOpt.on_enter = [this] { doLogin(); };

<<<<<<< HEAD
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
=======
    _userInput = ftxui::Input(&_username,
                              "username",
                              ftxui::InputOption{
                                  .multiline = false,
                                  .on_change = [this] {
                                      if (_username.size() > 23)
                                          _username.resize(23);
                                  },
                                  .on_enter = [this] { _passInput->TakeFocus(); },
                              });

    _passInput = ftxui::Input(&_password,
                              "password",
                              ftxui::InputOption{
                                  .password = true,
                                  .multiline = false,
                                  .on_change = [this] {
                                      if (_password.size() > 23)
                                          _password.resize(23);
                                  },
                                  .on_enter = [this] {
                                      if (_username.empty()) {
                                          _userInput->TakeFocus();
                                          return;
                                      }
                                      doLogin();
                                  },
                              });

    _loginBtn = ftxui::Button("  Login  ", [this] { doLogin(); },
                              ftxui::ButtonOption::Border());

    auto container = ftxui::Container::Vertical({
        _userInput,
        _passInput,
        ftxui::Container::Horizontal({
            _loginBtn,
        }),
    });

    return ftxui::Renderer(container, [this] {
        // Focus the username input on first render (entering the page).
        if (_pendingFocus) {
            _pendingFocus = false;
            _screen.Post([this] { _userInput->TakeFocus(); });
        }

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::text(" Authenticating...") | ftxui::dim;
            }
            if (!_status.empty()) {
                return ftxui::text(_status) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

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
                   statusEl,
                   ftxui::text(""),
                   ftxui::text("Ctrl+Left/Right to switch  ·  Tab between fields  ·  Enter to submit") | ftxui::dim | ftxui::center,
               })
               | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 30)
               | ftxui::center
               | ftxui::border;
    });
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
}
