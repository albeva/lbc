cmake_minimum_required(VERSION 4.2.2)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
include(FetchContent)

# Add project_options from https://github.com/aminya/project_options
FetchContent_Declare(_project_options URL "https://github.com/aminya/project_options/archive/refs/tags/v0.41.0.zip")
FetchContent_MakeAvailable(_project_options)
include(${_project_options_SOURCE_DIR}/Index.cmake)

# Configure
project(
    lbc
    VERSION 0.0.1
    DESCRIPTION "A toy BASIC compiler"
    HOMEPAGE_URL "https://github.com/albeva/lbc"
    LANGUAGES CXX C
)
project_options(
    WARNINGS_AS_ERRORS
)

# Disable rtti and exceptions
target_compile_options(project_options INTERFACE -fno-exceptions -fno-rtti)

# Add subfolders
add_subdirectory(configured_files)
add_subdirectory(src)
add_subdirectory(tests)

# Package the project
package_project(TARGETS lbc)
