# Doxygen documentation generation
find_package(Doxygen)

if(DOXYGEN_FOUND)
    configure_file(
        "${CMAKE_SOURCE_DIR}/Doxyfile.in"
        "${CMAKE_BINARY_DIR}/Doxyfile"
        @ONLY
    )

    add_custom_target(docs
        COMMAND Doxygen::doxygen "${CMAKE_BINARY_DIR}/Doxyfile"
        COMMAND open "${CMAKE_SOURCE_DIR}/docs/html/index.html"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating documentation with Doxygen"
        VERBATIM
    )
else()
    message(STATUS "Doxygen not found, 'docs' target will not be available")
endif()
