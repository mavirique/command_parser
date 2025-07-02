cmake_minimum_required(VERSION 3.20)
project(command_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

add_executable(command_parser
    src/main.cpp
    src/hash_util.cpp
    src/fs_util.cpp
    src/cli_parse.cpp
)

# Platform-specific settings
if (WIN32)
    target_compile_definitions(command_parser PRIVATE _WIN32)
    target_link_libraries(command_parser PRIVATE Advapi32)
elseif (UNIX)
    target_compile_definitions(command_parser PRIVATE __unix__)
endif()

# Always require OpenSSL and link it (for all platforms)
find_package(OpenSSL REQUIRED)
target_link_libraries(command_parser PRIVATE OpenSSL::Crypto OpenSSL::SSL)

# Warnings and secure options
if (MSVC)
    target_compile_options(command_parser PRIVATE /W4 /WX /permissive- /sdl)
    # /W4       : Enable high warning level
    # /WX       : Treat warnings as errors
    # /permissive- : Strict standard conformance
    # /sdl      : Enable additional security checks (buffer overrun, etc.)
    # /GS is enabled by default for stack protection
    target_link_options(command_parser PRIVATE /DYNAMICBASE) # Ensure ASLR
else()
    target_compile_options(command_parser PRIVATE
        -Wall
        -Wextra
        -pedantic
        -Werror                   # Treat warnings as errors
        -fstack-protector-strong  # Stack protector
        -fPIE                     # Position independent code for executables
    )
    target_link_options(command_parser PRIVATE -pie) # Enable PIE (ASLR)
endif()
