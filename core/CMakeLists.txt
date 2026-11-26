# Core simulation library: pure C++17, no Godot headers (checked by tools/checks/check_core.py).
add_library(bo_core STATIC
    src/data.cpp
    src/serialize.cpp
    src/simulation.cpp
)
target_include_directories(bo_core PUBLIC include)
target_include_directories(bo_core PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
target_compile_features(bo_core PUBLIC cxx_std_17)
set_target_properties(bo_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bo_core PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion -Wno-sign-conversion)
endif()

if(BO_BUILD_TESTS)
    enable_testing()
    add_executable(bo_core_tests
        tests/main.cpp
        tests/test_time.cpp
        tests/test_rng.cpp
        tests/test_fixed.cpp
        tests/test_commands.cpp
        tests/test_data.cpp
        tests/test_save.cpp
        tests/test_golden.cpp
    )
    target_link_libraries(bo_core_tests PRIVATE bo_core)
    target_include_directories(bo_core_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
    target_compile_definitions(bo_core_tests PRIVATE BO_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
    add_test(NAME bo_core_tests COMMAND bo_core_tests)
endif()
