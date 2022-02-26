//
// Created by Albert on 29/05/2021.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
struct AstModule;
struct AstStmtList;
struct AstUdtDecl;

namespace Sem {
    class TypePass;

    class UdtDeclPass final : public Pass {
    public:
        using Pass::Pass;
        void visit(AstModule& ast) noexcept;

    private:
        void visit(AstStmtList& ast) noexcept;
        void visit(AstUdtDecl& ast) noexcept;
    };
} // namespace Sem
} // namespace lbc
