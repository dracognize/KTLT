#include "server/Server.hpp"

auto main() -> int {
	Server server(8080);
	server.run();
}
