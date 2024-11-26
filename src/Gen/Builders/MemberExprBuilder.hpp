//
// Created by Albert Varaksin on 05/05/2024.
//
#pragma once
#include "pch.hpp"
#include "Ast/Ast.hpp"
#include "Builder.hpp"
#include "Gen/CodeGen.hpp"
#include "Gen/ValueHandler.hpp"

namespace lbc::Gen {

class MemberExprBuilder final : Builder<AstMemberExpr> {
public:
    using Builder::Builder;
    [[nodiscard]] auto build() -> llvm::Value*;

private:
    void gep();
    void base(AstExpr& ast);
    [[nodiscard]] auto member(AstExpr& ast) -> Symbol*;

    llvm::Type* m_type = nullptr;
    llvm::Value* m_addr = nullptr;
    llvm::SmallVector<llvm::Value*> m_idxs;
};

} // namespace lbc::Gen
