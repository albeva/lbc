# Tests
add_executable(tests
    CompileTests.cpp
    LexerTests.cpp
    CmdLineParserTests.cpp)

# Get GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Link
target_link_libraries(tests PRIVATE lbc_lib project_options gtest_main)
target_precompile_headers(tests REUSE_FROM lbc_lib)

include(GoogleTest)
gtest_discover_tests(tests WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
