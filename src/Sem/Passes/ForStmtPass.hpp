//
// Created by Albert on 29/05/2021.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
struct AstForStmt;

namespace Sem {
    class ForStmtPass final : public Pass {
    public:
        using Pass::Pass;
        void visit(AstForStmt& ast) const noexcept;

    private:
        void declare(AstForStmt& ast) const noexcept;
        void analyze(AstForStmt& ast) const noexcept;
        void determineForDirection(AstForStmt& ast) const noexcept;
    };
} // namespace Sem
} // namespace lbc
