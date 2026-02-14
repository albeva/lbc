add_executable(lbc-tblgen
    Builder.hpp
    GeneratorBase.hpp
    Generators.hpp
    gens/AstGen.cpp
    gens/TokensGen.cpp
    main.cpp
)
configure_tblgen(lbc-tblgen)
target_link_libraries(lbc-tblgen PRIVATE compiler_options compiler_warnings)

