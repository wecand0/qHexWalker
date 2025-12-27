# src/CMakeLists.txt
# Main application library and executable

# Collect source files
file(GLOB_RECURSE LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE LIB_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Remove main.cpp from library sources
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/core/main.cpp")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/models)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/core)

# ============================================================================
# Static library (shared between app, tests, benchmarks)
# ============================================================================
add_library(${PROJECT_NAME}_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_precompile_headers(${PROJECT_NAME}_lib
    PRIVATE ${PROJECT_BINARY_DIR}/config/pch.h
)

target_include_directories(${PROJECT_NAME}_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_link_libraries(${PROJECT_NAME}_lib
    PUBLIC
        Qt6::QuickControls2
        Qt6::Positioning
        Qt6::Sql
        spdlog::spdlog
        h3::h3
)

# ============================================================================
# Main executable
# ============================================================================
qt_add_executable(${PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/core/main.cpp"
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE ${PROJECT_BINARY_DIR}/config/pch.h
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC ${PROJECT_NAME}_lib
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC QT_DEBUG_NO_OUTPUT
)

qt_policy(SET QTP0004 NEW)

# ============================================================================
# QML Module
# ============================================================================
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/main.qml
    PROPERTIES QT_RESOURCE_ALIAS "ui/main.qml"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/components/TargetButton.qml
    PROPERTIES QT_RESOURCE_ALIAS "ui/components/TargetButton.qml"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/components/ShortcutHint.qml
    PROPERTIES QT_RESOURCE_ALIAS "ui/components/ShortcutHint.qml"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/components/TargetListItem.qml
    PROPERTIES QT_RESOURCE_ALIAS "ui/components/TargetListItem.qml"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/ui/components/TargetsList.qml
    PROPERTIES QT_RESOURCE_ALIAS "ui/components/TargetsList.qml"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/data/style.json
    PROPERTIES QT_RESOURCE_ALIAS "data/style.json"
)

qt_add_qml_module(${PROJECT_NAME}
    URI QHexWalker
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
        ${CMAKE_SOURCE_DIR}/ui/main.qml
        ${CMAKE_SOURCE_DIR}/ui/components/TargetButton.qml
        ${CMAKE_SOURCE_DIR}/ui/components/ShortcutHint.qml
        ${CMAKE_SOURCE_DIR}/ui/components/TargetListItem.qml
        ${CMAKE_SOURCE_DIR}/ui/components/TargetsList.qml
    RESOURCES
        ${CMAKE_SOURCE_DIR}/data/style.json
)

# ============================================================================
# Target properties
# ============================================================================
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
)

set(QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/H3VIEWER" ${QML_IMPORT_PATH})

# MapLibre plugins
qmaplibre_location_setup_plugins(${PROJECT_NAME})

# ============================================================================
# Installation
# ============================================================================
install(
    TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

# Platform-specific installation
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    install(
        IMPORTED_RUNTIME_ARTIFACTS QMapLibre::Core QMapLibre::Location
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(${PROJECT_NAME})
endif()