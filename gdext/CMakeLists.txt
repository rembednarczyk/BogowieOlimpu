# GDExtension: the only module that knows both bo_core and Godot (docs/SILNIK.md).
set(GODOTCPP_BUILD_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/build_profile.json" CACHE PATH "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party/godot-cpp ${CMAKE_BINARY_DIR}/godot-cpp)

# File names must match game/bo.gdextension.
if(ANDROID)
    set(BO_GDEXT_NAME "bo_gdext.android")
else()
    set(BO_GDEXT_NAME "bo_gdext")
endif()

add_library(bo_gdext SHARED
    src/register_types.cpp
    src/bo_simulation.cpp
)
target_link_libraries(bo_gdext PRIVATE bo_core godot::cpp)
if(ANDROID)
    target_link_options(bo_gdext PRIVATE "-Wl,-z,max-page-size=16384")  # 16 KB pages (Android 15+)
endif()
set_target_properties(bo_gdext PROPERTIES
    OUTPUT_NAME "${BO_GDEXT_NAME}"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/game/bin"
    PREFIX "lib")
