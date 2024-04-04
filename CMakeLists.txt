cmake_minimum_required(VERSION 3.23)

# Cmake complains about policy if this is not set
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0135 NEW)
endif()

# git commit FIXME get the sha automatically
set(GIT_SHA
    "Unknown"
    CACHE STRING "SHA this build was generated from")

# C++ language version to use
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

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

# Add project_options v0.35.1 https://github.com/cpp-best-practices/project_options
include(FetchContent)
FetchContent_Declare(_project_options URL https://github.com/aminya/project_options/archive/refs/tags/v0.35.1.zip)
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

# Initialize project_options variable related to this project This overwrites `project_options` and sets
# `project_warnings` uncomment to enable the options. Some of them accept one or more inputs:
project_options(
    # enabling cppcheck has issues with gtest
    # ENABLE_CPPCHECK
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

# MSVC specific
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
    target_compile_options(project_options INTERFACE "/Zc:__cplusplus")
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    target_compile_options(project_options INTERFACE -fno-exceptions -fno-rtti)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(project_options INTERFACE -fno-exceptions -fno-rtti)
endif()

# Sources
add_subdirectory(configured_files)
add_subdirectory(src)

# Tests
if (ENABLE_DEVELOPER_MODE)
    enable_testing()
    include(CTest)
    add_subdirectory(tests)
endif()

# Experience shows that explicit package naming can help make it easier to sort out potential ABI related issues before
# they start, while helping you track a build to a specific GIT SHA
set(CPACK_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}-${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)

include(CPack)
