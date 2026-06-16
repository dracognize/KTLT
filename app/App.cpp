#include "app/App.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"
#include "app/pages/LoginPage.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <thread>

void App::run() {
	Client client("127.0.0.1", 8080);

	auto screen = ftxui::ScreenInteractive::TerminalOutput();
	std::thread clientThread([&] { client.run(); });

	client.connect([&](bool ok) {
		if (!ok)
			screen.Exit();
	});

	std::string appUsername;
	int page = 0;

	LoginPage loginPage(client, screen, appUsername, page);
	DashboardPage dashboard(client, screen, appUsername, page, loginPage);

	loginPage.onEnterDashboard = [&] { dashboard.doRefresh(); };

	auto loginComp = loginPage.build();
	auto dashComp = dashboard.build();

	auto container = ftxui::Container::Tab({loginComp, dashComp}, &page);

	screen.Loop(container);
	client.stop();
	clientThread.join();
}
