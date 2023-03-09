cmake_minimum_required(VERSION 3.23)
include(cmake/llvm.cmake)

# Cmake complains about policy if this is not set
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

# C++ language version to use
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 90)

# Note: by default ENABLE_DEVELOPER_MODE is True This means that all analysis (sanitizers, static analysis) is enabled
# and all warnings are treated as errors if you want to switch this behavior, change TRUE to FALSE
set(ENABLE_DEVELOPER_MODE
    TRUE
    CACHE BOOL "Enable 'developer mode'")

# strongly encouraged to enable this globally to avoid conflicts between -Wpedantic being enabled and -std=c++20 and
# -std=gnu++20 for example when compiling with PCH enabled
set(CMAKE_CXX_EXTENSIONS OFF)

# Fail on warnings
set(WARNINGS_AS_ERRORS ON)

# Add project_options v0.25.2 https://github.com/cpp-best-practices/project_options
include(FetchContent)
FetchContent_Declare(_project_options URL https://github.com/aminya/project_options/archive/refs/tags/v0.25.2.zip)
FetchContent_MakeAvailable(_project_options)
include(${_project_options_SOURCE_DIR}/Index.cmake)

# Set the project name and language
project(
    lbc
    VERSION 0.0.1
    DESCRIPTION "A toy BASIC compiler"
    HOMEPAGE_URL "https://github.com/albeva/lbc"
    LANGUAGES CXX C)

# set the startup project for the "play" button in MSVC
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT lbc)

get_property(BUILDING_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(BUILDING_MULTI_CONFIG)
    if(NOT CMAKE_BUILD_TYPE)
        # Make sure that all supported configuration types have their associated conan packages available. You can
        # reduce this list to only the configuration types you use, but only if one is not forced-set on the command
        # line for VS
        message(TRACE "Setting up multi-config build types")
        set(CMAKE_CONFIGURATION_TYPES
            Debug
            Release
            RelWithDebInfo
            MinSizeRel
            CACHE STRING "Enabled build types" FORCE)
    else()
        message(TRACE "User chose a specific build type, so we are using that")
        set(CMAKE_CONFIGURATION_TYPES
            ${CMAKE_BUILD_TYPE}
            CACHE STRING "Enabled build types" FORCE)
    endif()
endif()

# Initialize project_options variable related to this project This overwrites `project_options` and sets
# `project_warnings` uncomment to enable the options. Some of them accept one or more inputs:
project_options(
    ENABLE_CPPCHECK
    CPPCHECK_OPTIONS
    --enable=style,performance,warning,portability
    --inline-suppr
    # We cannot act on a bug/missing feature of cppcheck
    --suppress=cppcheckError
    --suppress=internalAstError
    # if a file does not have an internalAstError, we get an unmatchedSuppression error
    --suppress=unmatchedSuppression
    --suppress=passedByValue
    --suppress=syntaxError
    # these cause issues
    --suppress=constParameter
    --suppress=functionStatic
    --suppress=functionConst
    --inconclusive
    ENABLE_VS_ANALYSIS
    ENABLE_PCH
    WARNINGS_AS_ERRORS)

target_compile_features(project_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
    target_compile_options(project_options INTERFACE "/Zc:__cplusplus")
endif()

# Sources
add_subdirectory(configured_files)
add_subdirectory(src)

# If MSVC is being used, and ASAN is enabled, we need to set the debugger environment so that it behaves well with
# MSVC's debugger, and we can run the target from visual studio
if(MSVC)
    get_all_installable_targets(all_targets)
    message("all_targets=${all_targets}")
    set_target_properties(${all_targets} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=$(VC_ExecutablePath_x64);%PATH%")
endif()

if(CMAKE_SKIP_INSTALL_RULES OR NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

# Experience shows that explicit package naming can help make it easier to sort out potential ABI related issues before
# they start, while helping you track a build to a specific GIT SHA
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
