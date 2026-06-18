#include "app/pages/SettingPage.hpp"
#include "app/Theme.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

// ── Inline password strength helpers ─────────────────────────────────
static int evaluateStrength(const std::string &pw) {
	auto len = pw.size();
	if (len == 0)
		return 0;
	if (len < 6)
		return 0; // Weak
	if (len < 12)
		return 1; // Medium
	return 2;	  // Strong
}

static const char *strengthLabel(int s) {
	switch (s) {
	case 0:
		return "Weak";
	case 1:
		return "Medium";
	case 2:
		return "Strong";
	default:
		return "?";
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
	auto input_option = ftxui::InputOption::Default();

	_currentPassInput = ftxui::Input(&_currentPassword, "current password", input_option);
	_currentPassInput |= ftxui::CatchEvent([this](ftxui::Event e) {
		if (e == ftxui::Event::Return) {
			_newPassInput->TakeFocus();
			return true;
		}
		return false;
	});

	_newPassInput = ftxui::Input(&_newPassword, "new password", input_option);
	_newPassInput |= ftxui::CatchEvent([this](ftxui::Event e) {
		if (e == ftxui::Event::Return) {
			_confirmInput->TakeFocus();
			return true;
		}
		return false;
	});

	_confirmInput = ftxui::Input(&_confirmPassword, "confirm new password", input_option);
	_confirmInput |= ftxui::CatchEvent([this](ftxui::Event e) {
		if (e == ftxui::Event::Return) {
			doChangePassword();
			return true;
		}
		return false;
	});

	_changeBtn = ftxui::Button(
		" CHANGE PASSWORD ", [this] { doChangePassword(); }, theme::Button(theme::Mauve));

	_logoutBtn = ftxui::Button(
		" LOGOUT ",
		[this] {
			_showLogoutConfirm = true;
			_screen.Post([this] { _logoutConfirmYes->TakeFocus(); });
		},
		theme::Button(theme::Overlay2));

	_lockBtn = ftxui::Button(
		" LOCK ACCOUNT ",
		[this] {
			_showLockConfirm = true;
			_screen.Post([this] { _lockConfirmYes->TakeFocus(); });
		},
		theme::Button(theme::Red));

	_logoutConfirmYes = ftxui::Button(
		" LOGOUT ",
		[this] {
			_showLogoutConfirm = false;
			if (_onLogout)
				_onLogout();
		},
		theme::Button(theme::Overlay2));
	_logoutConfirmNo = ftxui::Button(
		" CANCEL ", [this] { _showLogoutConfirm = false; }, theme::Button(theme::Overlay2));

	_lockConfirmYes
		= ftxui::Button(" LOCK ACCOUNT ", [this] { doLockAccount(); }, theme::Button(theme::Red));
	_lockConfirmNo = ftxui::Button(
		" CANCEL ", [this] { _showLockConfirm = false; }, theme::Button(theme::Overlay2));

	auto logoutConfirmContainer
		= ftxui::Container::Horizontal({_logoutConfirmYes, _logoutConfirmNo});
	auto lockConfirmContainer = ftxui::Container::Horizontal({_lockConfirmYes, _lockConfirmNo});

	auto container = ftxui::Container::Vertical({
		_currentPassInput,
		_newPassInput,
		_confirmInput,
		_changeBtn,
		_logoutBtn,
		_lockBtn,
		ftxui::Maybe(logoutConfirmContainer, &_showLogoutConfirm),
		ftxui::Maybe(lockConfirmContainer, &_showLockConfirm),
	});

	return ftxui::Renderer(
		container, [this, logoutConfirmContainer, lockConfirmContainer]() -> ftxui::Element {
			auto pwStrength = evaluateStrength(_newPassword);
			auto strengthColor = theme::Red;
			if (pwStrength == 1)
				strengthColor = theme::Yellow;
			if (pwStrength == 2)
				strengthColor = theme::Green;

			auto pwStrengthEl
				= _newPassword.empty()
					  ? ftxui::emptyElement()
					  : ftxui::hbox({
							ftxui::text(" Strength: ") | ftxui::color(theme::Subtext0),
							ftxui::text(strengthLabel(pwStrength)) | ftxui::bold
								| ftxui::color(strengthColor),
							ftxui::filler(),
						});

			auto statusEl = [this]() -> ftxui::Element {
				if (_loading) {
					++_spinnerFrame;
					return ftxui::hbox({
							   ftxui::spinner(21, _spinnerFrame) | ftxui::color(theme::Mauve),
							   ftxui::text(" Processing..."),
						   })
						   | ftxui::center;
				}
				if (!_status.empty()) {
					auto color = (_status.find("successfully") != std::string::npos) ? theme::Green
																					 : theme::Red;
					return ftxui::text(_status) | ftxui::color(color) | ftxui::center;
				}
				return ftxui::emptyElement();
			}();

			auto mainEl
				= ftxui::vbox({
					  ftxui::text("ACCOUNT SETTINGS") | ftxui::bold | ftxui::center
						  | ftxui::color(theme::Mauve),
					  ftxui::separator() | ftxui::color(theme::Mauve),
					  ftxui::text(""),
					  ftxui::text(" CHANGE PASSWORD") | ftxui::bold | ftxui::color(theme::Subtext1),
					  ftxui::hbox({
						  ftxui::text(" CURRENT ") | ftxui::color(theme::Subtext0)
							  | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
						  _currentPassInput->Render() | ftxui::flex,
					  }) | ftxui::borderStyled(ftxui::ROUNDED)
						  | ftxui::color(theme::Surface1),
					  ftxui::hbox({
						  ftxui::text(" NEW     ") | ftxui::color(theme::Subtext0)
							  | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
						  _newPassInput->Render() | ftxui::flex,
					  }) | ftxui::borderStyled(ftxui::ROUNDED)
						  | ftxui::color(theme::Surface1),
					  pwStrengthEl,
					  ftxui::hbox({
						  ftxui::text(" CONFIRM ") | ftxui::color(theme::Subtext0)
							  | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
						  _confirmInput->Render() | ftxui::flex,
					  }) | ftxui::borderStyled(ftxui::ROUNDED)
						  | ftxui::color(theme::Surface1),
					  ftxui::text(""),
					  ftxui::vbox({
						  _changeBtn->Render(),
						  _logoutBtn->Render(),
					  }),
					  statusEl,
					  ftxui::text(""),
					  ftxui::text(" DANGER ZONE ") | ftxui::bold | ftxui::center
						  | ftxui::color(theme::Red),
					  ftxui::separator() | ftxui::color(theme::Red),
					  ftxui::hbox({
						  ftxui::filler(),
						  _lockBtn->Render(),
						  ftxui::filler(),
					  }),
					  ftxui::text(" Locking your account prevents all transactions") | ftxui::dim
						  | ftxui::center | ftxui::color(theme::Overlay1),
					  ftxui::text(" and logging in until an administrator unlocks it.") | ftxui::dim
						  | ftxui::center | ftxui::color(theme::Overlay1),
					  ftxui::filler(),
				  })
				  | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40) | ftxui::center;

			if (_showLogoutConfirm) {
				return ftxui::dbox({
					mainEl,
					ftxui::vbox({
						ftxui::text(" LOGOUT ") | ftxui::bold | ftxui::center
							| ftxui::color(theme::Overlay2),
						ftxui::separator() | ftxui::color(theme::Overlay2),
						ftxui::text("Are you sure you want to logout?") | ftxui::center
							| ftxui::color(theme::Text),
						ftxui::text(""),
						ftxui::hbox({ftxui::filler(),
									 _logoutConfirmYes->Render(),
									 ftxui::text("  "),
									 _logoutConfirmNo->Render(),
									 ftxui::filler()}),
					}) | ftxui::borderStyled(ftxui::DOUBLE)
						| ftxui::color(theme::Overlay2) | ftxui::bgcolor(theme::Base)
						| ftxui::center | ftxui::clear_under,
				});
			}
			if (_showLockConfirm) {
				return ftxui::dbox({
					mainEl,
					ftxui::vbox({
						ftxui::text(" LOCK ACCOUNT ") | ftxui::bold | ftxui::center
							| ftxui::color(theme::Red),
						ftxui::separator() | ftxui::color(theme::Red),
						ftxui::text("Are you sure? This will disable your account") | ftxui::center
							| ftxui::color(theme::Text),
						ftxui::text("and log you out immediately.") | ftxui::center
							| ftxui::color(theme::Text),
						ftxui::text(""),
						ftxui::hbox({ftxui::filler(),
									 _lockConfirmYes->Render(),
									 ftxui::text("  "),
									 _lockConfirmNo->Render(),
									 ftxui::filler()}),
					}) | ftxui::borderStyled(ftxui::DOUBLE)
						| ftxui::color(theme::Red) | ftxui::bgcolor(theme::Base) | ftxui::center
						| ftxui::clear_under,
				});
			}
			return mainEl;
		});
}
