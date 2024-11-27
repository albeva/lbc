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
        [[nodiscard]] auto visit(AstTypeExpr& ast) const -> Result<const TypeRoot*>;
        [[nodiscard]] auto visit(AstFuncDecl& ast) const -> Result<const TypeRoot*>;
        [[nodiscard]] auto visit(AstTypeOf& ast) const -> Result<const TypeRoot*>;

    private:
        [[nodiscard]] auto visit(AstIdentExpr& ast) const -> Result<const TypeRoot*>;
    };
} // namespace Sem
} // namespace lbc
