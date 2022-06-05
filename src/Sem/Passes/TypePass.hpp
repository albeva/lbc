//
// Created by Albert Varaksin on 22/05/2021.
//
#pragma once
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
        [[nodiscard]] const TypeRoot* visit(AstTypeExpr& ast) const noexcept;
        [[nodiscard]] const TypeRoot* visit(AstFuncDecl& ast) const noexcept;
        [[nodiscard]] const TypeRoot* visit(AstTypeOf& ast) const noexcept;

    private:
        [[nodiscard]] const TypeRoot* visit(AstIdentExpr& ast) const noexcept;
    };
} // namespace Sem
} // namespace lbc
