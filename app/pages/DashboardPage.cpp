#include "app/pages/DashboardPage.hpp"
#include "app/Theme.hpp"
#include "app/client/Client.hpp"
#include "libs/base/string.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <charconv>
#include <cstdio>
#include <string>

DashboardPage::DashboardPage(Client &client,
                             ftxui::ScreenInteractive &screen,
                             const std::string &username)
    : _client(client), _screen(screen), _username(username) {
}

void DashboardPage::parseTransactionHistory(const std::string &raw,
											base::Vector<LogEntry> &parsed) {
	parsed.clear();

	if (raw.empty()) {
		return;
	}

	auto src = std::string_view{raw};
	if (!src.empty() && src.back() == '\n')
		src.remove_suffix(1);

	base::Vector<std::string_view> lines;
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

	parsed.reserve(lines.size());

	static constexpr const char *kOperationNames[] = {
		"Deposit",	   
		"Withdrawal",  
		"Transfer Out", 
		"Transfer In", 
	};

	for (auto i = 0; i < lines.size(); ++i) {
		auto line = lines[i];
		if (line.empty())
			continue;

		
		auto pipe1 = line.find('|');
		if (pipe1 == std::string_view::npos)
			continue;
		auto pipe2 = line.find('|', pipe1 + 1);
		if (pipe2 == std::string_view::npos)
			continue;
		auto pipe3 = line.find('|', pipe2 + 1);
		if (pipe3 == std::string_view::npos)
			continue;
		auto pipe4 = line.find('|', pipe3 + 1);
		if (pipe4 == std::string_view::npos)
			continue;

		auto typeStr = line.substr(0, pipe1);
		auto counterpartyStr = line.substr(pipe1 + 1, pipe2 - pipe1 - 1);
		auto amountStr = line.substr(pipe2 + 1, pipe3 - pipe2 - 1);
		auto balanceAfterStr = line.substr(pipe3 + 1, pipe4 - pipe3 - 1);
		auto timestampStr = line.substr(pipe4 + 1);

		unsigned int type = 0;
		std::from_chars(typeStr.data(), typeStr.data() + typeStr.size(), type);

		u64 amount = 0;
		std::from_chars(amountStr.data(), amountStr.data() + amountStr.size(), amount);

		u64 balanceAfter = 0;
		std::from_chars(balanceAfterStr.data(), balanceAfterStr.data() + balanceAfterStr.size(),
						balanceAfter);

		u64 timestamp = 0;
		std::from_chars(timestampStr.data(), timestampStr.data() + timestampStr.size(), timestamp);

		
		base::String operation;
		if (type < 4) {
			operation = base::String(kOperationNames[type]);
			if ((type == 2 || type == 3) && !counterpartyStr.empty()) {
				operation = base::String(std::string(kOperationNames[type]) + " " + std::string(counterpartyStr));
			}
		} else {
			operation = base::String("Unknown");
		}

		
		std::string amountRaw;
		if (type == 0 || type == 3) {
			amountRaw = "+" + std::to_string(amount);
		} else {
			amountRaw = std::to_string(amount);
		}
		base::String amountFormatted{std::string_view(amountRaw)};

		
		auto hrs = (timestamp / 3600) % 24;
		auto mins = (timestamp / 60) % 60;
		auto secs = timestamp % 60;
		char timeBuf[9];
		std::snprintf(timeBuf,
					  sizeof(timeBuf),
					  "%02llu:%02llu:%02llu",
					  static_cast<unsigned long long>(hrs),
					  static_cast<unsigned long long>(mins),
					  static_cast<unsigned long long>(secs));

		parsed.emplace_back(
			base::String(timeBuf), std::move(operation), std::move(amountFormatted), balanceAfter,
			base::String(counterpartyStr));
	}
}

void DashboardPage::doRefresh() {
    if (_logsStatus != "Loading...") {
        _balanceStr = "Loading...";
        _logsStatus = "Loading...";
        _status.clear();
    }

    _client.getTransactionHistory(_username, 5, [this](std::optional<std::string> resp) {
        auto parsed = base::Vector<LogEntry>{};
        auto ok = false;
        if (resp) {
            ok = true;
            parseTransactionHistory(*resp, parsed);
        }
        _screen.Post([this, parsed = std::move(parsed), ok] {
            if (ok) {
                _parsedLogs = std::move(parsed);
                _balanceHistory.clear();
                for (const auto &e : _parsedLogs) {
                    _balanceHistory.push_back(e.balance);
                }
                _balanceStr
                    = _balanceHistory.empty() ? "0" : std::to_string(_balanceHistory.back());
                _logsStatus.clear();
            } else {
                _parsedLogs.clear();
                _balanceHistory.clear();
                _balanceStr = "ERROR";
                _status = "Failed to fetch history";
                _logsStatus = "Failed to fetch history";
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
                _latencyVal = ms;
            } else {
                _latencyStr = "ERR";
                _latencyVal = -1;
            }
            _screen.RequestAnimationFrame();
        });
    });
}



