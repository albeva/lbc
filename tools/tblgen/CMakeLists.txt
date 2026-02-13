add_executable(lbc-tblgen
    main.cpp
    TokensGen.cpp
    Builder.hpp
    Generators.hpp
)
configure_tblgen(lbc-tblgen)
target_link_libraries(lbc-tblgen PRIVATE compiler_options compiler_warnings)
