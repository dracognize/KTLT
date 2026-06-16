#pragma once

#include <ftxui/component/component.hpp>
#include <functional>
#include <string>

class Client;

struct DashboardPage {
		using Post = std::function<void(std::function<void()>)>;

		DashboardPage(Client &client,
					  Post post,
					  std::function<std::string()> getUsername,
					  std::function<void()> onLogout,
					  std::function<void()> onExit);
		ftxui::Component build();

	private:
		void doRefresh();

		Client &_client;
		Post _post;
		std::function<std::string()> _getUsername;
		std::function<void()> _onLogout;
		std::function<void()> _onExit;
		std::string _balanceStr;
		std::string _status;
};
