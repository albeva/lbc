//
// Created by Albert on 26/02/2022.
//
#pragma once
#include "pch.hpp"
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
struct AstVarDecl;
class TypeRoot;

namespace Sem {
    class DeclPass final : public Pass {
    public:
        using Pass::Pass;
        void declare(AstStmtList& ast) noexcept;
        void declare(AstDecl& ast) noexcept;
        void declareAndDefine(const std::vector<AstVarDecl*>& vars) noexcept;
        void declareAndDefine(AstVarDecl& var) noexcept;
        void define(Symbol* symbol) noexcept;

    private:
        void defineFunc(AstFuncDecl& ast) noexcept;
        void defineFuncParam(AstFuncParamDecl& ast) noexcept;
        void defineAlias(AstTypeAlias& ast) noexcept;
        void defineUdt(AstUdtDecl& ast) noexcept;
        void defineVar(AstVarDecl& ast) noexcept;
        Symbol* createNewSymbol(AstDecl& ast, const TypeRoot* type) noexcept;
    };
} // namespace Sem
} // namespace lbc
