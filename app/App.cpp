#include "app/App.hpp"
#include "app/client/Client.hpp"
#include "app/pages/AccountSettingsPage.hpp"
#include "app/pages/DashboardPage.hpp"
#include "app/pages/DepositPage.hpp"
#include "app/pages/HistoryPage.hpp"
#include "app/pages/LoginPage.hpp"
<<<<<<< HEAD
#include "app/pages/RegisterPage.hpp"
#include "app/pages/TransferPage.hpp"
=======
#include "app/pages/SettingPage.hpp"
#include "app/pages/SignupPage.hpp"
#include "app/pages/TransferPage.hpp"
#include "app/pages/WithdrawPage.hpp"
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <string>
#include <thread>

<<<<<<< HEAD
// page numbering convention:
// 0 = Login, 1 = Dashboard, 2 = Register, 3 = Account Settings, 4 = Transfer
=======
// ── Non-focusable wrapper ─────────────────────────────────────────────
// Wraps a component so it renders and processes mouse events,
// but does NOT participate in Tab-key focus cycling.
class NonFocusable final : public ftxui::ComponentBase {
	public:
		explicit NonFocusable(ftxui::Component child) {
			Add(std::move(child));
		}

		bool Focusable() const override {
			return false;
		}

		bool OnEvent(ftxui::Event event) override {
			// Forward all events (keyboard + mouse) to the wrapped child.
			// The Menu inside still processes mouse clicks and arrow keys.
			if (ChildCount() > 0) {
				return ChildAt(0)->OnEvent(std::move(event));
			}
			return false;
		}
};
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

void App::run() {
	Client client("127.0.0.1", 8080);

<<<<<<< HEAD
	auto screen = ftxui::ScreenInteractive::TerminalOutput();
=======
	auto screen = ftxui::ScreenInteractive::Fullscreen();
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
	std::thread clientThread([&] { client.run(); });

	client.connect([&](bool ok) {
		if (!ok)
			screen.Exit();
	});

	std::string appUsername;
<<<<<<< HEAD
	int page = 0;

	LoginPage loginPage(client, screen, appUsername, page);
	DashboardPage dashboard(client, screen, appUsername, page, loginPage);
	RegisterPage registerPage(client, screen, page);
	AccountSettingsPage settingsPage(client, screen, appUsername, page);
	TransferPage transferPage(client, screen, appUsername, page);
=======
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
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)

	loginPage.onEnterDashboard = [&] { dashboard.doRefresh(); };

<<<<<<< HEAD
	auto loginComp = loginPage.build();
	auto dashComp = dashboard.build();
	auto registerComp = registerPage.build();
	auto settingsComp = settingsPage.build();
	auto transferComp = transferPage.build();

	auto container = ftxui::Container::Tab(
		{loginComp, dashComp, registerComp, settingsComp, transferComp}, &page);
	
	screen.Loop(container);
=======
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
	auto preAuthContent = ftxui::Container::Tab({loginComp, signupComp}, &preAuthPage);

	std::vector<std::string> preAuthEntries = {"Login", "Sign Up"};
	auto preAuthMenu
		= ftxui::Menu(preAuthEntries, &preAuthPage, ftxui::MenuOption::HorizontalAnimated());

	auto preAuthContainer = ftxui::Container::Vertical({
		ftxui::Renderer([] {
			return ftxui::text("KTLT Banking Terminal") | ftxui::bold | ftxui::center
				   | ftxui::size(ftxui::WIDTH, ftxui::LESS_THAN, 52) | ftxui::center;
		}),
		preAuthMenu,
		preAuthContent,
	});

	// ── Post-Auth ─────────────────────────────────────────────────────
	auto postAuthContent
		= ftxui::Container::Tab(
			  {dashComp, historyComp, depositComp, withdrawComp, transferComp, settingComp},
			  &postAuthPage)
		  | ftxui::flex;

	std::vector<std::string> postAuthEntries
		= {"Dashboard", "History", "Deposit", "Withdraw", "Transfer", "Settings"};

	// Centralised page-change handler — called by both Menu::on_change
	// AND keyboard shortcuts, because CatchEvent intercepts keyboard
	// events before they reach the Menu component.
	auto onPostAuthPageChanged = [&] {
		if (postAuthPage == 1) {
			historyPage.doRefresh();
		}
	};

	auto postAuthMenuOption = ftxui::MenuOption::HorizontalAnimated();
	postAuthMenuOption.on_change = onPostAuthPageChanged;

	auto postAuthMenu = ftxui::Menu(postAuthEntries, &postAuthPage, postAuthMenuOption);

	auto postAuthContainer = ftxui::Container::Vertical({
		postAuthMenu,
		postAuthContent,
	});

	// ── Top-Level Tab ────────────────────────────────────────────────
	auto container
		= ftxui::Container::Tab({preAuthContainer, postAuthContainer}, &section) | ftxui::border;

	// ── Global Keyboard Shortcuts ────────────────────────────────────
	auto app = ftxui::CatchEvent(container, [&](ftxui::Event event) -> bool {
		// ESC: quit
		if (event == ftxui::Event::Escape) {
			screen.Exit();
			return true;
		}

		// Ctrl+Left / Ctrl+Right: switch pages
		if (event == ftxui::Event::ArrowLeftCtrl) {
			if (section == 0) {
				// Pre-auth: 0=Login, 1=Sign Up
				if (preAuthPage > 0)
					preAuthPage--;
			} else {
				// Post-auth: 0..5
				if (postAuthPage > 0) {
					postAuthPage--;
					onPostAuthPageChanged();
				}
			}
			return true;
		}

		if (event == ftxui::Event::ArrowRightCtrl) {
			if (section == 0) {
				// Pre-auth: 0=Login, 1=Sign Up
				if (preAuthPage < 1)
					preAuthPage++;
			} else {
				// Post-auth: 0..5
				if (postAuthPage < 5) {
					postAuthPage++;
					onPostAuthPageChanged();
				}
			}
			return true;
		}

		if (event.is_character()) {
			auto c = event.character();
			if (c == "[") {
				if (section == 0) {
					if (preAuthPage > 0)
						preAuthPage--;
				} else {
					if (postAuthPage > 0) {
						postAuthPage--;
						onPostAuthPageChanged();
					}
				}
				return true;
			}
			if (c == "]") {
				if (section == 0) {
					if (preAuthPage < 1)
						preAuthPage++;
				} else {
					if (postAuthPage < 5) {
						postAuthPage++;
						onPostAuthPageChanged();
					}
				}
				return true;
			}
		}

		return false;
	});

	screen.Loop(app);
>>>>>>> 4758aae (Implement full banking terminal with comprehensive TUI and secure backend)
	client.stop();
	clientThread.join();
}
