# cmake/Packaging.cmake
# Packaging configuration for various platforms

# =============================================================================
# CPack General Settings
# =============================================================================
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${APP_VERSION}")
set(CPACK_PACKAGE_VENDOR "qHexWalker Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hexagonal Grid Pathfinding & Maze Visualization")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CONTACT "https://github.com/wecand0/qHexWalker")

# =============================================================================
# Platform-specific packaging
# =============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # AppImage target
    add_custom_target(appimage
        COMMAND ${CMAKE_SOURCE_DIR}/packaging/appimage/build-appimage.sh ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS ${PROJECT_NAME}
        COMMENT "Building AppImage..."
        VERBATIM
    )

    # DEB package settings
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "qHexWalker Team")
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6-core6, libqt6-gui6, libqt6-qml6")

    # RPM package settings
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Science")

    set(CPACK_GENERATOR "TBZ2;DEB")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME}")
    set(CPACK_SYSTEM_NAME "macOS")

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
endif()

include(CPack)