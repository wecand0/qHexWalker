# tests/CMakeLists.txt
# Unit tests configuration

if(NOT BUILD_TESTS)
    return()
endif()

# Collect test source files
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Test executable
add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME}_tests
    PRIVATE
        ${PROJECT_NAME}_lib
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(${PROJECT_NAME}_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/src
)

# Register tests with CTest
add_test(
    NAME ${PROJECT_NAME}_Tests
    COMMAND ${PROJECT_NAME}_tests
)

# Copy test data files
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/files")
    file(GLOB_RECURSE TEST_DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/files/*")
    foreach(file ${TEST_DATA_FILES})
        file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/files" ${file})
        configure_file(${file} "${CMAKE_CURRENT_BINARY_DIR}/tests/files/${rel_path}" COPYONLY)
    endforeach()
endif()