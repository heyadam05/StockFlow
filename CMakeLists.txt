cmake_minimum_required(VERSION 3.16)
project(StockFlow VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(STOCKFLOW_SOURCES
    src/Models.cpp
    src/Utils.cpp
    src/Inventory.cpp
    src/Storage.cpp
)

add_library(stockflow_core ${STOCKFLOW_SOURCES})
target_include_directories(stockflow_core PUBLIC include)

add_executable(stockflow src/main.cpp src/Application.cpp)
target_link_libraries(stockflow PRIVATE stockflow_core)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(stockflow_core PUBLIC stdc++fs)
endif()

enable_testing()
add_executable(stockflow_tests tests/tests.cpp)
target_link_libraries(stockflow_tests PRIVATE stockflow_core)
add_test(NAME stockflow_tests COMMAND stockflow_tests)

if(MSVC)
    target_compile_options(stockflow_core PRIVATE /W4 /permissive-)
    target_compile_options(stockflow PRIVATE /W4 /permissive-)
else()
    target_compile_options(stockflow_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(stockflow PRIVATE -Wall -Wextra -Wpedantic)
endif()
