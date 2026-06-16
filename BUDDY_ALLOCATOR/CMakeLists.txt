cmake_minimum_required(VERSION 3.10)
project(UDPClientServerTests)

enable_testing()

find_package(GTest REQUIRED)

# INFRA library
file(GLOB INFRA_SOURCES "../INFRA/src/*.c")
add_library(net_infra ${INFRA_SOURCES})
target_include_directories(net_infra PUBLIC "../INFRA/include")

# BUDDY ALLOCATOR library
file(GLOB BUDDY_ALLOC_SOURCES "src/*.c")
add_library(buddy_allocator_lib ${BUDDY_ALLOC_SOURCES})
target_include_directories(buddy_allocator_lib PUBLIC "include")
target_link_libraries(buddy_allocator_lib PRIVATE net_infra)

# Tests
add_executable(test_buddy_allocator "tests/buddy_allocator.cpp")
target_include_directories(test_buddy_allocator PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(test_buddy_allocator
    PRIVATE
      buddy_allocator_lib
      net_infra
      GTest::Main
      pthread
)

add_test(NAME BuddyAllocatorTest COMMAND test_buddy_allocator --gtest_output=stdout --gtest_color=yes)