#include "app/pages/DepositPage.hpp"
#include "app/Theme.hpp"
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
    auto input_option = ftxui::InputOption::Default();

    _amountInput = ftxui::Input(&_amountStr, "amount", input_option);
    _amountInput |= ftxui::CatchEvent([this](ftxui::Event e) {
        if (e == ftxui::Event::Return) {
            doDeposit();
            return true;
        }
        if (e.is_character()) {
            return !std::isdigit(e.character()[0]);
        }
        return false;
    });

    _depositBtn = ftxui::Button(" DEPOSIT ", [this] { doDeposit(); }, theme::Button(theme::Green));
    
    _backBtn = ftxui::Button(" BACK ", [this] { _postAuthPage = 0; }, theme::Button(theme::Overlay2));

    auto container = ftxui::Container::Vertical({
        _amountInput,
        ftxui::Container::Horizontal({
            _depositBtn,
            _backBtn,
        }),
    });

    return ftxui::Renderer(container, [this]() -> ftxui::Element {
        auto curBalance = _dashboard.balanceStr();
        if (curBalance.empty()) curBalance = "0";

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::hbox({
                    ftxui::spinner(21, _spinnerFrame) | ftxui::color(theme::Green),
                    ftxui::text(" Processing deposit..."),
                }) | ftxui::center;
            }
            if (!_status.empty()) {
                auto color = (_status.find("successful") != std::string::npos) ? theme::Green : theme::Red;
                return ftxui::text(_status) | ftxui::color(color) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

        auto depositBox = ftxui::vbox({
            ftxui::text("DEPOSIT FUNDS") | ftxui::bold | ftxui::center | ftxui::color(theme::Green),
            ftxui::separator() | ftxui::color(theme::Green),
            ftxui::text(""),
            ftxui::hbox({
                ftxui::text(" Your Balance ") | ftxui::color(theme::Subtext1),
                ftxui::filler(),
                ftxui::text("$ " + curBalance) | ftxui::bold | ftxui::color(theme::Yellow),
            }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
            ftxui::text(""),
            ftxui::hbox({
                ftxui::text(" AMOUNT ") | ftxui::color(theme::Subtext0) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
                _amountInput->Render() | ftxui::flex,
            }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
            ftxui::text(""),
            ftxui::separator() | ftxui::color(theme::Surface2),
            ftxui::hbox({
                ftxui::filler(),
                _depositBtn->Render(),
                ftxui::text("  "),
                _backBtn->Render(),
                ftxui::filler(),
            }),
            statusEl,
        }) | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40)
           | ftxui::center;

        return ftxui::vbox({
            ftxui::filler(),
            depositBox,
            ftxui::filler(),
        });
    });
}
