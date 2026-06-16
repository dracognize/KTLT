#pragma once

#include <ftxui/component/component.hpp>
#include <functional>
#include <string>

class Client;

struct LoginPage {
		using Post = std::function<void(std::function<void()>)>;
		using Handler = std::function<void(std::string)>;

		LoginPage(Client &client, Post post, Handler onLogin);
		ftxui::Component build();
		void reset();

	private:
		void doLogin();

		Client &_client;
		Post _post;
		Handler _onLogin;
		std::string _username;
		std::string _password;
		std::string _status;
		bool _loading = false;
};
