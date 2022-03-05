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
struct AstTypeOf;

namespace Sem {
    class TypePass final : public Pass {
    public:
        using Pass::Pass;
        /// Resolve type of given type expression. If owner is provided then
        /// certain avoid creating extra proxies for pointers
        [[nodiscard]] TypeProxy* visit(AstTypeExpr& ast, TypeProxy* owner = nullptr) const noexcept;
        [[nodiscard]] TypeProxy* visit(AstFuncDecl& ast) const noexcept;
        [[nodiscard]] TypeProxy* visit(AstTypeOf& ast) const noexcept;

    private:
        [[nodiscard]] TypeProxy* visit(AstIdentExpr& ast) const noexcept;
    };
} // namespace Sem
} // namespace lbc
