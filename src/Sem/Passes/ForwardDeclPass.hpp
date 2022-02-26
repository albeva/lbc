//
// Created by Albert on 26/02/2022.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
struct AstModule;

namespace Sem {

    /**
     * Forward declare all user defined types, aliases and precedures
     */
    class ForwardDeclPass final : public Pass {
    public:
        using Pass::Pass;
        void visit(AstModule&) const noexcept;
    };

} // namespace Sem
} // namespace lbc
