#include "app/App.hpp"
#include "app/client/Client.hpp"
#include "app/pages/DashboardPage.hpp"
#include "app/pages/LoginPage.hpp"

#include <ftxui/component/screen_interactive.hpp>
#include <memory>
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

	auto post = [&](auto fn) {
		screen.Post([&, fn = std::move(fn)] {
			fn();
			screen.RequestAnimationFrame();
		});
	};

	std::string loggedInUser;
	int page = 0;

	auto loginPage = std::make_shared<LoginPage>(client, post, [&](std::string user) {
		loggedInUser = std::move(user);
		page = 1;
		screen.RequestAnimationFrame();
	});

	auto dashboard = std::make_shared<DashboardPage>(
		client,
		post,
		[&] { return loggedInUser; },
		[&] {
			loggedInUser.clear();
			loginPage->reset();
			page = 0;
			screen.RequestAnimationFrame();
		},
		[&] { screen.Exit(); });

	auto loginComp = loginPage->build();
	auto dashComp = dashboard->build();

	auto container = ftxui::Container::Tab({loginComp, dashComp}, &page);

	screen.Loop(container);
	client.stop();
	clientThread.join();
}
