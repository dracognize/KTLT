#include "server/Server.hpp"

#include <csignal>

namespace {
	Server *g_server = nullptr;
}

extern "C" void handleSignal(int) {
	if (g_server)
		g_server->stop();
}

auto main() -> int {
	Server server(8080);
	g_server = &server;

	std::signal(SIGINT, handleSignal);
	std::signal(SIGTERM, handleSignal);

	server.run();
}
