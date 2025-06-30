cmake_minimum_required(VERSION 3.28)
project(3DEngine)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_subdirectory(submodules/SDL)

set(SRC_DIRS src src/vendor/imgui src/vendor/lodepng src/objects src/effects src/backgrounds)

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

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED on)