# cmake/CompilerFlags.cmake
# Compiler flags and build type configuration

if(NOT ANDROID)
    add_compile_options(-Wfatal-errors -Wall -Werror)
endif()

# Limit errors to one at a time
if(APPLE)
    # add_compile_options(--ferror-limit=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
    add_compile_options(-fmax-errors=1)
endif()

# Build type configuration
if(DEBUG)
    message(STATUS "Build Type       : RelWithDebInfo")
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
else()
    message(STATUS "Build Type       : Release")
    add_definitions(-DQT_NO_DEBUG_OUTPUT -DNDEBUG)
    set(CMAKE_BUILD_TYPE Release)

    if(ANDROID)
        # Android-specific release flags
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -fvisibility=hidden")
        message(STATUS "CPU Optimizations: Android (portable)")
    else()
        # Enable LTO/IPO for release builds (not on Android)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

        # Modern optimizations for maximum performance
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fvisibility=hidden -fstack-protector-strong -Wunreachable-code -Wno-attributes -Werror=return-type -pedantic")

        # Optional CPU-specific optimizations
        if(ENABLE_NATIVE_OPTIMIZATIONS)
            message(STATUS "CPU Optimizations: native (-march/-mtune=native)")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mtune=native")
        else()
            message(STATUS "CPU Optimizations: portable (no -march/-mtune)")
        endif()
    endif()
endif()