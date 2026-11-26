cmake_minimum_required(VERSION 3.22)
project(BogowieOlimpu LANGUAGES CXX)

option(BO_BUILD_TESTS "Build core tests" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(BO_BUILD_TESTS)
    enable_testing()
endif()
add_subdirectory(core)

option(BO_BUILD_GDEXT "Build the Godot extension (needs third_party/godot-cpp submodule)" OFF)
if(BO_BUILD_GDEXT)
    add_subdirectory(gdext)
endif()
