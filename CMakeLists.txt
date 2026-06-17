cmake_minimum_required(VERSION 4.2)
project(Study LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ftxui REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_COLOR_DIAGNOSTICS ON)

add_library(network
				libs/network/Packet.cpp
)

target_include_directories(network PUBLIC
				${CMAKE_SOURCE_DIR}
)

target_link_libraries(network PUBLIC
				Threads::Threads
)


add_executable(app
                app/main.cpp
                app/App.cpp
                app/client/Client.cpp
                app/pages/LoginPage.cpp
                app/pages/DashboardPage.cpp
                app/pages/RegisterPage.cpp
                app/pages/AccountSettingsPage.cpp
                app/pages/TransferPage.cpp
)

target_include_directories(app PUBLIC
				${CMAKE_SOURCE_DIR}
)

target_link_libraries(app PUBLIC
				network
				ftxui::screen
				ftxui::dom
				ftxui::component
				Threads::Threads
)


add_executable(server
				server/main.cpp
				server/Database.cpp
				server/Server.cpp
				server/ServerLog.cpp
)

target_include_directories(server PUBLIC
				${CMAKE_SOURCE_DIR}
)

target_link_libraries(server PUBLIC
				network
				Threads::Threads
)

if(WIN32)
    target_link_libraries(app PUBLIC ws2_32 mswsock)
    target_link_libraries(server PRIVATE ws2_32 mswsock)
endif()


add_executable(generate_test_data
				tests/generate_test_data.cpp
)

target_include_directories(generate_test_data PUBLIC
				${CMAKE_SOURCE_DIR}
)