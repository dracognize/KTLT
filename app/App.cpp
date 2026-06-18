#include "app/App.hpp"
#include "app/Theme.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"
#include "app/pages/DepositPage.hpp"
#include "app/pages/HistoryPage.hpp"
#include "app/pages/LoginPage.hpp"
#include "app/pages/SettingPage.hpp"
#include "app/pages/SignupPage.hpp"
#include "app/pages/TransferPage.hpp"
#include "app/pages/WithdrawPage.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <string>
#include <thread>

void App::run() {
	Client client("127.0.0.1", 8080);

	auto screen = ftxui::ScreenInteractive::Fullscreen();
	std::thread clientThread([&] { client.run(); });

	client.connect([&](bool ok) {
		if (!ok)
			screen.Exit();
	});

	std::string appUsername;
	int section = 0;
	int preAuthPage = 0;
	int postAuthPage = 0;

	LoginPage loginPage(client, screen, appUsername, section);
	SignupPage signupPage(client, screen, appUsername, section);
	DashboardPage dashboard(client, screen, appUsername);
	HistoryPage historyPage(client, screen, appUsername, dashboard);
	DepositPage depositPage(client, screen, appUsername, postAuthPage, dashboard);
	WithdrawPage withdrawPage(client, screen, appUsername, postAuthPage, dashboard);
	TransferPage transferPage(client, screen, appUsername, postAuthPage, dashboard);
	SettingPage settingPage(
		client,
		screen,
		appUsername,
		postAuthPage,
		[&] {
			loginPage.reset();
			postAuthPage = 0;
			section = 0;
		},
		dashboard);

	loginPage.setDashboard(dashboard);

	client.onDisconnect([&] {
		screen.Post([&] {
			loginPage.reset();
			loginPage.setStatus("Connection to server lost");
			postAuthPage = 0;
			section = 0;
			screen.RequestAnimationFrame();
		});
	});

	auto loginComp = loginPage.build();
	auto signupComp = signupPage.build();
	auto dashComp = dashboard.build();
	auto historyComp = historyPage.build();
	auto depositComp = depositPage.build();
	auto withdrawComp = withdrawPage.build();
	auto transferComp = transferPage.build();
	auto settingComp = settingPage.build();

	// ── Pre-Auth ──────────────────────────────────────────────────────
	auto preAuthTabs = ftxui::Container::Tab({loginComp, signupComp}, &preAuthPage);
	auto preAuthContent = preAuthTabs;

	std::vector<std::string> preAuthEntries = {"Login", "Sign Up"};
	auto preAuthMenuOption = ftxui::MenuOption::HorizontalAnimated();
    preAuthMenuOption.entries_option.transform = [](const ftxui::EntryState& s) {
        auto element = ftxui::text(s.label) | ftxui::center;
        if (s.active) {
            return element | ftxui::bold | ftxui::color(theme::Base) | ftxui::bgcolor(theme::Mauve);
        }
        if (s.focused) {
            return element | ftxui::bold | ftxui::color(theme::Mauve) | ftxui::bgcolor(theme::Surface1);
        }
        return element | ftxui::color(theme::Overlay2);
    };

	auto preAuthMenu
		= ftxui::Menu(preAuthEntries, &preAuthPage, preAuthMenuOption);

	auto preAuthContainer = ftxui::Container::Vertical({
		ftxui::Renderer([] {
			return ftxui::text("KTLT Banking Terminal") | ftxui::bold | ftxui::center
				   | ftxui::color(theme::Mauve)
				   | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 52) | ftxui::center;
		}),
		preAuthMenu,
		preAuthContent,
	});

	// ── Post-Auth ─────────────────────────────────────────────────────
	auto postAuthTabs = ftxui::Container::Tab(
			  {dashComp, historyComp, depositComp, withdrawComp, transferComp, settingComp},
			  &postAuthPage);
	auto postAuthContent = postAuthTabs | ftxui::flex;

	std::vector<std::string> postAuthEntries
		= {"Dashboard", "History", "Deposit", "Withdraw", "Transfer", "Settings"};

	auto onPostAuthPageChanged = [&] {
		if (postAuthPage == 0) {
			dashboard.doRefresh();
		} else if (postAuthPage == 1) {
			historyPage.doRefresh();
		}
		postAuthTabs->SetActiveChild(postAuthTabs->ChildAt(postAuthPage));
	};

	auto postAuthMenuOption = ftxui::MenuOption::HorizontalAnimated();
    postAuthMenuOption.on_change = onPostAuthPageChanged;
    postAuthMenuOption.entries_option.transform = [](const ftxui::EntryState& s) {
        auto element = ftxui::text(s.label) | ftxui::center;
        if (s.focused) {
            return element | ftxui::bold | ftxui::color(theme::Base) | ftxui::bgcolor(theme::Mauve);
        }
        if (s.active) {
            return element | ftxui::bold | ftxui::color(theme::Mauve);
        }
        return element | ftxui::color(theme::Overlay2);
    };

	auto postAuthMenu = ftxui::Menu(postAuthEntries, &postAuthPage, postAuthMenuOption);

	auto postAuthContainer = ftxui::Container::Vertical({
		postAuthMenu,
		postAuthContent,
	});

	// ── Top-Level Tab ────────────────────────────────────────────────
	auto topTabs
		= ftxui::Container::Tab({preAuthContainer, postAuthContainer}, &section);

	auto container = ftxui::Renderer(topTabs, [&] {
		return topTabs->Render()
			| ftxui::color(theme::Text)
			| ftxui::bgcolor(theme::Base)
			| ftxui::borderStyled(ftxui::ROUNDED)
			| ftxui::color(theme::Mauve);
	});

	// ── Global Keyboard Shortcuts ────────────────────────────────────
	auto app = ftxui::CatchEvent(container, [&](ftxui::Event event) -> bool {
		if (event == ftxui::Event::Escape) {
			screen.Exit();
			return true;
		}
        
        if (event == ftxui::Event::Character('r') || event == ftxui::Event::Character('R')) {
            if (section == 1) {
                if (postAuthPage == 0) dashboard.doRefresh();
                else if (postAuthPage == 1) historyPage.doRefresh();
                // Add more as needed
                return true;
            }
        }

		if (event == ftxui::Event::ArrowLeftCtrl || event == ftxui::Event::Character('[')) {
			if (section == 0) {
				if (preAuthPage > 0) {
					preAuthPage--;
					preAuthTabs->SetActiveChild(preAuthTabs->ChildAt(preAuthPage));
				}
			} else {
				if (postAuthPage > 0) {
					postAuthPage--;
					onPostAuthPageChanged();
				}
			}
			return true;
		}

		if (event == ftxui::Event::ArrowRightCtrl || event == ftxui::Event::Character(']')) {
			if (section == 0) {
				if (preAuthPage < 1) {
					preAuthPage++;
					preAuthTabs->SetActiveChild(preAuthTabs->ChildAt(preAuthPage));
				}
			} else {
				if (postAuthPage < 5) {
					postAuthPage++;
					onPostAuthPageChanged();
				}
			}
			return true;
		}

		return false;
	});

	screen.Loop(app);
	client.stop();
	clientThread.join();
}
