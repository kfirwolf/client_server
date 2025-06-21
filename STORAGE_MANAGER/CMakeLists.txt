cmake_minimum_required(VERSION 3.10)
project(store_manager_Tests)

enable_testing()

find_package(GTest REQUIRED)

# INFRA library
file(GLOB INFRA_SOURCES "../INFRA/src/*.c")
add_library(net_infra ${INFRA_SOURCES})
target_include_directories(net_infra PUBLIC "../INFRA/include")

# RAM FILE CTRL library
file(GLOB RAM_FILE_CTRL_SOURCES "../CLIENT_SERVER/src/ram_file_ctrl.c")
add_library(ram_file_ctrl ${RAM_FILE_CTRL_SOURCES})
target_include_directories(ram_file_ctrl PUBLIC "../CLIENT_SERVER/include")
target_link_libraries(ram_file_ctrl PRIVATE net_infra)

# BUDDY ALLOCATOR library
file(GLOB BUDDY_ALLOCATOR_SOURCES "../BUDDY_ALLOCATOR/src/*.c")
add_library(buddy_allocator ${BUDDY_ALLOCATOR_SOURCES})
target_include_directories(buddy_allocator PUBLIC "../BUDDY_ALLOCATOR/include")
target_link_libraries(buddy_allocator PRIVATE net_infra)


# STORAGE MANAGER library
file(GLOB STORAGE_MANAGER_SOURCES "src/*.c")
add_library(store_mngr_lib ${STORAGE_MANAGER_SOURCES})
target_include_directories(store_mngr_lib PUBLIC "include")
target_link_libraries(store_mngr_lib PRIVATE net_infra ram_file_ctrl buddy_allocator)

# Tests
add_executable(test_store_mngr "tests/store_mngr.cpp")
target_include_directories(test_store_mngr PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(test_store_mngr
    PRIVATE
      store_mngr_lib
      net_infra
      ram_file_ctrl
      buddy_allocator
      GTest::Main
      pthread
)

add_test(NAME store_mngr_test COMMAND test_store_mngr --gtest_output=stdout --gtest_color=yes)