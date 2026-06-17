#include "app/App.hpp"
#include "app/client/Client.hpp"
#include "app/pages/AccountSettingsPage.hpp"
#include "app/pages/DashboardPage.hpp"
#include "app/pages/LoginPage.hpp"
#include "app/pages/RegisterPage.hpp"
#include "app/pages/TransferPage.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <thread>

// page numbering convention:
// 0 = Login, 1 = Dashboard, 2 = Register, 3 = Account Settings, 4 = Transfer

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
	RegisterPage registerPage(client, screen, page);
	AccountSettingsPage settingsPage(client, screen, appUsername, page);
	TransferPage transferPage(client, screen, appUsername, page);

	loginPage.onEnterDashboard = [&] { dashboard.doRefresh(); };

	auto loginComp = loginPage.build();
	auto dashComp = dashboard.build();
	auto registerComp = registerPage.build();
	auto settingsComp = settingsPage.build();
	auto transferComp = transferPage.build();

	auto container = ftxui::Container::Tab(
		{loginComp, dashComp, registerComp, settingsComp, transferComp}, &page);
	
	screen.Loop(container);
	client.stop();
	clientThread.join();
}
