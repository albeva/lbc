//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IR/lib/Function.hpp"
#include "IR/lib/Literal.hpp"
#include "IR/lib/Temporary.hpp"
#include "IrGenerator.hpp"
#include "Symbol/Symbol.hpp"
using namespace lbc::ir::gen;

auto IrGenerator::accept(AstCastExpr& ast) -> Result {
    TRY(visit(*ast.getExpr()));
    auto* tmp = createTemporary(ast.getType());
    emit(makeCast(tmp, ast.getType(), ast.getExpr()->getOperand()));
    ast.setOperand(tmp);
    return {};
}

auto IrGenerator::accept(AstVarExpr& ast) -> Result {
    ast.setOperand(ast.getSymbol()->getOperand());
    return {};
}

auto IrGenerator::accept(AstCallExpr& ast) -> Result {
    // Visit callee
    TRY(visit(*ast.getCallee()));

    // Visit arguments
    const auto args = ast.getArgs();
    auto argValues = getContext().span<lib::Value*>(args.size());
    for (std::size_t i = 0; i < args.size(); i++) {
        TRY(visit(*args[i]));
        argValues[i] = args[i]->getOperand();
    }

    // TODO: Implement support for indirect function calls

    // Emit call
    auto* callee = llvm::cast<lib::Function>(ast.getCallee()->getOperand());
    auto* tmp = createTemporary(ast.getType());
    emit(makeCall(tmp, callee, argValues));
    ast.setOperand(tmp);
    return {};
}

auto IrGenerator::accept(AstLiteralExpr& ast) const -> Result {
    auto* literal = getContext().create<lib::Literal>(ast.getType(), ast.getValue());
    ast.setOperand(literal);
    return {};
}

auto IrGenerator::accept(AstUnaryExpr& ast) -> Result {
    TRY(visit(*ast.getExpr()));
    auto* operand = ast.getExpr()->getOperand();
    auto* tmp = createTemporary(ast.getType());

    switch (ast.getOp().value()) {
    case TokenKind::Negate:
        emit(makeNeg(tmp, operand));
        break;
    case TokenKind::LogicalNot:
        emit(makeNot(tmp, operand));
        break;
    case TokenKind::AddressOf:
        emit(makeAddrof(tmp, operand));
        break;
    case TokenKind::Dereference:
        emit(makeLoad(tmp, operand));
        break;
    default:
        std::unreachable();
    }

    ast.setOperand(tmp);
    return {};
}

auto IrGenerator::accept(AstBinaryExpr& ast) -> Result {
    TRY(visit(*ast.getLeft()));
    TRY(visit(*ast.getRight()));

    auto* lhs = ast.getLeft()->getOperand();
    auto* rhs = ast.getRight()->getOperand();
    auto* tmp = createTemporary(ast.getType());

    switch (ast.getOp().value()) {
    case TokenKind::Plus:
        emit(makeAdd(tmp, lhs, rhs));
        break;
    case TokenKind::Minus:
        emit(makeSub(tmp, lhs, rhs));
        break;
    case TokenKind::Multiply:
        emit(makeMul(tmp, lhs, rhs));
        break;
    case TokenKind::Divide:
        emit(makeDiv(tmp, lhs, rhs));
        break;
    case TokenKind::Modulus:
        emit(makeMod(tmp, lhs, rhs));
        break;
    case TokenKind::LogicalAnd:
        emit(makeAnd(tmp, lhs, rhs));
        break;
    case TokenKind::LogicalOr:
        emit(makeOr(tmp, lhs, rhs));
        break;
    case TokenKind::Equal:
        emit(makeEq(tmp, lhs, rhs));
        break;
    case TokenKind::NotEqual:
        emit(makeNe(tmp, lhs, rhs));
        break;
    case TokenKind::LessThan:
        emit(makeLt(tmp, lhs, rhs));
        break;
    case TokenKind::LessOrEqual:
        emit(makeLe(tmp, lhs, rhs));
        break;
    case TokenKind::GreaterThan:
        emit(makeGt(tmp, lhs, rhs));
        break;
    case TokenKind::GreaterOrEqual:
        emit(makeGe(tmp, lhs, rhs));
        break;
    default:
        std::unreachable();
    }

    ast.setOperand(tmp);
    return {};
}

auto IrGenerator::accept(AstMemberExpr& /*ast*/) -> Result {
    return notImplemented();
}
