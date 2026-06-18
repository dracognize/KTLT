#include "app/pages/HistoryPage.hpp"
#include "app/Theme.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"
#include "libs/base/algorithm.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <charconv>
#include <chrono>

HistoryPage::HistoryPage(Client &client,
                         ftxui::ScreenInteractive &screen,
                         const std::string &username,
                         DashboardPage &dashboard)
    : _client(client), _screen(screen), _username(username), _dashboard(dashboard) {
    _lastRefreshTime = std::chrono::steady_clock::now();
}

void HistoryPage::doRefresh() {
    if (_loading)
        return;
    _loading = true;
    _status.clear();
    _lastRefreshTime = std::chrono::steady_clock::now();
    _spinnerFrame = 0;
    _client.getTransactionHistory(_username, 100, [this](std::optional<std::string> resp) {
        auto parsed = base::Vector<DashboardPage::LogEntry>{};
        auto ok = false;
        if (resp) {
            ok = true;
            DashboardPage::parseTransactionHistory(*resp, parsed);
        }
        _screen.Post([this, parsed = std::move(parsed), ok] {
            _loading = false;
            if (ok) {
                _entries = std::move(parsed);
                _status.clear();
            } else {
                _entries.clear();
                _status = "Failed to fetch history";
            }
            _screen.RequestAnimationFrame();
        });
    });
}

// Sort helpers
static auto getHeaderLabel(HistoryPage::SortColumn col) -> std::string_view {
    switch (col) {
        case HistoryPage::Time:      return " TIME ";
        case HistoryPage::Operation: return " OPERATION ";
        case HistoryPage::Amount:    return " CHANGE ";
        case HistoryPage::Balance:   return " BALANCE ";
    }
    return "?";
}

static auto makeComparator(HistoryPage::SortColumn col, bool asc) {
    return [col, asc](const DashboardPage::LogEntry &a,
                      const DashboardPage::LogEntry &b) -> bool {
        switch (col) {
            case HistoryPage::Time:
                return asc ? a.time < b.time : b.time < a.time;
            case HistoryPage::Operation:
                return asc ? a.operation < b.operation : b.operation < a.operation;
            case HistoryPage::Amount:
                return asc ? a.amount < b.amount : b.amount < a.amount;
            case HistoryPage::Balance:
                return asc ? a.balance < b.balance : b.balance < a.balance;
        }
        return false;
    };
}

ftxui::Component HistoryPage::build() {
    auto input_option = ftxui::InputOption::Default();
    _searchInput = ftxui::Input(&_searchStr, "search transactions...", input_option);

    auto headerRow = ftxui::Container::Horizontal({});
    for (int ci = 0; ci < 4; ++ci) {
        auto col = static_cast<SortColumn>(ci);
        auto btn = ftxui::Button(
            std::string(getHeaderLabel(col)),
            [this, col] {
                if (_sortColumn == col) _sortAscending = !_sortAscending;
                else { _sortColumn = col; _sortAscending = true; }
                _screen.RequestAnimationFrame();
            },
            theme::Button(theme::Sapphire));
        headerRow->Add(btn);
    }

    auto tableRenderer = ftxui::Renderer([this]() -> ftxui::Element {
        auto displayEntries = base::Vector<DashboardPage::LogEntry>{};
        if (!_loading && _status.empty()) {
            auto filterStr = base::String(std::string_view{_searchStr});
            for (const auto &e : _entries) {
                if (filterStr.empty() || e.operation.find(filterStr) != base::String::npos)
                    displayEntries.push_back(e);
            }
            if (displayEntries.size() > 1) {
                auto cmp = makeComparator(_sortColumn, _sortAscending);
                base::sort(displayEntries.begin(), displayEntries.end(), cmp);
            }
        }

        auto elements = ftxui::Elements();
        if (_loading) {
            elements.push_back(ftxui::hbox({
                ftxui::spinner(21, ++_spinnerFrame) | ftxui::color(theme::Sapphire),
                ftxui::text(" Loading history..."),
            }) | ftxui::center);
        } else if (!_status.empty()) {
            elements.push_back(ftxui::text(_status) | ftxui::color(theme::Red) | ftxui::center);
        } else if (displayEntries.empty()) {
            elements.push_back(ftxui::text("No transactions found") | ftxui::dim | ftxui::center);
        } else {
            for (const auto &e : displayEntries) {
                auto opColor = theme::Text;
                if (e.operation.view().find("Deposit") != std::string::npos || e.operation.view().find("Transfer In") != std::string::npos) opColor = theme::Green;
                else if (e.operation.view().find("Withdraw") != std::string::npos || e.operation.view().find("Transfer Out") != std::string::npos) opColor = theme::Red;

                elements.push_back(ftxui::hbox({
                    ftxui::text(" " + std::string(e.time.view())) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12) | ftxui::color(theme::Overlay2),
                    ftxui::text(" " + std::string(e.operation.view())) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 22) | ftxui::color(opColor),
                    ftxui::text(std::string(e.amount.view())) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12) | ftxui::color(opColor) | ftxui::align_right,
                    ftxui::filler(),
                    ftxui::text(std::to_string(e.balance) + " ") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12) | ftxui::align_right | ftxui::color(theme::Text),
                }));
            }
        }

        return ftxui::vbox(std::move(elements));
    });

    auto container = ftxui::Container::Vertical({_searchInput, headerRow, tableRenderer});

    return ftxui::Renderer(container, [this, tableRenderer, headerRow]() -> ftxui::Element {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - _lastRefreshTime).count();
        std::string timeAgo = (elapsed < 5) ? "just now" : std::to_string(elapsed) + "s ago";

        return ftxui::vbox({
            ftxui::text("TRANSACTION HISTORY") | ftxui::bold | ftxui::center | ftxui::color(theme::Sapphire),
            ftxui::separator() | ftxui::color(theme::Sapphire),
            ftxui::hbox({
                ftxui::text(" SEARCH ") | ftxui::color(theme::Subtext0) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 10),
                _searchInput->Render() | ftxui::flex,
            }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1),
            headerRow->Render() | ftxui::center,
            ftxui::separator() | ftxui::color(theme::Surface2),
            tableRenderer->Render() | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex,
            ftxui::separator() | ftxui::color(theme::Surface2),
            ftxui::hbox({
                ftxui::text(" SORT: ") | ftxui::color(theme::Subtext0),
                ftxui::text(std::string(getHeaderLabel(_sortColumn))) | ftxui::bold | ftxui::color(theme::Sapphire),
                ftxui::text(_sortAscending ? " ▲" : " ▼") | ftxui::color(theme::Sapphire),
                ftxui::filler(),
                ftxui::text("Last updated: " + timeAgo) | ftxui::dim,
            }),
            ftxui::text("Showing " + std::to_string(_entries.size()) + " entries") | ftxui::dim | ftxui::center,
        });
    });
}
