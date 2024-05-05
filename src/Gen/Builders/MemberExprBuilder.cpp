//
// Created by Albert Varaksin on 05/05/2024.
//
#include "MemberExprBuilder.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Gen;

llvm::Value* MemberExprBuilder::build() {
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

    if (symbol->getType()->isPointer()) {
        gep();
        m_type = llvm::cast<TypePointer>(symbol->getType())->getBase()->getLlvmType(m_gen.getContext());
        m_addr = m_builder.CreateLoad(
            symbol->getType()->getLlvmType(m_gen.getContext()),
            m_addr
        );
    }
}

Symbol* MemberExprBuilder::member(AstExpr& ast) {
    auto* symbol = m_gen.visit(ast).dyn_cast<Symbol*>();
    if (symbol == nullptr) {
        fatalError("MemberAccess expressions shoudl be symbols!");
    }
    return symbol;
}