#include "server/Server.hpp"

#include <asio.hpp>

auto main() -> int {
	Server server(8080);

	// Graceful shutdown on SIGINT/SIGTERM using Asio's signal_set
	asio::signal_set signals(server.ioContext(), SIGINT, SIGTERM);
	signals.async_wait([&server](std::error_code /*ec*/, int /*sig*/) { server.stop(); });

	server.run();
}
