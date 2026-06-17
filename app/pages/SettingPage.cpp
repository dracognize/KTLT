#include "app/pages/SettingPage.hpp"
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

SettingPage::SettingPage(Client &client,
                         ftxui::ScreenInteractive &screen,
                         const std::string &username,
                         int &postAuthPage,
                         std::function<void()> onLogout,
                         DashboardPage &dashboard)
    : _client(client), _screen(screen), _username(username), _postAuthPage(postAuthPage),
      _onLogout(std::move(onLogout)), _dashboard(dashboard) {
}

void SettingPage::onLogout() {
    _showLogoutConfirm = true;
}

void SettingPage::doChangePassword() {
    if (_currentPassword.empty() || _newPassword.empty() || _confirmPassword.empty()) {
        _status = "Please fill in all fields";
        return;
    }
    if (_newPassword != _confirmPassword) {
        _status = "New passwords do not match";
        return;
    }
    if (_newPassword.size() > 23) {
        _status = "Password must be at most 23 characters";
        return;
    }
    _loading = true;
    _status.clear();
    _spinnerFrame = 0;
    _client.authenticate(_username, _currentPassword, [this](bool authOk) {
        _screen.Post([this, authOk] {
            if (!authOk) {
                _loading = false;
                _status = "Current password is incorrect";
                _screen.RequestAnimationFrame();
                return;
            }
            _client.changePassword(_username, _newPassword, [this](bool changed) {
                _screen.Post([this, changed] {
                    _loading = false;
                    if (changed) {
                        _status = "Password changed successfully!";
                        _currentPassword.clear();
                        _newPassword.clear();
                        _confirmPassword.clear();
                    } else {
                        _status = "Failed to change password";
                    }
                    _screen.RequestAnimationFrame();
                });
            });
        });
    });
}

void SettingPage::doLockAccount() {
	_showLockConfirm = false;
	_loading = true;
	_status.clear();
	_client.toggleAccount(_username, [this](bool ok) {
		_screen.Post([this, ok] {
			_loading = false;
			if (ok) {
				if (_onLogout)
					_onLogout();
			} else {
				_status = "Failed to lock account";
			}
			_screen.RequestAnimationFrame();
		});
	});
}

