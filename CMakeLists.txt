cmake_minimum_required(VERSION 3.10)
project(first_grpc_project VERSION 0.1.0 LANGUAGES C CXX)

include_directories(CMAKE_SOURCE_DIR/include)

add_executable(first_grpc_project "${CMAKE_SOURCE_DIR}/src/main.cpp")

