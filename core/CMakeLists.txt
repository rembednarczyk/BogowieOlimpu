# Core simulation library: pure C++17, no Godot headers (checked by tools/checks/check_core.py).
add_library(bo_core STATIC
    src/agora.cpp
    src/appeal.cpp
    src/city.cpp
    src/data.cpp
    src/data_economy.cpp
    src/extraction.cpp
    src/finance.cpp
    src/housing.cpp
    src/horses.cpp
    src/husbandry.cpp
    src/labour.cpp
    src/logistics.cpp
    src/map.cpp
    src/migration.cpp
    src/path.cpp
    src/placement.cpp
    src/walker.cpp
    src/wildlife.cpp
    src/risks.cpp
    src/serialize.cpp
    src/serialize_city.cpp
    src/serialize_economy.cpp
    src/serialize_goods.cpp
    src/simulation.cpp
    src/storage.cpp
    src/systems.cpp
    src/trade.cpp
    src/undo.cpp
)
target_include_directories(bo_core PUBLIC include)
target_include_directories(bo_core PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
target_compile_features(bo_core PUBLIC cxx_std_17)
set_target_properties(bo_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bo_core PRIVATE -Wall -Wextra -Wpedantic -Werror -Wconversion -Wno-sign-conversion)
endif()

if(BO_BUILD_TESTS)
    add_executable(bo_core_tests
        tests/main.cpp
        tests/test_time.cpp
        tests/test_rng.cpp
        tests/test_fixed.cpp
        tests/test_commands.cpp
        tests/test_data.cpp
        tests/test_save.cpp
        tests/test_golden.cpp
        tests/test_map.cpp
        tests/test_city.cpp
        tests/test_path.cpp
        tests/test_walker.cpp
        tests/test_placement.cpp
        tests/test_undo.cpp
        tests/test_appeal.cpp
        tests/test_housing.cpp
        tests/test_labour.cpp
        tests/test_migration.cpp
        tests/test_goods.cpp
        tests/test_terrain.cpp
        tests/test_horses.cpp
        tests/test_husbandry.cpp
        tests/test_extraction.cpp
        tests/test_wildlife.cpp
        tests/test_risks.cpp
        tests/test_trade.cpp
    )
    target_link_libraries(bo_core_tests PRIVATE bo_core)
    target_include_directories(bo_core_tests PRIVATE ${PROJECT_SOURCE_DIR}/third_party)
    target_compile_definitions(bo_core_tests PRIVATE BO_DATA_DIR="${PROJECT_SOURCE_DIR}/data")
    add_test(NAME bo_core_tests COMMAND bo_core_tests)
endif()
