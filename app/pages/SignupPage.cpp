#include "app/pages/SignupPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

// ── Inline password strength helpers ─────────────────────────────────
static int evaluateStrength(const std::string &pw) {
    auto len = pw.size();
    if (len == 0) return 0;
    if (len < 6) return 0; // Weak
    if (len < 12) return 1; // Medium
    return 2; // Strong
}

static const char* strengthLabel(int s) {
    switch (s) {
        case 0: return "Weak";
        case 1: return "Medium";
        case 2: return "Strong";
        default: return "?";
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
                                      _cachedStrength = evaluateStrength(_password);
                                  },
                                  .on_enter = [this] { _confirmInput->TakeFocus(); },
                              });

    _confirmInput = ftxui::Input(&_confirmPassword,
                                 "confirm password",
                                 ftxui::InputOption{
                                     .password = true,
                                     .multiline = false,
                                     .on_change = [this] {
                                         if (_confirmPassword.size() > 23)
                                             _confirmPassword.resize(23);
                                     },
                                     .on_enter = [this] { doCreateAccount(); },
                                 });

    _createBtn = ftxui::Button("  Create Account  ", [this] { doCreateAccount(); },
                               ftxui::ButtonOption::Border());

    auto container = ftxui::Container::Vertical({
        _userInput,
        _passInput,
        _confirmInput,
        ftxui::Container::Horizontal({
            _createBtn,
        }),
    });

    return ftxui::Renderer(container, [this] {
        // Password strength indicator (only when password is non-empty)
        auto pwStrengthEl = _password.empty()
                                ? ftxui::emptyElement()
                                : ftxui::hbox({
                                      ftxui::text(" Password strength: ") | ftxui::dim,
                                      ftxui::text(strengthLabel(_cachedStrength)) | ftxui::bold,
                                      ftxui::filler(),
                                  });

        // Confirmation match indicator
        auto matchEl = ftxui::emptyElement();
        if (!_confirmPassword.empty()) {
            if (_password == _confirmPassword) {
                matchEl = ftxui::text(" Passwords match") | ftxui::bold;
            } else {
                matchEl = ftxui::text(" Passwords do not match") | ftxui::bold;
            }
        }

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::text(" Creating account...") | ftxui::dim;
            }
            if (!_status.empty()) {
                return ftxui::text(_status) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

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
                   pwStrengthEl,
                   ftxui::hbox({
                       ftxui::text(" Confirm: "),
                       _confirmInput->Render() | ftxui::flex,
                   }) | ftxui::border,
                   matchEl | ftxui::center,
                   ftxui::separator(),
                   _createBtn->Render() | ftxui::center,
                   statusEl,
                   ftxui::text(""),
                   ftxui::text("Already have an account? Go to Login") | ftxui::dim | ftxui::center,
               })
               | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 30)
               | ftxui::center
               | ftxui::border;
    });
}
