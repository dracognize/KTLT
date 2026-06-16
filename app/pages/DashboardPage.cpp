#include "app/pages/DashboardPage.hpp"
#include "app/client/Client.hpp"
#include "app/pages/LoginPage.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>

DashboardPage::DashboardPage(Client &client,
							 ftxui::ScreenInteractive &screen,
							 const std::string &username,
							 int &page,
							 LoginPage &loginPage)
	: _client(client), _screen(screen), _username(username), _page(page), _loginPage(loginPage) {
}

void DashboardPage::doRefresh() {
	_balanceStr = "Loading...";
	_client.getBalance(_username, [this](std::optional<u64> balance) {
		_screen.Post([this, balance] {
			if (balance) {
				_balanceStr = std::to_string(*balance);
				_status = "";
			} else {
				_balanceStr = "ERROR";
				_status = "Failed to fetch balance";
			}
			_screen.RequestAnimationFrame();
		});
	});
}

void DashboardPage::onLogout() {
	_loginPage.reset();
	_page = 0;
	_screen.RequestAnimationFrame();
}

void DashboardPage::onExit() {
	_screen.Exit();
}

ftxui::Component DashboardPage::build() {
	auto refreshBtn = ftxui::Button("Refresh", [this] { doRefresh(); });
	auto logoutBtn = ftxui::Button("Logout", [this] { onLogout(); });
	auto exitBtn = ftxui::Button("Exit", [this] { onExit(); });

	auto container = ftxui::Container::Vertical({
		refreshBtn,
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
				   logoutBtn->Render() | ftxui::center,
				   exitBtn->Render() | ftxui::center,
				   ftxui::text(_status) | ftxui::center,
			   })
			   | ftxui::border | ftxui::size(ftxui::WIDTH, ftxui::GREATER_THAN, 40);
	});
}
