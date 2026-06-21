#include "server/Server.hpp"

#include <asio.hpp>

auto main() -> int {
	Server server(8080);

	
	asio::signal_set signals(server.ioContext(), SIGINT, SIGTERM);
	signals.async_wait([&server](std::error_code , int ) { server.stop(); });

	server.run();
}
