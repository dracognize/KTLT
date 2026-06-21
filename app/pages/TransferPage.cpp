#include "app/pages/TransferPage.hpp"
#include "app/Theme.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <cctype>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

TransferPage::TransferPage(Client &client,
                           ftxui::ScreenInteractive &screen,
                           const std::string &username,
                           int &postAuthPage,
                           DashboardPage &dashboard)
    : _client(client), _screen(screen), _username(username), _postAuthPage(postAuthPage),
      _dashboard(dashboard) {
}

void TransferPage::requestConfirmation() {
    
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
    _status.clear();
    _showConfirm = true;
    _screen.Post([this] { _confirmYes->TakeFocus(); });
}

void TransferPage::doTransfer() {
    u64 amount = 0;
    try {
        amount = std::stoull(_amountStr);
    } catch (...) {
        _status = "Invalid amount";
        _showConfirm = false;
        return;
    }
    _loading = true;
    _showConfirm = false;
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



void TransferPage::onRecipientChanged() {
    if (!_cacheLoaded && !_cacheLoading) {
        fetchUserDirectory();
    } else if (_cacheLoaded) {
        filterSuggestions();
    }
}

void TransferPage::fetchUserDirectory() {
    if (_cacheLoading)
        return;
    _cacheLoading = true;
    _client.searchUsers("", [this](std::optional<std::string> resp) {
        _screen.Post([this, resp = std::move(resp)] {
            _cacheLoading = false;
            _cacheLoaded = true;
            _userCache.clear();
            if (resp && !resp->empty()) {
                auto sv = std::string_view(*resp);
                std::string_view::size_type pos = 0;
                while (pos < sv.size()) {
                    auto pipe = sv.find('|', pos);
                    if (pipe == std::string_view::npos) {
                        _userCache.push_back(base::String(sv.substr(pos)));
                        break;
                    }
                    _userCache.push_back(base::String(sv.substr(pos, pipe - pos)));
                    pos = pipe + 1;
                }
            }
            filterSuggestions();
            _screen.RequestAnimationFrame();
        });
    });
}

void TransferPage::filterSuggestions() {
    _suggestions.clear();
    _suggestionStrings.clear();

    if (_recipient.size() < 3) {
        _showSuggestions = false;
        return;
    }

    auto prefix = base::String(std::string_view(_recipient));
    auto myName = base::String(std::string_view(_username));

    for (const auto &name : _userCache) {
        if (name == myName)
            continue;
        if (name.starts_with(prefix)) {
            _suggestions.push_back(name);
        }
    }

    
    if (_suggestions.size() > 5) {
        _suggestions.resize(5);
    }

    _suggestionStrings.reserve(_suggestions.size());
    for (const auto &s : _suggestions) {
        _suggestionStrings.push_back(std::string(s.view()));
    }

    _showSuggestions = !_suggestions.empty();
    _selectedSuggestion = 0;
}



ftxui::Component TransferPage::build() {
    auto menu_option = ftxui::MenuOption::Vertical();
    menu_option.entries_option.transform = [](const ftxui::EntryState& s) {
        auto element = ftxui::text(" " + s.label) | ftxui::flex;
        if (s.focused) {
            return element | ftxui::bold | ftxui::color(theme::Base) | ftxui::bgcolor(theme::Sapphire);
        }
        return element | ftxui::color(theme::Text);
    };

    _suggestionsMenu = ftxui::Menu(&_suggestionStrings, &_selectedSuggestion, menu_option);
    _suggestionsMenu |= ftxui::CatchEvent([this](ftxui::Event e) {
        if (e == ftxui::Event::Return && _showSuggestions && !_suggestionStrings.empty()) {
            _recipient = _suggestionStrings[static_cast<std::size_t>(_selectedSuggestion)];
            _showSuggestions = false;
            _amountInput->TakeFocus();
            _screen.RequestAnimationFrame();
            return true;
        }
        if (e == ftxui::Event::ArrowUp && _selectedSuggestion == 0) {
            _recipientInput->TakeFocus();
            return true;
        }
        return false;
    });

    auto input_option = ftxui::InputOption::Default();
    input_option.on_change = [this] { onRecipientChanged(); };

    _recipientInput = ftxui::Input(&_recipient, "recipient username", input_option);
    _recipientInput |= ftxui::CatchEvent([this](ftxui::Event e) {
        if (e == ftxui::Event::Return) {
            if (_showSuggestions && !_suggestionStrings.empty()) {
                _recipient = _suggestionStrings[static_cast<std::size_t>(_selectedSuggestion)];
                _showSuggestions = false;
            }
            _amountInput->TakeFocus();
            return true;
        }
        if (e == ftxui::Event::ArrowDown && _showSuggestions && !_suggestionStrings.empty()) {
            _suggestionsMenu->TakeFocus();
            return true;
        }
        return false;
    });

    _amountInput = ftxui::Input(&_amountStr, "amount");
    _amountInput |= ftxui::CatchEvent([this](ftxui::Event e) {
        
        _showSuggestions = false;
        if (e == ftxui::Event::Return) {
            requestConfirmation();
            return true;
        }
        if (e.is_character()) {
            return !std::isdigit(e.character()[0]);
        }
        return false;
    });

    _transferBtn = ftxui::Button(" TRANSFER ", [this] { requestConfirmation(); }, theme::Button(theme::Sapphire));
    
    _backBtn = ftxui::Button(" BACK ", [this] { _postAuthPage = 0; }, theme::Button(theme::Overlay2));

    _confirmYes = ftxui::Button(" CONFIRM ", [this] { doTransfer(); _showConfirm = false; }, theme::Button(theme::Sapphire));

    _confirmNo = ftxui::Button(" CANCEL ", [this] { _showConfirm = false; }, theme::Button(theme::Overlay2));

    auto confirmContent = ftxui::Container::Vertical({
        ftxui::Renderer([this] {
            return ftxui::vbox({
                ftxui::text(" CONFIRM TRANSFER ") | ftxui::bold | ftxui::center | ftxui::color(theme::Sapphire),
                ftxui::separator() | ftxui::color(theme::Sapphire),
                ftxui::text("Are you sure you want to transfer?") | ftxui::center | ftxui::color(theme::Text),
                ftxui::text("") ,
                ftxui::hbox({
                    ftxui::text(" Recipient: ") | ftxui::color(theme::Subtext0),
                    ftxui::text(_recipient) | ftxui::bold | ftxui::color(theme::Mauve),
                }) | ftxui::center,
                ftxui::hbox({
                    ftxui::text(" Amount:    ") | ftxui::color(theme::Subtext0),
                    ftxui::text("$" + _amountStr) | ftxui::bold | ftxui::color(theme::Yellow),
                }) | ftxui::center,
            });
        }),
        ftxui::Container::Horizontal({
            _confirmYes,
            _confirmNo,
        }) | ftxui::center,
    });

    auto confirmDialog = ftxui::Maybe(confirmContent, &_showConfirm);

    auto container = ftxui::Container::Vertical({
        _recipientInput,
        ftxui::Maybe(_suggestionsMenu, &_showSuggestions),
        _amountInput,
        ftxui::Container::Horizontal({
            _transferBtn,
            _backBtn,
        }),
        confirmDialog,
    });

    return ftxui::Renderer(container, [this, confirmDialog]() -> ftxui::Element {
        auto curBalance = _dashboard.balanceStr();
        if (curBalance.empty()) curBalance = "0";

        auto statusEl = [this]() -> ftxui::Element {
            if (_loading) {
                ++_spinnerFrame;
                return ftxui::hbox({
                    ftxui::spinner(21, _spinnerFrame) | ftxui::color(theme::Sapphire),
                    ftxui::text(" Processing transfer..."),
                }) | ftxui::center;
            }
            if (!_status.empty()) {
                auto color = (_status.find("successful") != std::string::npos) ? theme::Green : theme::Red;
                return ftxui::text(_status) | ftxui::color(color) | ftxui::center;
            }
            return ftxui::emptyElement();
        }();

        auto suggestionEl = _showSuggestions
            ? ftxui::vbox({
                  _suggestionsMenu->Render() | ftxui::vscroll_indicator | ftxui::frame
                      | ftxui::size(ftxui::HEIGHT, ftxui::LESS_THAN, 8)
                      | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
              })
            : ftxui::emptyElement();

        auto transferBox = ftxui::vbox({
                   ftxui::text("TRANSFER FUNDS") | ftxui::bold | ftxui::center | ftxui::color(theme::Sapphire),
                   ftxui::separator() | ftxui::color(theme::Sapphire),
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" Your Balance ") | ftxui::color(theme::Subtext1),
                       ftxui::filler(),
                       ftxui::text("$ " + curBalance) | ftxui::bold | ftxui::color(theme::Yellow),
                   }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
                   ftxui::text(""),
                   ftxui::hbox({
                       ftxui::text(" RECIPIENT ") | ftxui::color(theme::Subtext0) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
                       _recipientInput->Render() | ftxui::flex,
                   }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
                   suggestionEl,
                   ftxui::hbox({
                       ftxui::text(" AMOUNT ") | ftxui::color(theme::Subtext0) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
                       _amountInput->Render() | ftxui::flex,
                   }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
                   ftxui::text(""),
                   ftxui::separator() | ftxui::color(theme::Surface2),
                   ftxui::hbox({
                       ftxui::filler(),
                       _transferBtn->Render(),
                       ftxui::text("  "),
                       _backBtn->Render(),
                       ftxui::filler(),
                   }),
                   statusEl,
                   ftxui::filler(),
               })
               | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40)
               | ftxui::center;

        auto mainEl = ftxui::vbox({
            ftxui::filler(),
            transferBox,
            ftxui::filler(),
        });

        if (_showConfirm) {
            return ftxui::dbox({
                mainEl,
                confirmDialog->Render()
                    | ftxui::clear_under
                    | ftxui::center
                    | ftxui::borderStyled(ftxui::DOUBLE)
                    | ftxui::color(theme::Sapphire)
                    | ftxui::bgcolor(theme::Base),
            });
        }
        return mainEl;
    });
}
