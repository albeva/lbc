//
// Created by Albert Varaksin on 19/02/2026.
//
#include "Driver/Context.hpp"
#include "SemanticAnalyser.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Aggregate.hpp"
#include "Type/Compound.hpp"
using namespace lbc;

// =============================================================================
// Entry point
// =============================================================================

auto SemanticAnalyser::expression(AstExpr& ast, const Type* implicitType) -> Result {
    const ValueRestorer restore { m_implicitType, m_suggestedType };
    m_implicitType = implicitType;
    m_suggestedType = nullptr;
    TRY(visit(ast));
    if (m_implicitType != nullptr && ast.getType() != m_implicitType) {
        TRY_DECL(result, castOrCoerce(&ast, m_implicitType));
        (void)result;
    }
    return {};
}

// =============================================================================
// Helpers
// =============================================================================

auto SemanticAnalyser::coerceLiteral(AstLiteralExpr* ast, const Type* targetType) -> Result {
    const auto value = ast->getValue();

    // Integral literal → any integral type
    if (value.isIntegral() && targetType->isIntegral()) {
        ast->setType(targetType);
        return {};
    }

    // Float literal → any float type
    if (value.isFloatingPoint() && targetType->isFloatingPoint()) {
        ast->setType(targetType);
        return {};
    }

    // Null literal → any pointer type
    if (value.isNull() && targetType->isPointer()) {
        ast->setType(targetType);
        return {};
    }

    // No cross-family coercion
    return diag(diagnostics::typeMismatch(ast->getType()->getTokenKind().value_or(TokenKind::Invalid),
        targetType->getTokenKind().value_or(TokenKind::Invalid)), ast->getRange().Start, ast->getRange());
}

auto SemanticAnalyser::coerce(AstExpr* ast, const Type* targetType) -> DiagResult<AstExpr*> {
    if (ast->getType() == targetType) {
        return ast;
    }

    auto cmp = targetType->compare(ast->getType());
    if (cmp.result != TypeComparisonResult::Incompatible) {
        auto* typeExpr = m_context.create<AstBuiltInType>(ast->getRange(), targetType->getTokenKind().value_or(TokenKind::Invalid));
        typeExpr->setType(targetType);
        auto* cast = m_context.create<AstCastExpr>(ast->getRange(), ast, typeExpr, true);
        cast->setType(targetType);
        return cast;
    }

    return diag(diagnostics::typeMismatch(ast->getType()->getTokenKind().value_or(TokenKind::Invalid),
        targetType->getTokenKind().value_or(TokenKind::Invalid)), ast->getRange().Start, ast->getRange());
}

auto SemanticAnalyser::castOrCoerce(AstExpr* ast, const Type* targetType) -> DiagResult<AstExpr*> {
    if (auto* literal = llvm::dyn_cast<AstLiteralExpr>(ast)) {
        if (coerceLiteral(literal, targetType).has_value()) {
            return ast;
        }
    }
    return coerce(ast, targetType);
}

void SemanticAnalyser::setSuggestedType(const Type* type) {
    if (m_suggestedType == nullptr) {
        m_suggestedType = type;
    }
}

// =============================================================================
// Expressions
// =============================================================================

auto SemanticAnalyser::accept(AstLiteralExpr& ast) -> Result {
    auto& factory = getTypeFactory();
    const auto value = ast.getValue();

    const Type* naturalType = nullptr;
    if (value.isIntegral()) {
        naturalType = factory.getInteger();
    } else if (value.isFloatingPoint()) {
        naturalType = factory.getDouble();
    } else if (value.isBool()) {
        naturalType = factory.getBool();
    } else if (value.isString()) {
        naturalType = factory.getZString();
    } else if (value.isNull()) {
        naturalType = factory.getNull();
    }

    ast.setType(naturalType);

    // Try coercion to suggested or implicit type
    if (m_suggestedType != nullptr) {
        (void)coerceLiteral(&ast, m_suggestedType);
    } else if (m_implicitType != nullptr) {
        (void)coerceLiteral(&ast, m_implicitType);
    }

    return {};
}

auto SemanticAnalyser::accept(AstVarExpr& ast) -> Result {
    auto* symbol = m_symbolTable->find(ast.getName(), true);
    if (symbol == nullptr) {
        return diag(diagnostics::undeclaredIdentifier(ast.getName()), ast.getRange().Start, ast.getRange());
    }

    ast.setSymbol(symbol);
    ast.setType(symbol->getType());
    return {};
}

