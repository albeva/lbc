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
        auto declare(AstStmtList& ast) -> Result<void>;
        auto declare(AstDecl& ast) -> Result<void>;
        auto declareAndDefine(const std::vector<AstVarDecl*>& vars) -> Result<void>;
        auto declareAndDefine(AstVarDecl& var) -> Result<void>;
        auto define(AstDecl& ast) -> Result<void>;

    private:
        auto defineFunc(AstFuncDecl& ast) -> Result<void>;
        auto defineFuncParam(AstFuncParamDecl& ast) -> Result<void>;
        auto defineAlias(AstTypeAlias& ast) -> Result<void>;
        auto defineUdt(AstUdtDecl& ast) -> Result<void>;
        auto defineVar(AstVarDecl& ast) -> Result<void>;
        auto createNewSymbol(AstDecl& ast, const TypeRoot* type) -> Result<Symbol*>;
    };
} // namespace Sem
} // namespace lbc
