#include "app/pages/DashboardPage.hpp"
#include "app/client/Client.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <charconv>
#include <string>
#include <vector>

DashboardPage::DashboardPage(Client &client,
                             ftxui::ScreenInteractive &screen,
                             const std::string &username)
    : _client(client), _screen(screen), _username(username) {
}

void DashboardPage::parseRawLogs(const std::string &raw,
                                 std::vector<LogEntry> &parsed,
                                 std::vector<u64> &history,
                                 usize maxEntries) {
    parsed.clear();
    history.clear();

    if (raw.empty()) {
        return;
    }

    auto src = std::string_view{raw};
    if (!src.empty() && src.back() == '\n')
        src.remove_suffix(1);

    std::vector<std::string_view> lines;
    lines.reserve(maxEntries > 0 ? maxEntries : 20);
    {
        std::string_view::size_type pos = 0;
        while (pos < src.size()) {
            auto nl = src.find('\n', pos);
            if (nl == std::string_view::npos) {
                lines.push_back(src.substr(pos));
                break;
            }
            lines.push_back(src.substr(pos, nl - pos));
            pos = nl + 1;
        }
    }

    auto startIdx = lines.size() > maxEntries ? lines.size() - maxEntries : 0;
    parsed.reserve(maxEntries > 0 ? maxEntries : lines.size());
    history.reserve(parsed.capacity());

    for (auto i = startIdx; i < lines.size(); ++i) {
        auto line = lines[i];
        if (line.size() < 21 || line[0] != '[')
            continue;

        auto timeStr = std::string(line.substr(12, 8));

        auto bodyStart = line.find("] ");
        if (bodyStart == std::string_view::npos)
            continue;
        auto body = line.substr(bodyStart + 2);

        auto lastSpace = body.rfind(' ');
        if (lastSpace == std::string_view::npos)
            continue;

        u64 balance = 0;
        auto balStr = body.substr(lastSpace + 1);
        auto [ptr, ec] = std::from_chars(balStr.data(), balStr.data() + balStr.size(), balance);
        if (ec != std::errc{})
            continue;
        history.push_back(balance);

        auto bodyLessBal = body.substr(0, lastSpace);
        auto secondLastSpace = bodyLessBal.rfind(' ');

        std::string operation;
        std::string amount;

        if (secondLastSpace != std::string_view::npos) {
            auto lastToken = bodyLessBal.substr(secondLastSpace + 1);
            if (!lastToken.empty() && (lastToken[0] == '+' || lastToken[0] == '-')) {
                amount = lastToken;
                operation = bodyLessBal.substr(0, secondLastSpace);
            } else {
                amount.clear();
                operation = bodyLessBal;
            }
        } else {
            operation = bodyLessBal;
        }

        parsed.push_back({std::move(timeStr), std::move(operation), std::move(amount), balance});
    }
}

void DashboardPage::doRefresh() {
    if (_logsStatus != "Loading...") {
        _balanceStr = "Loading...";
        _logsStatus = "Loading...";
        _status.clear();
    }

    _client.getLogs(_username, [this](std::optional<std::string> logs) {
        auto parsed = std::vector<LogEntry>{};
        auto history = std::vector<u64>{};
        auto ok = false;
        if (logs) {
            ok = true;
            parseRawLogs(*logs, parsed, history, 5);
        }
        _screen.Post([this, parsed = std::move(parsed), history = std::move(history), ok] {
            if (ok) {
                _parsedLogs = std::move(parsed);
                _balanceHistory = std::move(history);
                _balanceStr
                    = _balanceHistory.empty() ? "0" : std::to_string(_balanceHistory.back());
                _logsStatus.clear();
            } else {
                _parsedLogs.clear();
                _balanceHistory.clear();
                _balanceStr = "ERROR";
                _status = "Failed to fetch logs";
                _logsStatus = "Failed to fetch logs";
            }
            _screen.RequestAnimationFrame();
        });
    });

    doPing();
}

void DashboardPage::doPing() {
    auto sentAt = std::chrono::steady_clock::now();
    _latencyStr = "?";
    _client.ping([this, sentAt](bool ok) {
        auto elapsed = std::chrono::steady_clock::now() - sentAt;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        _screen.Post([this, ok, ms] {
            if (ok) {
                _latencyStr = std::to_string(ms) + "ms";
            } else {
                _latencyStr = "ERR";
            }
            _screen.RequestAnimationFrame();
        });
    });
}

<<<<<<< HEAD
void DashboardPage::onExit() {
	_screen.Exit();
}