auto SemanticAnalyser::accept(AstUnaryExpr& ast) -> Result {
    TRY(visit(*ast.getExpr()));

    const auto* operandType = ast.getExpr()->getType();
    const auto op = ast.getOp();

    if (op == TokenKind::Negate) {
        if (!operandType->isNumeric()) {
            return diag(diagnostics::invalidOperands(op, operandType->getTokenKind().value_or(TokenKind::Invalid),
                operandType->getTokenKind().value_or(TokenKind::Invalid)), ast.getRange().Start, ast.getRange());
        }
        ast.setType(operandType);
    } else if (op == TokenKind::LogicalNot) {
        if (!operandType->isBool()) {
            return diag(diagnostics::invalidOperands(op, operandType->getTokenKind().value_or(TokenKind::Invalid),
                operandType->getTokenKind().value_or(TokenKind::Invalid)), ast.getRange().Start, ast.getRange());
        }
        ast.setType(operandType);
    } else if (op == TokenKind::AddressOf) {
        if (operandType->isNull()) {
            return diag(diagnostics::addressOfNull(), ast.getRange().Start, ast.getRange());
        }
        if (operandType->isReference()) {
            ast.setType(getTypeFactory().getPointer(operandType->getBaseType()));
        } else {
            ast.setType(getTypeFactory().getPointer(operandType));
        }
    } else if (op == TokenKind::Dereference) {
        if (!operandType->isPointer()) {
            return diag(diagnostics::invalidOperands(op, operandType->getTokenKind().value_or(TokenKind::Invalid),
                operandType->getTokenKind().value_or(TokenKind::Invalid)), ast.getRange().Start, ast.getRange());
        }
        ast.setType(operandType->getBaseType());
    }

    return {};
}

auto SemanticAnalyser::accept(AstBinaryExpr& ast) -> Result {
    const auto op = ast.getOp();
    const auto category = op.getCategory();

    // Analyse operands
    TRY(visit(*ast.getLeft()));
    TRY(visit(*ast.getRight()));

    auto* left = ast.getLeft();
    auto* right = ast.getRight();

    if (category == TokenKind::Category::Arithmetic || category == TokenKind::Category::Comparison) {
        // If a suggested type appeared from a sub-expression, try coercing literal operands
        if (m_suggestedType != nullptr) {
            if (auto* lit = llvm::dyn_cast<AstLiteralExpr>(left)) {
                (void)coerceLiteral(lit, m_suggestedType);
            }
            if (auto* lit = llvm::dyn_cast<AstLiteralExpr>(right)) {
                (void)coerceLiteral(lit, m_suggestedType);
            }
        }

        // If types still differ and one is a literal, coerce the literal to match
        if (left->getType() != right->getType()) {
            if (auto* leftLit = llvm::dyn_cast<AstLiteralExpr>(left)) {
                (void)coerceLiteral(leftLit, right->getType());
            } else if (auto* rightLit = llvm::dyn_cast<AstLiteralExpr>(right)) {
                (void)coerceLiteral(rightLit, left->getType());
            }
        }

        // Find common type
        const auto* commonType = left->getType()->common(right->getType());
        if (commonType == nullptr) {
            return diag(diagnostics::invalidOperands(op,
                left->getType()->getTokenKind().value_or(TokenKind::Invalid),
                right->getType()->getTokenKind().value_or(TokenKind::Invalid)),
                ast.getRange().Start, ast.getRange());
        }

        // Coerce operands to common type
        TRY_ASSIGN(left, coerce(left, commonType));
        ast.setLeft(left);
        TRY_ASSIGN(right, coerce(right, commonType));
        ast.setRight(right);

        if (category == TokenKind::Category::Comparison) {
            ast.setType(getTypeFactory().getBool());
        } else {
            ast.setType(commonType);
        }
    } else if (category == TokenKind::Category::Logical) {
        if (!left->getType()->isBool()) {
            return diag(diagnostics::invalidOperands(op,
                left->getType()->getTokenKind().value_or(TokenKind::Invalid),
                right->getType()->getTokenKind().value_or(TokenKind::Invalid)),
                ast.getRange().Start, ast.getRange());
        }
        if (!right->getType()->isBool()) {
            return diag(diagnostics::invalidOperands(op,
                left->getType()->getTokenKind().value_or(TokenKind::Invalid),
                right->getType()->getTokenKind().value_or(TokenKind::Invalid)),
                ast.getRange().Start, ast.getRange());
        }
        ast.setType(getTypeFactory().getBool());
    }

    return {};
}

auto SemanticAnalyser::accept(AstCastExpr& ast) -> Result {
    TRY(visit(*ast.getExpr()));
    TRY(visit(*ast.getTypeExpr()));

    const auto* targetType = ast.getTypeExpr()->getType();
    ast.setType(targetType);
    setSuggestedType(targetType);

    return {};
}

auto SemanticAnalyser::accept(AstCallExpr& ast) -> Result {
    TRY(visit(*ast.getCallee()));

    const auto* calleeType = ast.getCallee()->getType();
    const auto* funcType = llvm::dyn_cast<TypeFunction>(calleeType);
    if (funcType == nullptr) {
        return diag(diagnostics::typeMismatch(
            calleeType->getTokenKind().value_or(TokenKind::Invalid),
            TokenKind(TokenKind::Function)),
            ast.getRange().Start, ast.getRange());
    }

    const auto params = funcType->getParams();
    const auto args = ast.getArgs();

    if (args.size() > params.size()) {
        return diag(diagnostics::tooManyArguments(
            static_cast<int>(params.size()), static_cast<int>(args.size())),
            ast.getRange().Start, ast.getRange());
    }
    if (args.size() < params.size()) {
        return diag(diagnostics::tooFewArguments(
            static_cast<int>(params.size()), static_cast<int>(args.size())),
            ast.getRange().Start, ast.getRange());
    }

    for (std::size_t i = 0; i < args.size(); i++) {
        TRY(expression(*args[i], params[i]));
    }

    ast.setType(funcType->getReturnType());
    return {};
}

auto SemanticAnalyser::accept(AstMemberExpr& /*ast*/) -> Result {
    return notImplemented();
}
