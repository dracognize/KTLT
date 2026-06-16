#include "app/pages/DashboardPage.hpp"
#include "app/client/Client.hpp"

#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>

DashboardPage::DashboardPage(Client &client,
							 Post post,
							 std::function<std::string()> getUsername,
							 std::function<void()> onLogout,
							 std::function<void()> onExit)
	: _client(client), _post(std::move(post)), _getUsername(std::move(getUsername)),
	  _onLogout(std::move(onLogout)), _onExit(std::move(onExit)) {
}

void DashboardPage::doRefresh() {
	_balanceStr = "Loading...";
	_client.getBalance(_getUsername(), [this](std::optional<u64> balance) {
		_post([this, balance] {
			if (balance) {
				_balanceStr = std::to_string(*balance);
				_status = "";
			} else {
				_balanceStr = "ERROR";
				_status = "Failed to fetch balance";
			}
		});
	});
}

ftxui::Component DashboardPage::build() {
	auto refreshBtn = ftxui::Button("Refresh", [this] { doRefresh(); });
	auto logoutBtn = ftxui::Button("Logout", _onLogout);
	auto exitBtn = ftxui::Button("Exit", _onExit);

	auto container = ftxui::Container::Vertical({
		refreshBtn,
		logoutBtn,
		exitBtn,
	});

	return ftxui::Renderer(container, [this, refreshBtn, logoutBtn, exitBtn] {
		return ftxui::vbox({
				   ftxui::text("Dashboard") | ftxui::bold | ftxui::center,
				   ftxui::separator(),
				   ftxui::text(" Welcome, " + _getUsername() + "!"),
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
