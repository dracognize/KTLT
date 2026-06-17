#include "app/App.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"
#include "app/pages/LoginPage.hpp"
#include "app/pages/SignupPage.hpp"
#include "ftxui/dom/elements.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <thread>

void App::run() {
	Client client("127.0.0.1", 8080);

	auto screen = ftxui::ScreenInteractive::FullscreenAlternateScreen();
	std::thread clientThread([&] { client.run(); });

	client.connect([&](bool ok) {
		if (!ok)
			screen.Exit();
	});

	std::string appUsername;
	int section = 0;
	int preAuthPage = 0;

	LoginPage loginPage(client, screen, appUsername, section);
	SignupPage signupPage(client, screen, appUsername, section);
	DashboardPage dashboard(client, screen, appUsername, section, loginPage);

	loginPage.setDashboard(dashboard);
	signupPage.setDashboard(dashboard);

	auto loginComp = loginPage.build();
	auto dashComp = dashboard.build();
	auto signupComp = signupPage.build();

	auto preAuthContent = ftxui::Container::Tab({loginComp, signupComp}, &preAuthPage);

	std::vector<std::string> preAuthContentNavigatorEntry = {"Login", "Sign Up"};

	auto preAuthContentNavigator
		= ftxui::Menu(preAuthContentNavigatorEntry, &preAuthPage, ftxui::MenuOption::Horizontal())
		  | ftxui::border;

	auto preAuthContainer = ftxui::Container::Vertical({preAuthContentNavigator, preAuthContent});

	auto container = ftxui::Container::Tab({preAuthContainer, dashComp}, &section) | ftxui::border;

	auto app = ftxui::CatchEvent(container, [this, &screen](ftxui::Event event) -> bool {
		if (event == ftxui::Event::Escape) {
			screen.Exit();
			return true;
		}
		return false;
	});

	screen.Loop(app);
	client.stop();
	clientThread.join();
}
