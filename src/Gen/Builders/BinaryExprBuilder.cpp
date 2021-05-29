//
// Created by Albert on 28/05/2021.
//
#include "BinaryExprBuilder.hpp"
#include "Driver/Context.hpp"
#include "Gen/Helpers.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Gen;

llvm::Value* BinaryExprBuilder::build() noexcept {
    switch (Token::getOperatorType(m_ast->tokenKind)) {
    case OperatorType::Arithmetic:
        return arithmetic();
    case OperatorType::Logical:
        return logical();
    case OperatorType::Comparison:
        return comparison();
    default:
        llvm_unreachable("invalid operator type");
    }
}

llvm::Value* BinaryExprBuilder::comparison() noexcept {
    auto* lhsValue = m_gen.visit(m_ast->lhs.get());
    auto* rhsValue = m_gen.visit(m_ast->rhs.get());

    const auto* ty = m_ast->lhs->type;
    auto pred = Gen::getCmpPred(ty, m_ast->tokenKind);
    return m_builder.CreateCmp(pred, lhsValue, rhsValue);
}

llvm::Value* BinaryExprBuilder::arithmetic() noexcept {
    auto* lhsValue = m_gen.visit(m_ast->lhs.get());
    auto* rhsValue = m_gen.visit(m_ast->rhs.get());

    auto op = getBinOpPred(m_ast->lhs->type, m_ast->tokenKind);
    return m_builder.CreateBinOp(op, lhsValue, rhsValue);
}

llvm::Value* BinaryExprBuilder::logical() noexcept {
    // lhs
    auto* lhsValue = m_gen.visit(m_ast->lhs.get());
    auto* lhsBlock = m_builder.GetInsertBlock();

    auto* func = lhsBlock->getParent();
    const auto isAnd = m_ast->tokenKind == TokenKind::LogicalAnd;
    auto prefix = isAnd ? "and"s : "or"s;
    auto* elseBlock = llvm::BasicBlock::Create(m_llvmContext, prefix, func);
    auto* endBlock = llvm::BasicBlock::Create(m_llvmContext, prefix + ".end", func);

    if (isAnd) {
        m_builder.CreateCondBr(lhsValue, elseBlock, endBlock);
    } else {
        m_builder.CreateCondBr(lhsValue, endBlock, elseBlock);
    }

    // rhs
    m_builder.SetInsertPoint(elseBlock);
    auto* rhsValue = m_gen.visit(m_ast->rhs.get());
    auto* rhsBlock = m_builder.GetInsertBlock();

    // phi
    m_gen.switchBlock(endBlock);
    auto* phi = m_builder.CreatePHI(m_ast->type->getLlvmType(m_gen.getContext()), 2);

    if (isAnd) {
        phi->addIncoming(m_gen.getFalse(), lhsBlock);
    } else {
        phi->addIncoming(m_gen.getTrue(), lhsBlock);
    }
    phi->addIncoming(rhsValue, rhsBlock);
    return phi;
}