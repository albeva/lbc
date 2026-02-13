cmake_minimum_required(VERSION 4.2.2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL CACHE STRING "" FORCE)

# The Project
project(
    lbc
    VERSION 0.0.1
    DESCRIPTION "A toy BASIC compiler"
    HOMEPAGE_URL "https://github.com/albeva/lbc"
    LANGUAGES CXX C
)

# Misc
include(FetchContent)
include(cmake/warnings.cmake)
include(cmake/options.cmake)
include(cmake/tblgen.cmake)
include(cmake/llvm.cmake)

# Add sources
add_subdirectory(configured_files)
add_subdirectory(tools)
add_subdirectory(src)
add_subdirectory(tests)
