#include "app/pages/HistoryPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <charconv>
#include <chrono>

HistoryPage::HistoryPage(Client &client,
                         ftxui::ScreenInteractive &screen,
                         const std::string &username,
                         DashboardPage &dashboard)
    : _client(client), _screen(screen), _username(username), _dashboard(dashboard) {
    _pendingRefresh = true;
    _lastRefreshTime = std::chrono::steady_clock::now();
}

void HistoryPage::doRefresh() {
    if (_loading) [[unlikely]]
        return;
    _loading = true;
    _status.clear();
    _lastRefreshTime = std::chrono::steady_clock::now();
    _spinnerFrame = 0;
    _client.getLogs(_username, [this](std::optional<std::string> logs) {
        auto parsed = std::vector<DashboardPage::LogEntry>{};
        auto history = std::vector<u64>{};
        auto ok = false;
        if (logs) {
            ok = true;
            DashboardPage::parseRawLogs(*logs, parsed, history);
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

// ── Append helpers ──

static void appendPaddedRight(std::string &out, std::string_view s, std::size_t n) {
    if (s.size() >= n) {
        out.append(s.substr(0, n));
    } else {
        out.append(s);
        out.append(n - s.size(), ' ');
    }
}

static void appendPaddedLeft(std::string &out, std::string_view s, std::size_t n) {
    if (s.size() >= n) {
        out.append(s.substr(0, n));
    } else {
        out.append(n - s.size(), ' ');
        out.append(s);
    }
}

static void formatLogLine(std::string &line,
                          std::string_view time,
                          std::string_view operation,
                          std::string_view amount,
                          u64 balance) {
    char balBuf[32];
    auto [p, _] = std::to_chars(balBuf, balBuf + sizeof(balBuf), balance);
    auto balStr = std::string_view(balBuf, static_cast<std::size_t>(p - balBuf));

    line.clear();
    line.reserve(64);
    line += ' ';
    appendPaddedRight(line, time, 10);
    appendPaddedRight(line, operation, 22);
    appendPaddedLeft(line, amount.empty() ? std::string_view("--") : amount, 12);
    line += "  ";
    appendPaddedLeft(line, balStr, 12);
}

// ── Build ──

ftxui::Component HistoryPage::build() {
    auto renderer = ftxui::Renderer([this] {
        ++_spinnerFrame;

        // Trigger a refresh on first render
        if (_pendingRefresh) {
            _pendingRefresh = false;
            _screen.Post([this] { doRefresh(); });
        }

        // ── Last updated timestamp ──────────────────────────────
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - _lastRefreshTime).count();
        std::string timeAgo;
        if (elapsed < 5) {
            timeAgo = "just now";
        } else if (elapsed < 60) {
            timeAgo = std::to_string(elapsed) + "s ago";
        } else if (elapsed < 3600) {
            timeAgo = std::to_string(elapsed / 60) + "m ago";
        } else {
            timeAgo = "a while ago";
        }

        // ── Table Content ───────────────────────────────────────
        auto elements = ftxui::Elements();
        elements.reserve(4 + _entries.size());

        if (_loading) {
            elements.push_back(ftxui::text(" Loading history...") | ftxui::dim);
        } else if (!_status.empty()) {
            elements.push_back(ftxui::text(" " + _status));
        } else if (_entries.empty()) {
            elements.push_back(ftxui::text(" No transactions yet") | ftxui::dim);
        } else {
            // Header
            std::string hdr;
            hdr.reserve(1 + 10 + 22 + 12 + 2 + 12);
            hdr += ' ';
            appendPaddedRight(hdr, "Time", 10);
            appendPaddedRight(hdr, "Operation", 22);
            appendPaddedLeft(hdr, "Change", 12);
            hdr += "  ";
            appendPaddedLeft(hdr, "Balance", 12);
            elements.push_back(ftxui::text(std::move(hdr)) | ftxui::bold);
            elements.push_back(ftxui::separator() | ftxui::dim);

            // Data rows
            std::string line;
            for (const auto &e : _entries) {
                formatLogLine(line, e.time, e.operation, e.amount, e.balance);
                elements.push_back(ftxui::text(line));
            }
        }

        auto table = ftxui::vbox(std::move(elements))
                     | ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN, 10);

        // ── Footer info ─────────────────────────────────────────
        auto footer = ftxui::hbox({
            ftxui::text(" Last updated: ") | ftxui::dim,
            ftxui::text(timeAgo) | ftxui::bold,
            ftxui::filler(),
            ftxui::text(" Auto-refresh") | ftxui::dim,
        });

        // ── Entry count ─────────────────────────────────────────
        auto countInfo = ftxui::text(" Showing " + std::to_string(_entries.size()) + " entries ")
                         | ftxui::dim | ftxui::center;

        return ftxui::vbox({
                   ftxui::text("Transaction History") | ftxui::bold | ftxui::center,
                   ftxui::separator(),
                   ftxui::text(""),
                   table | ftxui::border | ftxui::flex,
                   countInfo,
                   footer,
                   ftxui::filler(),
                   ftxui::text("[1] Dashboard  [2] History  [3] Deposit  [4] Withdraw  [5] Transfer  [6] Settings") | ftxui::dim | ftxui::center,
               })
               | ftxui::border;
    });

    return renderer;
}
