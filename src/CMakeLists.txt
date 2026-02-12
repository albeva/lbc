# lbc library
add_library(lbc_lib)

target_sources(lbc_lib
    PRIVATE
    Lexer/Lexer.cpp
    Parser/Parser.cpp
    Utilities/Sample.cpp

    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
    Lexer/Lexer.hpp
    Parser/Parser.hpp
    Utilities/Sample.hpp
)
target_link_libraries(lbc_lib PRIVATE project_options project_warnings)
target_include_directories(lbc_lib PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")

# precompile header
target_precompile_headers(lbc_lib PUBLIC pch.hpp)

# link llvm
configure_llvm(lbc_lib)

# lbc executable
add_executable(lbc main.cpp)
target_link_libraries(lbc PRIVATE lbc_lib project_options project_warnings)
target_precompile_headers(lbc REUSE_FROM lbc_lib)
set_target_properties(lbc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
