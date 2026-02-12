# Generate TokenKinds.inc from Tokens.td
set(TOKEN_KINDS_INC "${CMAKE_CURRENT_BINARY_DIR}/generated/Lexer/TokenKinds.inc")
add_custom_command(
    OUTPUT "${TOKEN_KINDS_INC}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/generated/Lexer"
    COMMAND lbc-tblgen "${CMAKE_CURRENT_SOURCE_DIR}/Lexer/Tokens.td" -o "${TOKEN_KINDS_INC}"
    DEPENDS lbc-tblgen "${CMAKE_CURRENT_SOURCE_DIR}/Lexer/Tokens.td"
    COMMENT "Generating TokenKinds.inc from Tokens.td"
)
add_custom_target(generate_token_kinds DEPENDS "${TOKEN_KINDS_INC}")

# lbc library
add_library(lbc_lib)

target_sources(lbc_lib
    PRIVATE
    Lexer/Lexer.cpp
    Lexer/Token.cpp
    Parser/Parser.cpp
    Utilities/Sample.cpp

    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES
    Lexer/Lexer.hpp
    Lexer/Token.hpp
    Parser/Parser.hpp
    Utilities/Diagnostics.hpp
    Utilities/Sample.hpp
    Utilities/Try.hpp
)
target_link_libraries(lbc_lib PRIVATE project_options project_warnings)
target_include_directories(lbc_lib PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
target_include_directories(lbc_lib PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
add_dependencies(lbc_lib generate_token_kinds)

# precompile header
target_precompile_headers(lbc_lib PUBLIC pch.hpp)

# link llvm
configure_llvm(lbc_lib)

# lbc executable
add_executable(lbc main.cpp)
target_link_libraries(lbc PRIVATE lbc_lib project_options project_warnings)
target_precompile_headers(lbc REUSE_FROM lbc_lib)
set_target_properties(lbc PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
