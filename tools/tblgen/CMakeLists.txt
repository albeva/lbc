add_executable(lbc-tblgen
    TokensGen.cpp
)
configure_tblgen(lbc-tblgen)
target_link_libraries(lbc-tblgen PRIVATE project_options project_warnings)
