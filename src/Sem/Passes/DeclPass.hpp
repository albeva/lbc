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
        void declare(AstStmtList& ast);
        void declare(AstDecl& ast);
        void declareAndDefine(const std::vector<AstVarDecl*>& vars);
        void declareAndDefine(AstVarDecl& var);
        void define(Symbol* symbol);

    private:
        void defineFunc(AstFuncDecl& ast);
        void defineFuncParam(AstFuncParamDecl& ast);
        void defineAlias(AstTypeAlias& ast);
        void defineUdt(AstUdtDecl& ast);
        void defineVar(AstVarDecl& ast);
        Symbol* createNewSymbol(AstDecl& ast, const TypeRoot* type);
    };
} // namespace Sem
} // namespace lbc
