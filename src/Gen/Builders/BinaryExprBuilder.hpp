//
// Created by Albert on 28/05/2021.
//
#pragma once
#include "pch.hpp"
#include "Ast/Ast.hpp"
#include "Builder.hpp"
#include "Gen/CodeGen.hpp"
#include "Gen/ValueHandler.hpp"

namespace lbc::Gen {

class BinaryExprBuilder final : Builder<AstBinaryExpr> {
public:
    using Builder::Builder;
    auto build() -> ValueHandler;

private:
    auto comparison() -> ValueHandler;
    auto arithmetic() -> ValueHandler;
    auto logical() -> ValueHandler;
};

} // namespace lbc::Gen
