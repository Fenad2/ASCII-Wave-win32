cmake_minimum_required(VERSION 3.28)

project(ascii_wave LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ascii_wave main.cpp)
target_compile_definitions(ascii_wave PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

if (MSVC)
    target_compile_options(ascii_wave PRIVATE /W4 /permissive-)
else()
    target_compile_options(ascii_wave PRIVATE -Wall -Wextra -Wpedantic)
endif()
