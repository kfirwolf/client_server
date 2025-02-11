cmake_minimum_required(VERSION 3.10)
project(UDPClientServerTests)

# Enable testing
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)

# Add the networking library sources
add_library(net_client ${CMAKE_SOURCE_DIR}/src/net_client.c)
add_library(net_server ${CMAKE_SOURCE_DIR}/src/net_server.c)
add_library(net_infra ${CMAKE_SOURCE_DIR}/src/net_infra.c)
add_library(ram_file_ctrl ${CMAKE_SOURCE_DIR}/src/ram_file_ctrl.c)

# Link libraries for net_client and net_server
target_link_libraries(net_client PRIVATE net_infra)
target_link_libraries(net_server PRIVATE net_infra)


# Add test executable
add_executable(test_udp_client_server ${CMAKE_SOURCE_DIR}/tests/test_udp_client_server.cpp)

# Link the test executable with Google Test and networking modules.
# Note: Here, net_infra is listed before net_client and net_server.
target_link_libraries(test_udp_client_server PRIVATE ${GTEST_LIBRARIES} pthread gtest_main net_infra net_client net_server)

# Add test
add_test(NAME UDPClientServerTest COMMAND test_udp_client_server)


# Add test executable for the second test
add_executable(test_udp_server ${CMAKE_SOURCE_DIR}/tests/test_udp_server.cpp)

# Link the second test executable with Google Test and networking modules.
target_link_libraries(test_udp_server PRIVATE ${GTEST_LIBRARIES} pthread gtest_main net_infra net_server)

# Add test for the second test
add_test(NAME UDPServerTest COMMAND test_udp_server)



# Add test executable
add_executable(test_ram_file_ctrl ${CMAKE_SOURCE_DIR}/tests/test_ram_file_ctrl.cpp)

# Link the 3th test executable with Google Test and networking modules.
target_link_libraries(test_ram_file_ctrl PRIVATE ${GTEST_LIBRARIES} pthread gtest_main ram_file_ctrl)

# Add test for the 3th test
add_test(NAME RamFileCtrlTest COMMAND test_ram_file_ctrl)