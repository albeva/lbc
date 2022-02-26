//
// Created by Albert on 25/02/2022.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
struct AstModule;
struct AstTypeAlias;

namespace Sem {
    class TypeAliasDeclPass final: public Pass {
    public:
        using Pass::Pass;
        void visit(AstModule& ast) const noexcept;

    private:
        void visit(AstTypeAlias& ast) const noexcept;
    };
} // namespace Sem
} // namespace lbc