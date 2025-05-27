cmake_minimum_required(VERSION 3.10)
project(IdPoolProject C CXX)

# Specify C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# make <common_defs.h> visible to *all* targets in this directory
include_directories(${PROJECT_SOURCE_DIR}/../include)

# Add your id_pool library (C)
add_library(id_pool STATIC
    src/id_pool.c
)

target_include_directories(id_pool PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${COMMON_INCLUDE_DIR}            # <-- so id_pool can #include <common_defs.h>
)

# GoogleTest integration
enable_testing()
find_package(GTest REQUIRED)

# Add test executable (C++)
add_executable(id_pool_test
    tests/id_pool_test.cpp
)

target_include_directories(id_pool_test PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${COMMON_INCLUDE_DIR}             # <-- so tests can #include <common_defs.h>
)

# Link test executable with id_pool, GTest main (which provides main), and pthread
# GTest::gtest_main includes the main() entrypoint for tests
target_link_libraries(id_pool_test
    PRIVATE
        id_pool
        GTest::gtest_main
        pthread
)

# Let Google Test discover tests
include(GoogleTest)
gtest_discover_tests(id_pool_test)