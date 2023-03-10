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
        void visit(AstForStmt& ast) const ;

    private:
        void declare(AstForStmt& ast) const ;
        void analyze(AstForStmt& ast) const ;
        void determineForDirection(AstForStmt& ast) const ;
    };
} // namespace Sem
} // namespace lbc
