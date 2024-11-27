//
// Created by Albert on 29/05/2021.
//
#pragma once
#include "pch.hpp"
#include "Pass.hpp"

namespace lbc {
struct AstForStmt;

namespace Sem {
    class ForStmtPass final : public Pass {
    public:
        using Pass::Pass;
        auto visit(AstForStmt& ast) const -> Result<void>;

    private:
        auto declare(AstForStmt& ast) const -> Result<void>;
        auto analyze(AstForStmt& ast) const -> Result<void>;
        void determineForDirection(AstForStmt& ast) const;
    };
} // namespace Sem
} // namespace lbc
