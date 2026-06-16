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
    Utils/CompilerBase.cpp
    Utils/CompilerBase.hpp
    fixtures/CompileFailureTests.cpp
    fixtures/CompileSuccessTests.cpp
    unittests/backend/GenTests.cpp
    unittests/backend/IrGenTests.cpp
    unittests/frontend/AstVisitorTests.cpp
    unittests/frontend/LexerTests.cpp
    unittests/frontend/ParserTests.cpp
    unittests/frontend/SemaExprTests.cpp
    unittests/frontend/TypeComparisonTests.cpp
    unittests/frontend/TypeFactoryTests.cpp
    unittests/frontend/ValueCategoryTests.cpp
)

# Link (the JIT used by the success-test harness lives only in the test target)
llvm_map_components_to_libnames(test_jit_libs orcjit native)
target_link_libraries(tests PRIVATE lbc_lib compiler_options gtest_main ${test_jit_libs})
target_include_directories(tests SYSTEM PRIVATE ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(tests REUSE_FROM lbc_lib)

# Discover tests
include(GoogleTest)
gtest_discover_tests(tests
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DISCOVERY_MODE PRE_TEST
)
