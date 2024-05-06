//
// Created by Albert Varaksin on 22/05/2021.
//
#pragma once
#include "pch.hpp"
#include "Pass.hpp"

namespace lbc {
class TypeRoot;
struct AstTypeExpr;
struct AstIdentExpr;
struct AstFuncDecl;
struct AstTypeOf;

namespace Sem {
    class TypePass final : public Pass {
    public:
        using Pass::Pass;
        [[nodiscard]] Result<const TypeRoot*> visit(AstTypeExpr& ast) const;
        [[nodiscard]] Result<const TypeRoot*> visit(AstFuncDecl& ast) const;
        [[nodiscard]] Result<const TypeRoot*> visit(AstTypeOf& ast) const;

    private:
        [[nodiscard]] Result<const TypeRoot*> visit(AstIdentExpr& ast) const;
    };
} // namespace Sem
} // namespace lbc