ftxui::Component DashboardPage::build() {
	auto refreshBtn = ftxui::Button("Refresh", [this] { doRefresh(); });
	auto logoutBtn = ftxui::Button("Logout", [this] { onLogout(); });
	auto exitBtn = ftxui::Button("Exit", [this] { onExit(); });
	auto settingsBtn = ftxui::Button("Account Settings", [this] { _page = 3; });
	auto logoutBtn = ftxui::Button("Logout", [this] { onLogout(); });
	auto exitBtn = ftxui::Button("Exit", [this] { onExit(); });

	auto container = ftxui::Container::Vertical({
		refreshBtn,
		transferBtn,
		settingsBtn,
		logoutBtn,
		exitBtn,
	});

	return ftxui::Renderer(container, [this, refreshBtn, logoutBtn, exitBtn] {
		return ftxui::vbox({
				   ftxui::text("Dashboard") | ftxui::bold | ftxui::center,
				   ftxui::separator(),
				   ftxui::text(" Welcome, " + _username + "!"),
				   ftxui::text(" Balance: " + _balanceStr),
				   ftxui::separator(),
				   refreshBtn->Render() | ftxui::center,
				   transferBtn->Render() | ftxui::center,
				   settingsBtn->Render() | ftxui::center,
				   logoutBtn->Render() | ftxui::center,
				   exitBtn->Render() | ftxui::center,
				   ftxui::text(_status) | ftxui::center,
			   })
			   | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40);
	});
=======
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
    appendPaddedRight(line, operation, 20);
    appendPaddedLeft(line, amount.empty() ? std::string_view("--") : amount, 10);
    line += "  ";
    appendPaddedLeft(line, balStr, 12);
}

// ── Build ──

ftxui::Component DashboardPage::build() {
    auto renderer = ftxui::Renderer([this] {
        ++_spinnerFrame;

        // ── Account Info ───────────────────────────────────────
        auto balanceAmount = _balanceStr.empty() ? "0" : std::string_view(_balanceStr);

        std::string balanceDispStr;
        balanceDispStr.reserve(balanceAmount.size() + 2);
        balanceDispStr += "$ ";
        balanceDispStr += balanceAmount;

        auto accountInfo = ftxui::vbox({
            ftxui::hbox({
                ftxui::text(" User: "),
                ftxui::text(_username) | ftxui::bold,
                ftxui::filler(),
                ftxui::text(" Latency: "),
                ftxui::text(_latencyStr) | ftxui::bold,
            }),
        }) | ftxui::border;

        // ── Balance Display ────────────────────────────────────
        auto balanceDisplay = ftxui::hbox({
            ftxui::text(" Current Balance: ") | ftxui::bold,
            ftxui::text(balanceDispStr),
            ftxui::filler(),
        }) | ftxui::border
           | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 28)
           | ftxui::center;

        // ── Recent Activity ────────────────────────────────────
        auto logElements = ftxui::Elements();
        logElements.reserve(4 + _parsedLogs.size());

        if (!_logsStatus.empty()) {
            if (_logsStatus == "Loading...") {
                logElements.push_back(ftxui::text(" Fetching data...") | ftxui::dim);
            } else {
                logElements.push_back(ftxui::text(" " + _logsStatus));
            }
        } else if (_parsedLogs.empty()) {
            logElements.push_back(ftxui::text(" No transactions yet") | ftxui::dim);
        } else {
            // Header row
            std::string hdr;
            hdr.reserve(1 + 10 + 20 + 10 + 2 + 12);
            hdr += ' ';
            appendPaddedRight(hdr, "Time", 10);
            appendPaddedRight(hdr, "Operation", 20);
            appendPaddedLeft(hdr, "Change", 10);
            hdr += "  ";
            appendPaddedLeft(hdr, "Balance", 12);
            logElements.push_back(ftxui::text(std::move(hdr)) | ftxui::bold);
            logElements.push_back(ftxui::separator() | ftxui::dim);

            std::string line;
            for (const auto &e : _parsedLogs) {
                formatLogLine(line, e.time, e.operation, e.amount, e.balance);
                logElements.push_back(ftxui::text(line));
            }
        }

        auto logBox = ftxui::vbox(std::move(logElements))
                      | ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN, 5);

        auto logsCard = ftxui::vbox({
            ftxui::text(" Recent Activity") | ftxui::bold,
            ftxui::separator(),
            logBox,
        }) | ftxui::border;

        // ── Status line ────────────────────────────────────────
        auto errEl = _status.empty()
                         ? ftxui::emptyElement()
                         : ftxui::text(_status) | ftxui::center;

        // ── Assemble ────────────────────────────────────────────
        return ftxui::vbox({
                   ftxui::text("Dashboard") | ftxui::bold | ftxui::center,
                   ftxui::separator(),
                   ftxui::text(""),
                   balanceDisplay,
                   ftxui::text(""),
                   accountInfo,
                   ftxui::text(""),
                   logsCard,
                   errEl,
                   ftxui::filler(),
                   ftxui::text("[1] Dashboard  [2] History  [3] Deposit  [4] Withdraw  [5] Transfer  [6] Settings") | ftxui::dim | ftxui::center,
               })
               | ftxui::border;
    });

    return renderer;
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
}
