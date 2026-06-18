#pragma once

#include "libs/base/string.hpp"
#include "libs/base/vector.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <vector>

class Client;
struct DashboardPage;

struct TransferPage {
		TransferPage(Client &client,
					 ftxui::ScreenInteractive &screen,
					 const std::string &username,
					 int &postAuthPage,
					 DashboardPage &dashboard);
		ftxui::Component build();

	private:
		void doTransfer();
		void onRecipientChanged();
		void fetchUserDirectory();
		void filterSuggestions();
		void requestConfirmation();

		Client &_client;
		ftxui::ScreenInteractive &_screen;
		DashboardPage &_dashboard;

		const std::string &_username;
		int &_postAuthPage;

		std::string _recipient;
		ftxui::Component _recipientInput;
		std::string _amountStr;
		ftxui::Component _amountInput;
		ftxui::Component _transferBtn;
		ftxui::Component _backBtn;

		ftxui::Component _confirmYes;
		ftxui::Component _confirmNo;

		std::string _status;
		bool _loading = false;
		int _spinnerFrame = 0;
		bool _showConfirm = false;

		// ── User directory / autocomplete ──
		base::Vector<base::String> _userCache;
		base::Vector<base::String> _suggestions;
		std::vector<std::string> _suggestionStrings;
		ftxui::Component _suggestionsMenu;
		int _selectedSuggestion = 0;
		bool _showSuggestions = false;
		bool _cacheLoaded = false;
		bool _cacheLoading = false;
};