ftxui::Component DashboardPage::build() {
    auto renderer = ftxui::Renderer([this]() -> ftxui::Element {
        
        auto latencyColor = theme::Green;
        if (_latencyVal > 100) latencyColor = theme::Yellow;
        if (_latencyVal > 500 || _latencyVal < 0) latencyColor = theme::Red;

        
        auto accountInfo = ftxui::vbox({
            ftxui::hbox({
                ftxui::text(" User: ") | ftxui::color(theme::Subtext0),
                ftxui::text(_username) | ftxui::bold | ftxui::color(theme::Mauve),
                ftxui::filler(),
                ftxui::text(" Latency: ") | ftxui::color(theme::Subtext0),
                ftxui::text(_latencyStr) | ftxui::bold | ftxui::color(latencyColor),
            }),
        }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface0);

        
        auto balanceDisplay = ftxui::hbox({
            ftxui::text(" Current Balance ") | ftxui::bold | ftxui::color(theme::Subtext1),
            ftxui::filler(),
            ftxui::text("$ ") | ftxui::color(theme::Yellow),
            ftxui::text(_balanceStr.empty() ? "0" : _balanceStr) | ftxui::bold | ftxui::color(theme::Yellow),
        }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Yellow)
           | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 30)
           | ftxui::center;

        
        auto logElements = ftxui::Elements();
        if (!_logsStatus.empty()) {
            logElements.push_back(ftxui::text(_logsStatus) | ftxui::dim | ftxui::center);
        } else if (_parsedLogs.empty()) {
            logElements.push_back(ftxui::text("No transactions yet") | ftxui::dim | ftxui::center);
        } else {
            
            logElements.push_back(ftxui::hbox({
                ftxui::text(" TIME") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
                ftxui::text(" OPERATION") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32),
                ftxui::text(" CHANGE") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
                ftxui::filler(),
                ftxui::text(" BALANCE ") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12),
            }) | ftxui::bold | ftxui::color(theme::Subtext1));
            logElements.push_back(ftxui::separator() | ftxui::color(theme::Surface1));

            for (const auto &e : _parsedLogs) {
                auto opColor = theme::Text;
                if (e.operation.view().find("Deposit") != std::string::npos || 
                    e.operation.view().find("Transfer In") != std::string::npos) {
                    opColor = theme::Green;
                } else if (e.operation.view().find("Withdraw") != std::string::npos || 
                           e.operation.view().find("Transfer Out") != std::string::npos) {
                    opColor = theme::Red;
                }

                logElements.push_back(ftxui::hbox({
                    ftxui::text(" " + std::string(e.time.view())) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12) | ftxui::color(theme::Overlay2),
                    ftxui::text(" " + std::string(e.operation.view())) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 32) | ftxui::color(opColor),
                    ftxui::text(std::string(e.amount.view())) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12) | ftxui::color(opColor) | ftxui::align_right,
                    ftxui::filler(),
                    ftxui::text(std::to_string(e.balance) + " ") | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 12) | ftxui::align_right | ftxui::color(theme::Text),
                }));
            }
        }

        auto logsCard = ftxui::vbox({
            ftxui::text(" Recent Activity") | ftxui::bold | ftxui::color(theme::Sapphire),
            ftxui::separator() | ftxui::color(theme::Sapphire),
            ftxui::vbox(std::move(logElements)) | ftxui::size(ftxui::HEIGHT, ftxui::GREATER_THAN, 5),
        }) | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1);

		
		auto graphElement = ftxui::emptyElement();
		if (_balanceHistory.size() >= 2) {
			graphElement = ftxui::vbox({
							   ftxui::text(" Balance History") | ftxui::bold | ftxui::color(theme::Teal),
							   ftxui::separator() | ftxui::color(theme::Teal),
							   ftxui::graph([this](int width, int height) {
								   std::vector<int> out(width);
								   if (_balanceHistory.empty())
									   return out;
								   u64 minBal = _balanceHistory[0];
								   u64 maxBal = _balanceHistory[0];
								   for (auto b : _balanceHistory) {
									   if (b < minBal)
										   minBal = b;
									   if (b > maxBal)
										   maxBal = b;
								   }
								   for (int i = 0; i < width; ++i) {
									   if (_balanceHistory.size() < 2) {
										   out[i] = height / 2;
										   continue;
									   }
									   float t = (float)i / (width - 1);
									   float idxF = t * (_balanceHistory.size() - 1);
									   size_t idx = (size_t)idxF;
									   float mix = idxF - idx;

									   u64 val = _balanceHistory[idx];
									   if (idx + 1 < _balanceHistory.size()) {
										   val = (u64)((1.0f - mix) * _balanceHistory[idx]
													   + mix * _balanceHistory[idx + 1]);
									   }

									   if (maxBal == minBal) {
										   out[i] = height / 2;
									   } else {
										   out[i]
											   = (int)((float)(val - minBal) / (maxBal - minBal)
													   * (height - 1));
									   }
								   }
								   return out;
							   }) | ftxui::color(theme::Teal)
								   | ftxui::flex,
						   })
						   | ftxui::borderStyled(ftxui::ROUNDED) | ftxui::color(theme::Surface1)
						   | ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 10);
		}

        
        auto errEl = _status.empty() ? ftxui::emptyElement() : ftxui::text(_status) | ftxui::color(theme::Red) | ftxui::center;

        return ftxui::vbox({
                   ftxui::text("BANKING DASHBOARD") | ftxui::bold | ftxui::center | ftxui::color(theme::Mauve),
                   ftxui::separator() | ftxui::color(theme::Mauve),
                   ftxui::text(""),
                   ftxui::hbox({
                       balanceDisplay | ftxui::flex,
                       ftxui::text("  "),
                       accountInfo | ftxui::flex,
                   }),
                   ftxui::text(""),
                   graphElement,
                   ftxui::text(""),
                   logsCard | ftxui::flex,
                   errEl,
                   ftxui::filler(),
                   ftxui::hbox({
                       ftxui::text(" Ctrl+Arrows or [ ] to switch tabs ") | ftxui::dim,
                       ftxui::filler(),
                       ftxui::text(" Press 'R' to refresh data ") | ftxui::dim,
                   }),
               });
    });

    auto component = ftxui::CatchEvent(renderer, [this](ftxui::Event e) {
        if (e == ftxui::Event::Character('r') || e == ftxui::Event::Character('R')) {
            doRefresh();
            return true;
        }
        return false;
    });
    return component;
}
