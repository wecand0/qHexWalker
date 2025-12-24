cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

set(APP_VERSION "0.0.1")

get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME} VERSION ${APP_VERSION} LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ensure that we link with the threading library
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

add_compile_options(-Wfatal-errors -Wall -Werror)
# Не больше одной ошибки за раз
if(APPLE)
    #add_compile_options(--ferror-limit=1)
elseif (UNIX)
    add_compile_options(-fmax-errors=1)
endif ()

# Print CMake configuration
message(STATUS "System           : ${CMAKE_SYSTEM}")
message(STATUS "Версия ПО        : ${APP_VERSION}")

#-DCMAKE_TOOLCHAIN_FILE=/Users/user/.vcpkg-clion/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_PREFIX_PATH="/Users/user/Qt/6.10.1/macos"  -DCMAKE_C_COMPILER_LAUNCHER="ccache" -DCMAKE_CXX_COMPILER_LAUNCHER="ccache"
option(BUILD_TESTS "ENABLE TESTING" OFF)
option(BENCHMARK_ENABLE "ENABLE BENCHMARK" OFF)
option(DEBUG "ENABLE DEBUG" OFF)
option(ENABLE_NATIVE_OPTIMIZATIONS "Enable -march=native/-mtune=native (non-portable across CPUs)" ON)

## CONFIG
set(LOGGER_NAME "core")
set(VERSION ${APP_VERSION})
set(APP_NAME ${PROJECT_NAME})
configure_file(config/config.in ${PROJECT_BINARY_DIR}/config/config.h)
configure_file(config/pch.h.in ${PROJECT_BINARY_DIR}/config/pch.h)
include_directories(${PROJECT_BINARY_DIR}/config)
####################

if (DEBUG)
    message(STATUS "Сборка           : RelWithDebInfo")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
    # Отдельные флаги для конфигурации Debug
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
else ()
    message(STATUS "Сборка           : Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT -DNDEBUG)
    set(CMAKE_BUILD_TYPE Release)

    # Включаем LTO/IPO для релиза (если поддерживается)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

    # Современные оптимизации для максимальной производительности в релизе
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fvisibility=hidden -fstack-protector-strong -Wunreachable-code -Wno-attributes -Werror=return-type -pedantic")

    # Опционально включаем -march/-mtune=native (непереносимо между разными CPU)
    if (ENABLE_NATIVE_OPTIMIZATIONS)
        message(STATUS "CPU              : сборка будет привязана к CPU сборочного хоста (-march/-mtune=native)")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mtune=native")
    else ()
        message(STATUS "CPU              : используется переносимая сборка без -march/-mtune=native")
    endif ()
endif ()

if (BUILD_TESTS)
    message(STATUS "Тесты            : вкл")
    # Включаем тестирование
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
else ()
    message(STATUS "Тесты            : выкл")
endif ()

if(BENCHMARK_ENABLE)
    message(STATUS "Benchmark        : вкл")
    find_package(benchmark CONFIG REQUIRED)
else ()
    message(STATUS "Benchmark        : вкл")
endif ()

find_package(spdlog CONFIG REQUIRED)
find_package(h3 CONFIG REQUIRED)

# Найти Qt компоненты
find_package(Qt6 REQUIRED COMPONENTS QuickControls2 Sql REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_resources(RESOURCES resources.qrc)

find_package(QMapLibre COMPONENTS Location REQUIRED)

include_directories(src/models)
include_directories(src/core)

file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/src/*.h")
list(REMOVE_ITEM SRC_FILES "${CMAKE_SOURCE_DIR}/src/core/main.cpp")

add_library(${PROJECT_NAME}_imp STATIC ${SRC_FILES} ${HEADER_FILES})
target_precompile_headers(${PROJECT_NAME}_imp PRIVATE ${PROJECT_BINARY_DIR}/config/pch.h)

target_link_libraries(${PROJECT_NAME}_imp PUBLIC
        Qt6::QuickControls2
        Qt6::Positioning
        Qt6::Sql

        spdlog::spdlog
        h3::h3
)

qt_add_executable(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/src/core/main.cpp")
target_precompile_headers(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR}/config/pch.h)

target_link_libraries(${PROJECT_NAME} PUBLIC
        ${PROJECT_NAME}_imp
)

target_compile_definitions(${PROJECT_NAME} PUBLIC QT_DEBUG_NO_OUTPUT)
qt_policy(SET QTP0004 NEW)

# Настройка QML ресурсов
qt_add_qml_module(
        ${PROJECT_NAME}
        URI QHexWalker
        VERSION 1.0
        RESOURCE_PREFIX "/"
        QML_FILES
            ui/main.qml

        RESOURCES
            data/style.json
)

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
)

set(QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/H3VIEWER" ${QML_IMPORT_PATH})

qmaplibre_location_setup_plugins(${PROJECT_NAME})

# Функция копирования ресурсов рекурсивно
function(copy_recursive SOURCE_PATH DESTINATION_PATH REGEX)
    file(GLOB_RECURSE
            FILES
            ${SOURCE_PATH}
            "${SOURCE_PATH}/${REGEX}")
    foreach (file ${FILES})
        # To recreate the same folder structure we first need to read the base folder
        file(RELATIVE_PATH RELATIVE_FILE_PATH ${SOURCE_PATH} ${file})
        get_filename_component(FOLDER ${RELATIVE_FILE_PATH} DIRECTORY ${SOURCE_PATH})
        file(MAKE_DIRECTORY ${DESTINATION_PATH}/${FOLDER})
        configure_file(${file} "${DESTINATION_PATH}/${RELATIVE_FILE_PATH}" COPYONLY)
    endforeach ()
endfunction()
#
copy_recursive(${CMAKE_SOURCE_DIR}/data/ ${CMAKE_CURRENT_BINARY_DIR} *)

########################################
#
# Тесты
#
if (BUILD_TESTS)
    # Рекурсивно ищем все используемые в тестах .cpp файлы
    file(GLOB TEST_SRC_FILES "${CMAKE_SOURCE_DIR}/tests/*.cpp")

    add_executable(${PROJECT_NAME}_tests "${TEST_SRC_FILES}")
    target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_imp GTest::gtest GTest::gtest_main)
    target_include_directories(${PROJECT_NAME}_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME ${PROJECT_NAME}_Tests COMMAND ${PROJECT_NAME}_tests)

    copy_recursive(${CMAKE_SOURCE_DIR}/tests/files ${CMAKE_CURRENT_BINARY_DIR}/tests/files *)
endif ()

########################################
#
# Benchmark
#
if (BENCHMARK_ENABLE)
    #    # Рекурсивно ищем все используемые в benchmark .cpp файлы
    file(GLOB BENCHMARK_SRC_FILES "${CMAKE_SOURCE_DIR}/benchmark/*.cpp")

    add_executable(${PROJECT_NAME}_benchmark "${BENCHMARK_SRC_FILES}")
    target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE ${PROJECT_NAME}_imp benchmark::benchmark benchmark::benchmark_main)
    target_include_directories(${PROJECT_NAME}_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endif ()

########################################

install(
        TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
        TARGET  ${PROJECT_NAME}
        OUTPUT_SCRIPT deploy_script
        MACOS_BUNDLE_POST_BUILD
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    install(
            IMPORTED_RUNTIME_ARTIFACTS QMapLibre::Core QMapLibre::Location
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
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

# Packaging
set(CPACK_GENERATOR TBZ2)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPACK_SYSTEM_NAME "macOS")
else()
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}")
endif()

include(CPack)