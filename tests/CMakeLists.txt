# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)
project(relay_protocol_host_test VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(relay_protocol_test relay_protocol_test.c ../src/relay_protocol.c)
target_include_directories(relay_protocol_test PRIVATE ../src)
target_compile_options(relay_protocol_test PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME relay_protocol COMMAND relay_protocol_test)
