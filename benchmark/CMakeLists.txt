# benchmark/CMakeLists.txt
# Performance benchmarks configuration

if(NOT BENCHMARK_ENABLE)
    return()
endif()

# Collect benchmark source files
file(GLOB BENCHMARK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Benchmark executable
add_executable(${PROJECT_NAME}_benchmark ${BENCHMARK_SOURCES})

target_link_libraries(${PROJECT_NAME}_benchmark
    PRIVATE
        ${PROJECT_NAME}_lib
        benchmark::benchmark
        benchmark::benchmark_main
)

target_include_directories(${PROJECT_NAME}_benchmark
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)