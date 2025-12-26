# cmake/Dependencies.cmake
# External dependencies configuration

# Logging
find_package(spdlog CONFIG REQUIRED)

# H3 hexagonal indexing
find_package(h3 CONFIG REQUIRED)

# Qt 6 components
find_package(Qt6 REQUIRED COMPONENTS QuickControls2 Sql)
qt_standard_project_setup(REQUIRES 6.5)

# MapLibre for map rendering
find_package(QMapLibre COMPONENTS Location REQUIRED)

# Optional: Testing
if(BUILD_TESTS)
    message(STATUS "Tests            : ON")
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
else()
    message(STATUS "Tests            : OFF")
endif()

# Optional: Benchmarking
if(BENCHMARK_ENABLE)
    message(STATUS "Benchmark        : ON")
    find_package(benchmark CONFIG REQUIRED)
else()
    message(STATUS "Benchmark        : OFF")
endif()

# Optional: Documentation
if(BUILD_DOCS)
    find_package(Doxygen COMPONENTS dot)
    if(DOXYGEN_FOUND)
        message(STATUS "Doxygen          : ON (${DOXYGEN_VERSION})")
    else()
        message(STATUS "Doxygen          : not found")
        set(BUILD_DOCS OFF)
    endif()
else()
    message(STATUS "Doxygen          : OFF")
endif()