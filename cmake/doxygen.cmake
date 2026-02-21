# Doxygen documentation generation
find_package(Doxygen REQUIRED)

configure_file(
    "${CMAKE_SOURCE_DIR}/Doxyfile.in"
    "${CMAKE_BINARY_DIR}/Doxyfile"
    @ONLY
)

add_custom_target(docs
    COMMAND Doxygen::doxygen "${CMAKE_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating documentation with Doxygen"
    VERBATIM
)
