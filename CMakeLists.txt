cmake_minimum_required(VERSION 3.14)
project(splitwise-plusplus VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include third_party)

set(SPLITWISE_SOURCES
    src/balance_sheet.cpp
    src/expense.cpp
    src/group.cpp
    src/main.cpp
    src/split_strategy.cpp
    src/split_strategy_factory.cpp
    src/splitwise_manager.cpp
    src/user.cpp)

add_library(splitwise_core STATIC
    src/balance_sheet.cpp
    src/expense.cpp
    src/group.cpp
    src/split_strategy.cpp
    src/split_strategy_factory.cpp
    src/splitwise_manager.cpp
    src/user.cpp)

target_include_directories(splitwise_core PUBLIC include third_party)

add_executable(splitwise src/main.cpp)
target_link_libraries(splitwise PRIVATE splitwise_core)

add_executable(tests
    tests/strategy_tests.cpp
    tests/reconciliation_tests.cpp)
target_link_libraries(tests PRIVATE splitwise_core)

enable_testing()
add_test(NAME splitwise_tests COMMAND tests)

