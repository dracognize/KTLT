#include "app/pages/LoginPage.hpp"
#include "app/Theme.hpp"
#include "app/pages/DashboardPage.hpp"
#include "app/client/Client.hpp"

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
    if (_dashboard)
        _dashboard->doRefresh();
    _section = 1;
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
    auto input_option = ftxui::InputOption::Default();
    // Removed unsupported focused_color/cursor_color

    _userInput = ftxui::Input(&_username, "username", input_option);
    _userInput |= ftxui::CatchEvent([this](ftxui::Event e) {
        if (e == ftxui::Event::Return) {
            _passInput->TakeFocus();
            return true;
        }
        if (_username.size() > 23) _username.resize(23);
        return false;
    });

    auto pass_option = input_option;
    pass_option.password = true;
    _passInput = ftxui::Input(&_password, "password", pass_option);
    _passInput |= ftxui::CatchEvent([this](ftxui::Event e) {
        if (e == ftxui::Event::Return) {
            if (_username.empty()) {
                _userInput->TakeFocus();
            } else {
                doLogin();
            }
            return true;
        }
        if (_password.size() > 23) _password.resize(23);
        return false;
    });

    _loginBtn = ftxui::Button(" LOGIN ", [this] { doLogin(); }, theme::Button(theme::Mauve));

    auto container = ftxui::Container::Vertical({
        _userInput,
        _passInput,
        _loginBtn,
    });

    return ftxui::Renderer(container, [this]() -> ftxui::Element {
        if (_pendingFocus) {
            _pendingFocus = false;
            _screen.Post([this] { _userInput->TakeFocus(); });
        }

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::hbox({
                    ftxui::spinner(21, _spinnerFrame) | ftxui::color(theme::Mauve),
                    ftxui::text(" Authenticating..."),
                }) | ftxui::center;
            }
            if (!_status.empty()) {
                return ftxui::text(_status) | ftxui::color(theme::Red) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

        auto loginBox = ftxui::vbox({
            ftxui::text(" LOGIN ") | ftxui::bold | ftxui::center | ftxui::color(theme::Mauve),
            ftxui::separator() | ftxui::color(theme::Mauve),
            ftxui::text(""),
            ftxui::hbox({
                ftxui::text(" USERNAME ") | ftxui::color(theme::Subtext0) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
                _userInput->Render() | ftxui::flex,
            }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
            ftxui::hbox({
                ftxui::text(" PASSWORD ") | ftxui::color(theme::Subtext0) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
                _passInput->Render() | ftxui::flex,
            }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
            ftxui::text(""),
            _loginBtn->Render() | ftxui::center,
            ftxui::text(""),
            statusEl,
        }) | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40)
           | ftxui::center
           | ftxui::borderStyled(ftxui::ROUNDED)
           | ftxui::color(theme::Surface0);

        return ftxui::vbox({
            ftxui::filler(),
            loginBox,
            ftxui::filler(),
            ftxui::text("Tab to switch · Enter to submit") | ftxui::dim | ftxui::center,
        });
    });
}
