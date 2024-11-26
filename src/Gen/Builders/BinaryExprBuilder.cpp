//
// Created by Albert on 28/05/2021.
//
#include "BinaryExprBuilder.hpp"
#include "Gen/Helpers.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Gen;

auto BinaryExprBuilder::build() -> ValueHandler {
    switch (Token::getOperatorType(m_ast.token.getKind())) {
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

auto BinaryExprBuilder::comparison() -> ValueHandler {
    auto* lhsValue = m_gen.visit(*m_ast.lhs).load();
    auto* rhsValue = m_gen.visit(*m_ast.rhs).load();

    const auto* ty = m_ast.lhs->type;
    auto pred = Gen::getCmpPred(ty, m_ast.token.getKind());
    return { &m_gen, m_ast.type, m_builder.CreateCmp(pred, lhsValue, rhsValue) };
}

auto BinaryExprBuilder::arithmetic() -> ValueHandler {
    auto* lhsValue = m_gen.visit(*m_ast.lhs).load();
    auto* rhsValue = m_gen.visit(*m_ast.rhs).load();

    auto op = getBinOpPred(m_ast.lhs->type, m_ast.token.getKind());
    return { &m_gen, m_ast.type, m_builder.CreateBinOp(op, lhsValue, rhsValue) };
}

auto BinaryExprBuilder::logical() -> ValueHandler {
    // lhs
    auto* lhsValue = m_gen.visit(*m_ast.lhs).load();
    auto* lhsBlock = m_builder.GetInsertBlock();

    auto* func = lhsBlock->getParent();
    const auto isAnd = m_ast.token.getKind() == TokenKind::LogicalAnd;
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
    auto* rhsValue = m_gen.visit(*m_ast.rhs).load();
    auto* rhsBlock = m_builder.GetInsertBlock();

    // phi
    m_gen.switchBlock(endBlock);
    auto* phi = m_builder.CreatePHI(m_ast.type->getLlvmType(m_gen.getContext()), 2);

    if (isAnd) {
        phi->addIncoming(m_gen.getFalse(), lhsBlock);
    } else {
        phi->addIncoming(m_gen.getTrue(), lhsBlock);
    }
    phi->addIncoming(rhsValue, rhsBlock);
    return { &m_gen, m_ast.type, phi };
}
