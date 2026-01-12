# =============================================================================
# qHexWalker - Hexagonal Grid Pathfinding & Maze Visualization
# =============================================================================
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

# Project configuration
set(APP_VERSION "0.0.2")
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME} VERSION ${APP_VERSION} LANGUAGES CXX C)

# =============================================================================
# C++ Standard
# =============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

# =============================================================================
# Build Options
# =============================================================================
option(BUILD_TESTS    "Build unit tests"                    OFF)
option(BUILD_DOCS     "Build Doxygen documentation"         OFF)
option(BENCHMARK_ENABLE "Build performance benchmarks"      OFF)
option(DEBUG          "Enable debug build"                  OFF)
option(ENABLE_NATIVE_OPTIMIZATIONS "Enable -march=native"   OFF)

# =============================================================================
# Configuration Files
# =============================================================================
set(LOGGER_NAME "core")
set(VERSION ${APP_VERSION})
set(APP_NAME ${PROJECT_NAME})

configure_file(config/config.in ${PROJECT_BINARY_DIR}/config/config.h)
configure_file(config/pch.h.in ${PROJECT_BINARY_DIR}/config/pch.h)
include_directories(${PROJECT_BINARY_DIR}/config)

# =============================================================================
# Print Configuration
# =============================================================================
message(STATUS "=== ${PROJECT_NAME} v${APP_VERSION} ===")
message(STATUS "System           : ${CMAKE_SYSTEM}")

# =============================================================================
# CMake Modules
# =============================================================================
include(cmake/CompilerFlags.cmake)
include(cmake/Dependencies.cmake)

# =============================================================================
# Copy Data Files
# =============================================================================
function(copy_recursive SOURCE_PATH DESTINATION_PATH REGEX)
    file(GLOB_RECURSE FILES ${SOURCE_PATH} "${SOURCE_PATH}/${REGEX}")
    foreach(file ${FILES})
        file(RELATIVE_PATH rel_path ${SOURCE_PATH} ${file})
        get_filename_component(dir ${rel_path} DIRECTORY ${SOURCE_PATH})
        file(MAKE_DIRECTORY ${DESTINATION_PATH}/${dir})
        configure_file(${file} "${DESTINATION_PATH}/${rel_path}" COPYONLY)
    endforeach()
endfunction()

copy_recursive(${CMAKE_SOURCE_DIR}/data/ ${CMAKE_CURRENT_BINARY_DIR}/src *)

# =============================================================================
# Subdirectories
# =============================================================================
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmark)
add_subdirectory(doc)

# =============================================================================
# Packaging (CPack, AppImage, etc.)
# =============================================================================
include(cmake/Packaging.cmake)