#include "app/pages/WithdrawPage.hpp"
#include "app/Theme.hpp"
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
    auto input_option = ftxui::InputOption::Default();

    _amountInput = ftxui::Input(&_amountStr, "amount", input_option);
    _amountInput |= ftxui::CatchEvent([this](ftxui::Event e) {
        if (e == ftxui::Event::Return) {
            _showConfirm = true;
            _screen.Post([this] { _confirmYes->TakeFocus(); });
            return true;
        }
        if (e.is_character()) {
            return !std::isdigit(e.character()[0]);
        }
        return false;
    });

    _withdrawBtn = ftxui::Button(" WITHDRAW ", [this] {
              _showConfirm = true;
              _screen.Post([this] { _confirmYes->TakeFocus(); });
          }, theme::Button(theme::Red));
    
    _backBtn = ftxui::Button(" BACK ", [this] { _postAuthPage = 0; }, theme::Button(theme::Overlay2));

    _confirmYes = ftxui::Button(" CONFIRM ", [this] { doWithdraw(); _showConfirm = false; }, theme::Button(theme::Red));
    
    auto confirmNo  = ftxui::Button(" CANCEL ", [this] { _showConfirm = false; }, theme::Button(theme::Overlay2));

    auto confirmContent = ftxui::Container::Vertical({
        ftxui::Renderer([] {
            return ftxui::vbox({
                ftxui::text(" CONFIRM WITHDRAWAL ") | ftxui::bold | ftxui::center | ftxui::color(theme::Red),
                ftxui::separator() | ftxui::color(theme::Red),
                ftxui::text("Are you sure you want to withdraw?") | ftxui::center | ftxui::color(theme::Text),
            });
        }),
        ftxui::Container::Horizontal({
            _confirmYes,
            confirmNo,
        }) | ftxui::center,
    });

    auto confirmDialog = ftxui::Maybe(confirmContent, &_showConfirm);

    auto container = ftxui::Container::Vertical({
        _amountInput,
        _withdrawBtn,
        _backBtn,
        confirmDialog,
    });

    return ftxui::Renderer(container, [this, confirmDialog]() -> ftxui::Element {
        auto curBalance = _dashboard.balanceStr();
        if (curBalance.empty()) curBalance = "0";

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::hbox({
                    ftxui::spinner(21, _spinnerFrame) | ftxui::color(theme::Red),
                    ftxui::text(" Processing withdrawal..."),
                }) | ftxui::center;
            }
            if (!_status.empty()) {
                auto color = (_status.find("successful") != std::string::npos) ? theme::Green : theme::Red;
                return ftxui::text(_status) | ftxui::color(color) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

        auto withdrawBox = ftxui::vbox({
            ftxui::text("WITHDRAW FUNDS") | ftxui::bold | ftxui::center | ftxui::color(theme::Red),
            ftxui::separator() | ftxui::color(theme::Red),
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
                _withdrawBtn->Render(),
                ftxui::text("  "),
                _backBtn->Render(),
                ftxui::filler(),
            }),
            statusEl,
        }) | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40)
           | ftxui::center;

        auto mainEl = ftxui::vbox({
            ftxui::filler(),
            withdrawBox,
            ftxui::filler(),
        });

        if (_showConfirm) {
            return ftxui::dbox({
                mainEl,
                confirmDialog->Render() 
                    | ftxui::clear_under 
                    | ftxui::center 
                    | ftxui::borderStyled(ftxui::DOUBLE) 
                    | ftxui::color(theme::Red)
                    | ftxui::bgcolor(theme::Base),
            });
        }
        return mainEl;
    });
}
