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
        Result<void> declare(AstStmtList& ast);
        Result<void> declare(AstDecl& ast);
        Result<void> declareAndDefine(const std::vector<AstVarDecl*>& vars);
        Result<void> declareAndDefine(AstVarDecl& var);
        Result<void> define(AstDecl& ast);

    private:
        Result<void> defineFunc(AstFuncDecl& ast);
        Result<void> defineFuncParam(AstFuncParamDecl& ast);
        Result<void> defineAlias(AstTypeAlias& ast);
        Result<void> defineUdt(AstUdtDecl& ast);
        Result<void> defineVar(AstVarDecl& ast);
        Result<Symbol*> createNewSymbol(AstDecl& ast, const TypeRoot* type);
    };
} // namespace Sem
} // namespace lbc
