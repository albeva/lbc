//
// Created by Albert on 26/02/2022.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
class Symbol;
struct AstModule;
struct AstStmtList;
struct AstDecl;
struct AstDeclList;
struct AstFuncDecl;
struct AstUdtDecl;
struct AstTypeAlias;

namespace Sem {

    /**
     * Forward declare all user defined types, aliases and precedures
     */
    class ForwardDeclPass final : public Pass {
    public:
        using Pass::Pass;
        void visit(AstModule&) noexcept;

    private:
        void declare(AstDecl& ast) noexcept;
        std::vector<Symbol*> m_types{};
        std::vector<Symbol*> m_procs{};
    };

} // namespace Sem
} // namespace lbc
