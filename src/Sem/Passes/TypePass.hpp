//
// Created by Albert Varaksin on 22/05/2021.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
class TypeProxy;
struct AstTypeExpr;
struct AstIdentExpr;
struct AstFuncDecl;

namespace Sem {
    class TypePass final : public Pass {
    public:
        using Pass::Pass;
        [[nodiscard]] TypeProxy* visit(AstTypeExpr& ast) const noexcept;
        [[nodiscard]] TypeProxy* visit(AstFuncDecl& ast) const noexcept;

    private:
        [[nodiscard]] TypeProxy* visit(AstIdentExpr& ast) const noexcept;
    };
} // namespace Sem
} // namespace lbc
