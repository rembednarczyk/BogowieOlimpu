# GDExtension: the only module that knows both bo_core and Godot (docs/SILNIK.md).
set(GODOTCPP_BUILD_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/build_profile.json" CACHE PATH "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/godot-cpp ${CMAKE_BINARY_DIR}/godot-cpp)

add_library(bo_gdext SHARED
    src/register_types.cpp
    src/bo_simulation.cpp
)
target_link_libraries(bo_gdext PRIVATE bo_core godot::cpp)
set_target_properties(bo_gdext PROPERTIES
    OUTPUT_NAME "bo_gdext"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/game/bin"
    PREFIX "lib")
