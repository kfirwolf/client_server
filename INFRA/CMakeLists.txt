cmake_minimum_required(VERSION 3.10)
project(INFRA_Tests)

# Enable testing
enable_testing()

# Find GoogleTest
find_package(GTest REQUIRED)
include(GoogleTest)

# Collect source files
file(GLOB INFRA_SOURCES "${CMAKE_SOURCE_DIR}/src/*.c")

# Create the infra library
add_library(net_infra ${INFRA_SOURCES})
target_include_directories(net_infra PUBLIC "${CMAKE_SOURCE_DIR}/include")

# Add test executable
add_executable(id_pool_test "${CMAKE_SOURCE_DIR}/tests/id_pool_test.cpp")

# Link against GoogleTest and the infra library
target_link_libraries(id_pool_test PRIVATE GTest::GTest GTest::Main pthread net_infra)

# Discover individual tests in the executable
gtest_discover_tests(id_pool_test)
