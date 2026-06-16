cmake_minimum_required(VERSION 3.28)
project(3DEngine)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include(FetchContent)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# SDL3
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.0
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(SDL3)

# render3d — the software rendering library (extracted from this project).
# For local development the sibling checkout is used directly. To consume from a
# remote instead, replace SOURCE_DIR with GIT_REPOSITORY/GIT_TAG, e.g.:
#   FetchContent_Declare(render3d GIT_REPOSITORY <url> GIT_TAG main)
# and optionally override locally with -DFETCHCONTENT_SOURCE_DIR_RENDER3D=<path>.
FetchContent_Declare(
    render3d
    GIT_REPOSITORY https://github.com/m-alcu/render3d.git
    GIT_TAG master
)
FetchContent_MakeAvailable(render3d)

# Application-side sources only — the renderer now lives in the render3d library.
set(SRC_DIRS src src/vendor/imgui src/vendor/nothings src/vendor/tinyobjloader src/scenes src/assets)

set(SRC_FILES "")
foreach(dir ${SRC_DIRS})
    file(GLOB_RECURSE DIR_SRC CONFIGURE_DEPENDS ${dir}/*.cpp)
    list(APPEND SRC_FILES ${DIR_SRC})
endforeach()


message(STATUS "SRC_FILES: ${SRC_FILES}")

add_executable(3DEngine ${SRC_FILES})

# Properly handle multi-config (Visual Studio style) output folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Ensure each configuration type has its own folder under bin/
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUTCONFIG})
endforeach()


# Copy the resources folder to the binary directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 render3d::render3d)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED on)

# ============================================================================
# Testing with Google Test
# ============================================================================
# Core math/ECS/transform unit tests live in the render3d library. Here we test
# the application-side asset code (PrefabFactory) against the render3d library.
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # Prevent gtest from overriding compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(test_ecs
        tests/test_ecs.cpp
        src/assets/prefab_factory.cpp
        src/assets/texture_loader.cpp
        src/vendor/nothings/stb_image.cpp
        src/vendor/tinyobjloader/tiny_obj_loader.cpp
    )
    target_include_directories(test_ecs PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_ecs PRIVATE render3d::render3d GTest::gtest_main)
    set_target_properties(test_ecs PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED on)

    include(GoogleTest)
    gtest_discover_tests(test_ecs)
endif()
