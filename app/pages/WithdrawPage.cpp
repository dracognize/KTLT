#include "app/pages/WithdrawPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

WithdrawPage::WithdrawPage(Client &client,
                           ftxui::ScreenInteractive &screen,
                           const std::string &username,
                           int &postAuthPage,
                           DashboardPage &dashboard)
    : _client(client), _screen(screen), _username(username), _postAuthPage(postAuthPage),
      _dashboard(dashboard) {
}

void WithdrawPage::doWithdraw() {
    if (_amountStr.empty()) {
        _status = "Please enter an amount";
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
    _client.changeBalance(_username, -static_cast<i64>(amount), [this](bool ok) {
        _screen.Post([this, ok] {
            _loading = false;
            if (ok) {
                _status = "Withdrawal successful!";
                _amountStr.clear();
                _dashboard.doRefresh();
            } else {
                _status = "Withdrawal failed (insufficient balance)";
            }
            _screen.RequestAnimationFrame();
        });
    });
}

ftxui::Component WithdrawPage::build() {
    _amountInput = ftxui::Input(&_amountStr,
                                "amount",
                                ftxui::InputOption{
                                    .multiline = false,
                                    .on_enter = [this] { _showConfirm = true; },
                                });

    _withdrawBtn
        = ftxui::Button("  Withdraw  ", [this] { _showConfirm = true; },
                        ftxui::ButtonOption::Border());
    _backBtn = ftxui::Button(
        "  Back  ",
        [this] {
            _postAuthPage = 0;
            _screen.RequestAnimationFrame();
        },
        ftxui::ButtonOption::Border());

    // ── Confirmation dialog ─────────────────────────────────────
    auto confirmYes = ftxui::Button("  Confirm  ", [this] { doWithdraw(); _showConfirm = false; },
                                    ftxui::ButtonOption::Border());
    auto confirmNo  = ftxui::Button("  Cancel  ", [this] { _showConfirm = false; },
                                    ftxui::ButtonOption::Border());

    auto confirmContent = ftxui::Container::Vertical({
        ftxui::Renderer([] {
            return ftxui::vbox({
                ftxui::text("Confirm Withdrawal") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                ftxui::text("Are you sure you want to withdraw?") | ftxui::center,
            }) | ftxui::border | ftxui::center;
        }),
        ftxui::Container::Horizontal({
            confirmYes,
            confirmNo,
        }),
    });

    auto confirmDialog = ftxui::Maybe(confirmContent, &_showConfirm);

    auto container = ftxui::Container::Vertical({
        _amountInput,
        ftxui::Container::Horizontal({
            _withdrawBtn,
            _backBtn,
        }),
        confirmDialog,
    });

    return ftxui::Renderer(container, [this] {
        auto curBalance = _dashboard.balanceStr();
        if (curBalance.empty()) curBalance = "0";

        // Status line
        ftxui::Element statusEl;
        if (_loading) {
            ++_spinnerFrame;
            statusEl = ftxui::text(" Processing withdrawal...") | ftxui::dim;
        } else if (!_status.empty()) {
            statusEl = ftxui::text(_status) | ftxui::center;
        } else {
            statusEl = ftxui::emptyElement();
        }

        // Build layout
        ftxui::Elements mainContent;
        mainContent.push_back(ftxui::text("Withdraw") | ftxui::bold | ftxui::center);
        mainContent.push_back(ftxui::separator());
        mainContent.push_back(ftxui::text(""));
        mainContent.push_back(
            ftxui::hbox({
                ftxui::text(" Your Balance: ") | ftxui::bold,
                ftxui::text("$ " + curBalance),
                ftxui::filler(),
            }) | ftxui::border
        );
        mainContent.push_back(ftxui::text(""));
        mainContent.push_back(
            ftxui::hbox({
                ftxui::text(" Amount to withdraw: "),
                _amountInput->Render() | ftxui::flex,
            }) | ftxui::border
        );
        mainContent.push_back(ftxui::text(""));
        mainContent.push_back(ftxui::separator());

        ftxui::Elements buttonRow;
        buttonRow.push_back(ftxui::filler());
        buttonRow.push_back(_withdrawBtn->Render());
        buttonRow.push_back(ftxui::text("  "));
        buttonRow.push_back(_backBtn->Render());
        buttonRow.push_back(ftxui::filler());
        mainContent.push_back(ftxui::hbox(std::move(buttonRow)));

        mainContent.push_back(statusEl);
        mainContent.push_back(ftxui::text(""));
        mainContent.push_back(ftxui::text("Enter positive numbers only") | ftxui::dim | ftxui::center);
        mainContent.push_back(ftxui::filler());
        mainContent.push_back(ftxui::text("[Enter] Withdraw  [Back] Return  [ESC] Quit") | ftxui::dim | ftxui::center);

        return ftxui::vbox(std::move(mainContent))
               | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 30)
               | ftxui::center
               | ftxui::border;
    });
}
