# doc/CMakeLists.txt
# Doxygen documentation configuration

if(NOT BUILD_DOCS OR NOT DOXYGEN_FOUND)
    return()
endif()

set(DOXYGEN_CONFIG ${CMAKE_SOURCE_DIR}/Doxyfile)

add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)

# Optional: Add docs to default build
# add_dependencies(${PROJECT_NAME} docs)