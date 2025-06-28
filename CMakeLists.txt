cmake_minimum_required(VERSION 3.18)
project(command_parser LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
include_directories(include)

add_executable(command_parser
    src/main.cpp
    src/hash_util.cpp
    src/cli_parse.cpp
)

find_package(OpenSSL REQUIRED)
target_link_libraries(command_parser OpenSSL::Crypto OpenSSL::SSL)