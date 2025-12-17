cmake_minimum_required(VERSION 3.10)
project(OrderBook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(OrderBook
    src/main.cpp
)

target_include_directories(OrderBook PRIVATE include)

add_executable(OrderBookBenchmark
    src/benchmark.cpp
)

target_include_directories(OrderBookBenchmark PRIVATE include)
