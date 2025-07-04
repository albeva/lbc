//
// Created by Albert Varaksin on 05/05/2024.
//
#include "MemberExprBuilder.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Gen;

auto MemberExprBuilder::build() -> llvm::Value* {
    m_idxs.push_back(m_builder.getInt32(0));
    base(*m_ast.base);
    auto* symbol = member(*m_ast.member);
    m_idxs.push_back(m_builder.getInt32(symbol->getIndex()));
    gep();
    return m_addr;
}

void MemberExprBuilder::gep() {
    m_addr = m_builder.CreateGEP(m_type, m_addr, m_idxs);
    m_idxs.pop_back_n(m_idxs.size() - 1);
}

void MemberExprBuilder::base(AstExpr& ast) {
    if (auto* memberExpr = llvm::dyn_cast<AstMemberExpr>(&ast)) {
        base(*memberExpr->base);
        base(*memberExpr->member);
        return;
    }

    auto* symbol = member(ast);
    if (m_addr == nullptr) {
        m_type = symbol->getType()->getLlvmType(m_gen.getContext());
        m_addr = symbol->getLlvmValue();
    } else {
        m_idxs.push_back(m_builder.getInt32(symbol->getIndex()));
    }

    if (const auto* ptr = llvm::dyn_cast<TypePointer>(symbol->getType())) {
        gep();
        m_type = ptr->getBase()->getLlvmType(m_gen.getContext());
        m_addr = m_builder.CreateLoad(
            ptr->getLlvmType(m_gen.getContext()),
            m_addr
        );
    } else if (const auto* ref = llvm::dyn_cast<TypeReference>(symbol->getType())) {
        gep();
        m_type = ref->getBase()->getLlvmType(m_gen.getContext());
        m_addr = m_builder.CreateLoad(
            ref->getLlvmType(m_gen.getContext()),
            m_addr
        );
    }
}

auto MemberExprBuilder::member(AstExpr& ast) -> Symbol* {
    auto* symbol = dyn_cast<Symbol*>(m_gen.visit(ast).value());
    if (symbol == nullptr) {
        fatalError("MemberAccess expressions should be symbols!");
    }
    return symbol;
}
