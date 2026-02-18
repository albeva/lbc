# Get GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
if (MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif ()
FetchContent_MakeAvailable(googletest)

# Tests
add_executable(tests
    AstVisitorTests.cpp
    BasicTests.cpp
    LexerTests.cpp
    ParserTests.cpp
)

# Link
target_link_libraries(tests PRIVATE lbc_lib compiler_options gtest_main)
target_include_directories(tests SYSTEM PRIVATE ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
target_precompile_headers(tests REUSE_FROM lbc_lib)

# Discover tests
include(GoogleTest)
gtest_discover_tests(tests
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DISCOVERY_MODE PRE_TEST
)
