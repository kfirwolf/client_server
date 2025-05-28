cmake_minimum_required(VERSION 3.10)
project(UDPClientServerTests)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/../INFRA/include)

# ---------------------
# INFRA library
# ---------------------
# Gather and configure INFRA library
file(GLOB INFRA_SOURCES "${CMAKE_SOURCE_DIR}/../INFRA/src/*.c")

# Create infra library target
add_library(net_infra ${INFRA_SOURCES})

# Make INFRA headers available to consumers
target_include_directories(net_infra PUBLIC "${CMAKE_SOURCE_DIR}/../INFRA/include")


# ---------------------
# CLIENT_SERVER library
# ---------------------
file(GLOB CLIENT_SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

add_library(client_server_lib ${CLIENT_SOURCES})

target_include_directories(client_server_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(client_server_lib PRIVATE net_infra)

# ---------------------
# Tests
# ---------------------
# Add test executable
add_executable(test_udp_client_server ${CMAKE_SOURCE_DIR}/tests/test_udp_client_server.cpp)

# Link the test executable with Google Test and networking modules.
# Note: Here, net_infra is listed before net_client and net_server.
target_link_libraries(test_udp_client_server PRIVATE GTest::GTest GTest::Main pthread gtest_main client_server_lib)

# Add test
add_test(NAME UDPClientServerTest COMMAND test_udp_client_server)

# Add test executable for the second test
add_executable(test_udp_server ${CMAKE_SOURCE_DIR}/tests/test_udp_server.cpp)

# Link the second test executable with Google Test and networking modules.
target_link_libraries(test_udp_server PRIVATE GTest::GTest GTest::Main pthread gtest_main client_server_lib)

# Add test for the second test
add_test(NAME UDPServerTest COMMAND test_udp_server)

# Add test executable
add_executable(test_ram_file_ctrl ${CMAKE_SOURCE_DIR}/tests/test_ram_file_ctrl.cpp)

# Link the 3th test executable with Google Test and networking modules.
target_link_libraries(test_ram_file_ctrl PRIVATE GTest::GTest GTest::Main pthread gtest_main client_server_lib)

# Add test for the 3th test
add_test(NAME RamFileCtrlTest COMMAND test_ram_file_ctrl)