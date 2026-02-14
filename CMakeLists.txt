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

# rapidobj
FetchContent_Declare(
    rapidobj
    GIT_REPOSITORY https://github.com/guybrush77/rapidobj.git
    GIT_TAG v1.1
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(rapidobj)

# YAML parsing library
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.9.0
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

set(SRC_DIRS src src/vendor/imgui src/vendor/stb src/objects src/effects src/backgrounds src/scenes src/ecs)

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

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 rapidobj::rapidobj yaml-cpp::yaml-cpp)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED on)

# ============================================================================
# Testing with Google Test
# ============================================================================
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

    # Math library test
    add_executable(test_math
        tests/test_math.cpp
        src/slib.cpp
        src/smath.cpp
    )
    target_include_directories(test_math PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_math PRIVATE GTest::gtest_main)
    set_target_properties(test_math PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED on)

    # Transform component test
    add_executable(test_transform
        tests/test_transform.cpp
        src/slib.cpp
        src/smath.cpp
    )
    target_include_directories(test_transform PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_transform PRIVATE GTest::gtest_main)
    set_target_properties(test_transform PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED on)

    # ECS (Entity, ComponentStore, Registry) test
    add_executable(test_ecs
        tests/test_ecs.cpp
        src/slib.cpp
        src/smath.cpp
        src/ecs/prefab_factory.cpp
        src/vendor/nothings/stb_image.cpp
    )
    target_include_directories(test_ecs PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_ecs PRIVATE GTest::gtest_main rapidobj::rapidobj)
    set_target_properties(test_ecs PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED on)

    include(GoogleTest)
    gtest_discover_tests(test_math)
    gtest_discover_tests(test_transform)
    gtest_discover_tests(test_ecs)
endif()
