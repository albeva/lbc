//
// Created by Albert Varaksin on 01/05/2021.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
class Symbol;
struct AstStmtList;
struct AstModule;
struct AstFuncDecl;
struct AstFuncParamDecl;

namespace Sem {
    /**
     * Semantic pass that declares all the functions
     * and declarations in the ast
     */
    class FuncDeclarerPass final: public Pass {
    public:
        using Pass::Pass;
        void visit(AstModule& ast);

    private:
        void visit(lbc::AstStmtList& ast);
        void visitFuncDecl(AstFuncDecl& ast, bool external);
        void visit(AstFuncParamDecl& ast);
        [[nodiscard]] Symbol* createParamSymbol(AstFuncParamDecl& ast);
    };

} // namespace Sem
} // namespace lbc
