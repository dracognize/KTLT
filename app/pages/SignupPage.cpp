#include "app/pages/SignupPage.hpp"
#include "app/Theme.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>


static int evaluateStrength(const std::string &pw) {
    auto len = pw.size();
    if (len == 0) return 0;
    
    int score = 0;
    if (len >= 8) score++;
    if (len >= 12) score++;
    
    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;
    bool hasSpecial = false;
    
    for (char c : pw) {
        if (std::isupper(c)) hasUpper = true;
        else if (std::islower(c)) hasLower = true;
        else if (std::isdigit(c)) hasDigit = true;
        else hasSpecial = true;
    }
    
    if (hasUpper && hasLower) score++;
    if (hasDigit) score++;
    if (hasSpecial) score++;
    
    if (score < 2) return 0; 
    if (score < 4) return 1; 
    return 2; 
}

static const char* strengthLabel(int s) {
    switch (s) {
        case 0: return " WEAK ";
        case 1: return " MEDIUM ";
        case 2: return " STRONG ";
        default: return " ? ";
    }
}

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
    _spinnerFrame = 0;
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
    auto input_option = ftxui::InputOption::Default();

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
            _confirmInput->TakeFocus();
            return true;
        }
        if (_password.size() > 23) _password.resize(23);
        _cachedStrength = evaluateStrength(_password);
        return false;
    });

    _confirmInput = ftxui::Input(&_confirmPassword, "confirm password", pass_option);
    _confirmInput |= ftxui::CatchEvent([this](ftxui::Event e) {
        if (e == ftxui::Event::Return) {
            doCreateAccount();
            return true;
        }
        if (_confirmPassword.size() > 23) _confirmPassword.resize(23);
        return false;
    });

    _createBtn = ftxui::Button(" CREATE ACCOUNT ", [this] { doCreateAccount(); }, theme::Button(theme::Mauve));

    auto container = ftxui::Container::Vertical({
        _userInput,
        _passInput,
        _confirmInput,
        _createBtn,
    });

    return ftxui::Renderer(container, [this]() -> ftxui::Element {
        auto strengthColor = theme::Red;
        auto strengthBg = theme::Surface1;
        if (_cachedStrength == 1) { strengthColor = theme::Base; strengthBg = theme::Yellow; }
        if (_cachedStrength == 2) { strengthColor = theme::Base; strengthBg = theme::Green; }
        if (_cachedStrength == 0 && !_password.empty()) { strengthColor = theme::Base; strengthBg = theme::Red; }

        auto pwStrengthEl = _password.empty()
                                ? ftxui::emptyElement()
                                : ftxui::hbox({
                                      ftxui::text(" Strength: ") | ftxui::color(theme::Subtext0),
                                      ftxui::text(strengthLabel(_cachedStrength)) | ftxui::bold | ftxui::color(strengthColor) | ftxui::bgcolor(strengthBg),
                                      ftxui::filler(),
                                  });

        auto matchEl = ftxui::emptyElement();
        if (!_confirmPassword.empty()) {
            if (_password == _confirmPassword) {
                matchEl = ftxui::text(" Passwords match ") | ftxui::bold | ftxui::color(theme::Green);
            } else {
                matchEl = ftxui::text(" Passwords do not match ") | ftxui::bold | ftxui::color(theme::Red);
            }
        }

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::hbox({
                    ftxui::spinner(21, _spinnerFrame) | ftxui::color(theme::Mauve),
                    ftxui::text(" Creating account..."),
                }) | ftxui::center;
            }
            if (!_status.empty()) {
                return ftxui::text(_status) | ftxui::color(theme::Red) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

        auto signupBox = ftxui::vbox({
            ftxui::text(" SIGN UP ") | ftxui::bold | ftxui::center | ftxui::color(theme::Mauve),
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
            pwStrengthEl,
            ftxui::hbox({
                ftxui::text(" CONFIRM  ") | ftxui::color(theme::Subtext0) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
                _confirmInput->Render() | ftxui::flex,
            }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
            matchEl | ftxui::center,
            ftxui::text(""),
            _createBtn->Render() | ftxui::center,
            ftxui::text(""),
            statusEl,
        }) | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40)
           | ftxui::center
           | ftxui::borderStyled(ftxui::ROUNDED)
           | ftxui::color(theme::Surface0);

        return ftxui::vbox({
            ftxui::filler(),
            signupBox,
            ftxui::filler(),
            ftxui::text("Already have an account? Go back to Login") | ftxui::dim | ftxui::center,
        });
    });
}