ftxui::Component SettingPage::build() {
    _currentPassInput = ftxui::Input(&_currentPassword,
                                     "current password",
                                     ftxui::InputOption{
                                         .password = true,
                                         .multiline = false,
                                         .on_enter = [this] { _newPassInput->TakeFocus(); },
                                     });

    _newPassInput = ftxui::Input(&_newPassword,
                                 "new password",
                                 ftxui::InputOption{
                                     .password = true,
                                     .multiline = false,
                                     .on_enter = [this] { _confirmInput->TakeFocus(); },
                                 });

    _confirmInput = ftxui::Input(&_confirmPassword,
                                 "confirm new password",
                                 ftxui::InputOption{
                                     .password = true,
                                     .multiline = false,
                                     .on_enter = [this] { doChangePassword(); },
                                 });

    _changeBtn = ftxui::Button(
        "  Change Password  ", [this] { doChangePassword(); }, ftxui::ButtonOption::Border());

    // ── Logout confirmation ─────────────────────────────────────
    auto confirmYes = ftxui::Button("  Logout  ", [this] { _showLogoutConfirm = false; if (_onLogout) _onLogout(); },
                                    ftxui::ButtonOption::Border());
    auto confirmNo  = ftxui::Button("  Cancel  ", [this] { _showLogoutConfirm = false; },
                                    ftxui::ButtonOption::Border());

    auto confirmContent = ftxui::Container::Vertical({
        ftxui::Renderer([] {
            return ftxui::vbox({
                ftxui::text("Logout") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                ftxui::text("Are you sure you want to logout?") | ftxui::center,
            }) | ftxui::border | ftxui::center;
        }),
        ftxui::Container::Horizontal({
            confirmYes,
            confirmNo,
        }),
    });

    auto confirmDialog = ftxui::Maybe(confirmContent, &_showLogoutConfirm);

    // ── Lock confirmation ────────────────────────────────────────
    auto lockConfirmYes = ftxui::Button(
        "  Lock Account  ", [this] { doLockAccount(); },
        ftxui::ButtonOption::Border());
    auto lockConfirmNo  = ftxui::Button(
        "  Cancel  ", [this] { _showLockConfirm = false; },
        ftxui::ButtonOption::Border());

    auto lockConfirmContent = ftxui::Container::Vertical({
        ftxui::Renderer([] {
            return ftxui::vbox({
                ftxui::text("Lock Account") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                ftxui::text("Are you sure? You will not be able to log in") | ftxui::center,
                ftxui::text("again until an administrator unlocks your account.") | ftxui::center,
            }) | ftxui::border | ftxui::center;
        }),
        ftxui::Container::Horizontal({
            lockConfirmYes,
            lockConfirmNo,
        }),
    });

    auto lockConfirmDialog = ftxui::Maybe(lockConfirmContent, &_showLockConfirm);

    _logoutBtn = ftxui::Button("  Logout  ", [this] { _showLogoutConfirm = true; },
                               ftxui::ButtonOption::Border());

    _lockBtn = ftxui::Button("  Lock Account  ", [this] { _showLockConfirm = true; },
                             ftxui::ButtonOption::Border());

    auto container = ftxui::Container::Vertical({
        _currentPassInput,
        _newPassInput,
        _confirmInput,
        ftxui::Container::Horizontal({
            _changeBtn,
            _logoutBtn,
        }),
        _lockBtn,
        lockConfirmDialog,
        confirmDialog,
    });

    return ftxui::Renderer(container, [this] {
        // Password strength indicator for new password
        auto pwStrength = evaluateStrength(_newPassword);
        auto pwStrengthEl = _newPassword.empty()
                                ? ftxui::emptyElement()
                                : ftxui::hbox({
                                      ftxui::text(" New password strength: ") | ftxui::dim,
                                      ftxui::text(strengthLabel(pwStrength)) | ftxui::bold,
                                      ftxui::filler(),
                                  });

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::text(" Changing password...") | ftxui::dim;
            }
            if (!_status.empty()) {
                return ftxui::text(_status) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

        return ftxui::vbox({
                   ftxui::text("Settings") | ftxui::bold | ftxui::center,
                   ftxui::separator(),
                   ftxui::text(""),
                   ftxui::text("Change Password") | ftxui::bold,
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" Current: "),
                       _currentPassInput->Render() | ftxui::flex,
                   }) | ftxui::border,
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" New: "),
                       _newPassInput->Render() | ftxui::flex,
                   }) | ftxui::border,
                   pwStrengthEl,
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" Confirm: "),
                       _confirmInput->Render() | ftxui::flex,
                   }) | ftxui::border,
                   ftxui::text(""),
                   ftxui::separator(),
                   ftxui::hbox({
                       ftxui::filler(),
                       _changeBtn->Render(),
                       ftxui::text("  "),
                       _logoutBtn->Render(),
                       ftxui::filler(),
                   }),
                   statusEl,
                   ftxui::text(""),
                   ftxui::text(" Danger Zone ") | ftxui::bold | ftxui::center,
                   ftxui::separator(),
                   ftxui::hbox({
                       ftxui::filler(),
                       _lockBtn->Render(),
                       ftxui::filler(),
                   }),
                   ftxui::text(" Locking your account prevents all transactions") | ftxui::dim | ftxui::center,
                   ftxui::text(" and logging in until an administrator unlocks it.") | ftxui::dim | ftxui::center,
                   ftxui::text(""),
                   ftxui::text(" Logging out will return you to the login screen.") | ftxui::dim | ftxui::center,
                   ftxui::filler(),
                   ftxui::text("[Enter] Change  [Tab] Focus  [ESC] Quit") | ftxui::dim | ftxui::center,
               })
               | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 30)
               | ftxui::center
               | ftxui::border;
    });
}
