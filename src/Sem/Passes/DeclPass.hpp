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
        auto declare(const AstStmtList& ast) const -> Result<void>;
        auto declare(AstDecl& ast) const -> Result<void>;
        auto declareAndDefine(const std::vector<AstVarDecl*>& vars) -> Result<void>;
        auto declareAndDefine(AstVarDecl& var) -> Result<void>;
        auto define(AstDecl& ast) -> Result<void>;

    private:
        auto defineFunc(AstFuncDecl& ast) const -> Result<void>;
        auto defineFuncParam(AstFuncParamDecl& ast) const -> Result<void>;
        auto defineAlias(const AstTypeAlias& ast) const -> Result<void>;
        auto defineUdt(AstUdtDecl& ast) -> Result<void>;
        auto defineVar(AstVarDecl& ast) const -> Result<void>;
        auto createNewSymbol(AstDecl& ast, const TypeRoot* type) const -> Result<Symbol*>;
    };
} // namespace Sem
} // namespace lbc
