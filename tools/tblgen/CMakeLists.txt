add_executable(lbc-tblgen
    Builder.hpp
    GeneratorBase.hpp
    gens/ast/AstClass.cpp
    gens/ast/AstClass.hpp
    gens/ast/AstArg.cpp
    gens/ast/AstArg.hpp
    gens/ast/AstFwdDeclGen.cpp
    gens/ast/AstFwdDeclGen.hpp
    gens/ast/AstGen.cpp
    gens/ast/AstGen.hpp
    gens/ast/AstVisitorGen.cpp
    gens/ast/AstVisitorGen.hpp
    gens/diag/DiagGen.cpp
    gens/diag/DiagGen.hpp
    gens/diag/DiagImplGen.cpp
    gens/diag/DiagImplGen.hpp
    gens/TokensGen.cpp
    gens/TokensGen.hpp
    main.cpp
)
configure_tblgen(lbc-tblgen)
target_link_libraries(lbc-tblgen PRIVATE compiler_options compiler_warnings)

