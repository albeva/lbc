add_executable(lbc-tblgen
    Builder.hpp
    GeneratorBase.hpp
    gens/AstGen.cpp
    gens/AstGen.hpp
    gens/TokensGen.cpp
    gens/TokensGen.hpp
    main.cpp
)
configure_tblgen(lbc-tblgen)
target_link_libraries(lbc-tblgen PRIVATE compiler_options compiler_warnings)

