<<<<<<< HEAD
#include "app/pages/AccountSettingsPage.hpp"
#include "app/client/Client.hpp"
=======
#include "app/pages/TransferPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

<<<<<<< HEAD
AccountSettingsPage::AccountSettingsPage(Client &client, ftxui::ScreenInteractive &screen,
										  const std::string &username, int &page)
	: _client(client), _screen(screen), _username(username), _page(page) {
}

void AccountSettingsPage::doChangePassword() {
	if (_newPassword.empty()) {
		_status = "Please enter a new password";
		return;
	}
	if (_newPassword != _confirmPassword) {
		_status = "Passwords do not match";
		return;
	}

	_loading = true;
	_status = "";
	_client.changePassword(_username, _newPassword, [this](bool ok) {
		_screen.Post([this, ok] {
			_loading = false;
			_status = ok ? "Password changed successfully" : "Failed to change password";
			if (ok) {
				_newPassword.clear();
				_confirmPassword.clear();
			}
			_screen.RequestAnimationFrame();
		});
	});
}

void AccountSettingsPage::doToggleLock() {
	_loading = true;
	_status = "";
	_client.toggleAccount(_username, [this](bool ok) {
		_screen.Post([this, ok] {
			_loading = false;
			_status = ok ? "Account lock status toggled" : "Failed to toggle account";
			_screen.RequestAnimationFrame();
		});
	});
}

void AccountSettingsPage::reset() {
	_newPassword.clear();
	_confirmPassword.clear();
	_status.clear();
	_loading = false;
}

ftxui::Component AccountSettingsPage::build() {
	auto newPassOpt = ftxui::InputOption{};
	newPassOpt.password = true;
	newPassOpt.multiline = false;

	auto confirmOpt = ftxui::InputOption{};
	confirmOpt.password = true;
	confirmOpt.multiline = false;
	confirmOpt.on_enter = [this] { doChangePassword(); };

	auto newPassInput = ftxui::Input(&_newPassword, "new password", newPassOpt);
	auto confirmInput = ftxui::Input(&_confirmPassword, "confirm new password", confirmOpt);
	auto changePassBtn = ftxui::Button("Change Password", [this] { doChangePassword(); });
	auto toggleLockBtn = ftxui::Button("Lock/Unlock My Account", [this] { doToggleLock(); });
	auto backBtn = ftxui::Button("Back", [this] { _page = 1; });

	auto container = ftxui::Container::Vertical({
		newPassInput,
		confirmInput,
		changePassBtn,
		toggleLockBtn,
		backBtn,
	});

	return ftxui::Renderer(container, [this, newPassInput, confirmInput, changePassBtn, toggleLockBtn, backBtn] {
		return ftxui::vbox({
				   ftxui::text("Account Settings") | ftxui::bold | ftxui::center,
				   ftxui::text(" User: " + _username),
				   ftxui::separator(),
				   ftxui::hbox({
					   ftxui::text(" New password: "),
					   newPassInput->Render() | ftxui::flex,
				   }),
				   ftxui::hbox({
					   ftxui::text(" Confirm:      "),
					   confirmInput->Render() | ftxui::flex,
				   }),
				   changePassBtn->Render() | ftxui::center,
				   ftxui::separator(),
				   toggleLockBtn->Render() | ftxui::center,
				   ftxui::separator(),
				   backBtn->Render() | ftxui::center,
				   ftxui::text(_status) | ftxui::center,
			   })
			   | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32);
	});
}
=======
TransferPage::TransferPage(Client &client,
                           ftxui::ScreenInteractive &screen,
                           const std::string &username,
                           int &postAuthPage,
                           DashboardPage &dashboard)
    : _client(client), _screen(screen), _username(username), _postAuthPage(postAuthPage),
      _dashboard(dashboard) {
}

void TransferPage::doTransfer() {
    if (_recipient.empty() || _amountStr.empty()) {
        _status = "Please fill in all fields";
        return;
    }
    if (_recipient == _username) {
        _status = "Cannot transfer to yourself";
        return;
    }
    u64 amount = 0;
    try {
        amount = std::stoull(_amountStr);
    } catch (...) {
        _status = "Invalid amount";
        return;
    }
    if (amount == 0) {
        _status = "Amount must be greater than 0";
        return;
    }
    _loading = true;
    _status.clear();
    _spinnerFrame = 0;
    _client.transferBalance(_username, _recipient, amount, [this](bool ok) {
        _screen.Post([this, ok] {
            _loading = false;
            if (ok) {
                _status = "Transfer successful!";
                _recipient.clear();
                _amountStr.clear();
                _dashboard.doRefresh();
            } else {
                _status = "Transfer failed (check recipient and balance)";
            }
            _screen.RequestAnimationFrame();
        });
    });
}

ftxui::Component TransferPage::build() {
    _recipientInput = ftxui::Input(&_recipient,
                                   "recipient username",
                                   ftxui::InputOption{
                                       .multiline = false,
                                       .on_enter = [this] { _amountInput->TakeFocus(); },
                                   });

    _amountInput = ftxui::Input(&_amountStr,
                                "amount",
                                ftxui::InputOption{
                                    .multiline = false,
                                    .on_enter = [this] { doTransfer(); },
                                });

    _transferBtn
        = ftxui::Button("  Transfer  ", [this] { doTransfer(); },
                        ftxui::ButtonOption::Border());
    _backBtn = ftxui::Button(
        "  Back  ",
        [this] {
            _postAuthPage = 0;
            _screen.RequestAnimationFrame();
        },
        ftxui::ButtonOption::Border());

    auto container = ftxui::Container::Vertical({
        _recipientInput,
        _amountInput,
        ftxui::Container::Horizontal({
            _transferBtn,
            _backBtn,
        }),
    });

    return ftxui::Renderer(container, [this] {
        auto curBalance = _dashboard.balanceStr();
        if (curBalance.empty()) curBalance = "0";

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::text(" Processing transfer...") | ftxui::dim;
            }
            if (!_status.empty()) {
                return ftxui::text(_status) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

        return ftxui::vbox({
                   ftxui::text("Transfer") | ftxui::bold | ftxui::center,
                   ftxui::separator(),
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" Your Balance: ") | ftxui::bold,
                       ftxui::text("$ " + curBalance),
                       ftxui::filler(),
                   }) | ftxui::border,
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" Recipient: "),
                       _recipientInput->Render() | ftxui::flex,
                   }) | ftxui::border,
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" Amount: "),
                       _amountInput->Render() | ftxui::flex,
                   }) | ftxui::border,
                   ftxui::text(""),
                   ftxui::separator(),
                   ftxui::hbox({
                       ftxui::filler(),
                       _transferBtn->Render(),
                       ftxui::text("  "),
                       _backBtn->Render(),
                       ftxui::filler(),
                   }),
                   statusEl,
                   ftxui::text(""),
                   ftxui::text("Cannot transfer to yourself") | ftxui::dim | ftxui::center,
               })
               | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 30)
               | ftxui::center
               | ftxui::border;
    });
}
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
