#include "app/pages/DepositPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

DepositPage::DepositPage(Client &client,
                         ftxui::ScreenInteractive &screen,
                         const std::string &username,
                         int &postAuthPage,
                         DashboardPage &dashboard)
    : _client(client), _screen(screen), _username(username), _postAuthPage(postAuthPage),
      _dashboard(dashboard) {
}

void DepositPage::doDeposit() {
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
    _client.changeBalance(_username, static_cast<i64>(amount), [this](bool ok) {
        _screen.Post([this, ok] {
            _loading = false;
            if (ok) {
                _status = "Deposit successful!";
                _amountStr.clear();
                _dashboard.doRefresh();
            } else {
                _status = "Deposit failed";
            }
            _screen.RequestAnimationFrame();
        });
    });
}

ftxui::Component DepositPage::build() {
    _amountInput = ftxui::Input(&_amountStr,
                                "amount",
                                ftxui::InputOption{
                                    .multiline = false,
                                    .on_enter = [this] { doDeposit(); },
                                });

    _depositBtn = ftxui::Button("  Deposit  ", [this] { doDeposit(); },
                                ftxui::ButtonOption::Border());
    _backBtn = ftxui::Button(
        "  Back  ",
        [this] {
            _postAuthPage = 0;
            _screen.RequestAnimationFrame();
        },
        ftxui::ButtonOption::Border());

    auto container = ftxui::Container::Vertical({
        _amountInput,
        ftxui::Container::Horizontal({
            _depositBtn,
            _backBtn,
        }),
    });

    return ftxui::Renderer(container, [this] {
        auto curBalance = _dashboard.balanceStr();
        if (curBalance.empty()) curBalance = "0";

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::text(" Processing deposit...") | ftxui::dim;
            }
            if (!_status.empty()) {
                return ftxui::text(_status) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

        return ftxui::vbox({
                   ftxui::text("Deposit") | ftxui::bold | ftxui::center,
                   ftxui::separator(),
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" Your Balance: ") | ftxui::bold,
                       ftxui::text("$ " + curBalance),
                       ftxui::filler(),
                   }) | ftxui::border,
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" Amount to deposit: "),
                       _amountInput->Render() | ftxui::flex,
                   }) | ftxui::border,
                   ftxui::text(""),
                   ftxui::separator(),
                   ftxui::hbox({
                       ftxui::filler(),
                       _depositBtn->Render(),
                       ftxui::text("  "),
                       _backBtn->Render(),
                       ftxui::filler(),
                   }),
                   statusEl,
                   ftxui::text(""),
                   ftxui::text("Enter positive numbers only") | ftxui::dim | ftxui::center,
               })
               | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 30)
               | ftxui::center
               | ftxui::border;
    });
}
