//
// Created by Albert on 26/02/2022.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
class Symbol;
struct AstModule;
struct AstStmtList;
struct AstDecl;
struct AstFuncDecl;
struct AstUdtDecl;
struct AstTypeAlias;
struct AstFuncParamDecl;

namespace Sem {
    class DeclPass final : public Pass {
    public:
        using Pass::Pass;
        void visit(AstModule&) noexcept;
        void define(Symbol* symbol) noexcept;

    private:
        void declare(AstStmtList& ast) noexcept;
        void declare(AstDecl& ast) noexcept;

        void defineFunc(AstFuncDecl& ast) noexcept;
        void defineFuncParam(AstFuncParamDecl& ast) noexcept;
        void defineAlias(AstTypeAlias& ast) noexcept;
        void defineUdt(AstUdtDecl& ast) noexcept;

        Symbol* createParamSymbol(AstFuncParamDecl& ast) noexcept;
    };
} // namespace Sem
} // namespace lbc
