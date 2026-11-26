cmake_minimum_required(VERSION 3.22)
project(BogowieOlimpu LANGUAGES CXX)

option(BO_BUILD_TESTS "Build core tests" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(core)
