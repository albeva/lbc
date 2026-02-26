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

// Expression analysis uses two type propagation mechanisms:
//
// - m_implicitType (top-down): pushed from the caller, e.g. the declared
//   type of a variable in `DIM x AS BYTE = <expr>`. After the expression
//   visitor completes, if the result type differs from the implicit type
//   the expression is coerced or wrapped in an implicit cast.
//
// - m_suggestedType (bottom-up): set by any typed sub-expression (variables,
//   literals, casts, calls) and propagated upward through the expression tree.
//   When multiple suggestions compete, their common type is used. Allows
//   `2 + 3 AS BYTE` or `2 + b` (where b is BYTE) to type the literals as BYTE.
//
// Both are saved/restored per expression() call via ValueRestorer so nested
// expression analyses (e.g. function arguments) don't leak state.

auto SemanticAnalyser::expression(AstExpr& ast, const Type* explicitType) -> DiagResult<AstExpr*> {
    const ValueRestorer restore { m_explicitType, m_suggestedType };
    m_explicitType = explicitType;
    m_suggestedType = nullptr;

    TRY(visit(ast));

    if (m_explicitType != nullptr) {
        if (ast.getType() != m_explicitType) {
            return castOrCoerce(ast, m_explicitType);
        }
        return &ast;
    }

    if (m_suggestedType != nullptr) {
        m_explicitType = m_suggestedType;
        m_suggestedType = nullptr;
        TRY(visit(ast));
    }
    return &ast;
}

// =============================================================================
// Helpers
// =============================================================================

// Literal coercion re-types a literal node within its type family without
// inserting a cast node. This is valid because literals have no fixed storage
// — their bit representation adapts to the target type at codegen time.

auto SemanticAnalyser::coerceLiteral(AstLiteralExpr& ast, const Type* targetType) -> Result {
    // Already expected type?
    if (ast.getType() == targetType) {
        return {};
    }

    const auto value = ast.getValue();

    // Integral literal → any integral type
    if (value.isIntegral() && targetType->isIntegral()) {
        ast.setType(targetType);
        return {};
    }

    // Float literal → any float type
    if (value.isFloatingPoint() && targetType->isFloatingPoint()) {
        ast.setType(targetType);
        return {};
    }

    // Null literal → any pointer type
    if (value.isNull() && targetType->isPointer()) {
        ast.setType(targetType);
        return {};
    }

    // No cross-family coercion
    return diag(diagnostics::typeMismatch(*ast.getType(), *targetType), {}, ast.getRange());
}

auto SemanticAnalyser::coerce(AstExpr& ast, const Type* targetType) -> DiagResult<AstExpr*> {
    if (ast.getType() == targetType) {
        return &ast;
    }

    auto cmp = targetType->compare(ast.getType());
    if (cmp.result != TypeComparisonResult::Incompatible) {
        auto* cast = m_context.create<AstCastExpr>(ast.getRange(), &ast, nullptr, true);
        cast->setType(targetType);
        return cast;
    }

    return diag(diagnostics::typeMismatch(*ast.getType(), *targetType), {}, ast.getRange());
}

auto SemanticAnalyser::castOrCoerce(AstExpr& ast, const Type* targetType) -> DiagResult<AstExpr*> {
    if (auto* literal = llvm::dyn_cast<AstLiteralExpr>(&ast)) {
        if (coerceLiteral(*literal, targetType).has_value()) {
            return &ast;
        }
    }
    return coerce(ast, targetType);
}

void SemanticAnalyser::setSuggestedType(const Type* type) {
    if (m_explicitType != nullptr) {
        return;
    }

    if (m_suggestedType == nullptr) {
        m_suggestedType = type;
    } else {
        m_suggestedType = type->common(m_suggestedType);
    }
}

auto SemanticAnalyser::ensureAddressable(AstExpr& ast) -> Result {
    if (llvm::isa<AstVarExpr>(ast)) {
        return {};
    }
    return diag(diagnostics::nonAddressableExpr(), {}, ast.getRange());
}

// =============================================================================
// Expressions
// =============================================================================

// Determine the literal's natural type, then attempt coercion to the
// suggested type (from an AS cast) or implicit type (from the caller).
// Coercion only succeeds within the same type family.
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
    if (m_explicitType != nullptr) {
        TRY(coerceLiteral(ast, m_explicitType));
    } else if (m_suggestedType != nullptr) {
        TRY(coerceLiteral(ast, m_suggestedType));
    } else {
        setSuggestedType(ast.getType());
    }
    return {};
}

auto SemanticAnalyser::accept(AstVarExpr& ast) -> Result {
    auto* symbol = m_symbolTable->find(ast.getName(), true);
    if (symbol == nullptr) {
        return diag(diagnostics::undeclaredIdentifier(ast.getName()), {}, ast.getRange());
    }

    if (not symbol->hasFlag(SymbolFlags::Defined)) {
        return diag(diagnostics::useBeforeDefinition(symbol->getName()), {}, ast.getRange());
    }

    ast.setSymbol(symbol);
    ast.setType(symbol->getType()->removeReference());
    setSuggestedType(ast.getType());
    return {};
}

// Validate the operand type for each unary operator:
// - Negate: signed integral and floating-point types only
// - LogicalNot: boolean only
// - AddressOf: operand must be addressable (lvalue); produces a pointer
// - Dereference: pointer types only; produces the pointee type
auto SemanticAnalyser::accept(AstUnaryExpr& ast) -> Result {
    TRY(visit(*ast.getExpr()));

    const auto* operandType = ast.getExpr()->getType();
    const auto op = ast.getOp();

    if (op == TokenKind::Negate) {
        if (!(operandType->isSignedIntegral() || operandType->isFloatingPoint())) {

            return diag(diagnostics::invalidUnaryOperand(op, *operandType), {}, ast.getRange());
        }
        ast.setType(operandType);
    } else if (op == TokenKind::LogicalNot) {
        if (!operandType->isBool()) {
            return diag(diagnostics::invalidUnaryOperand(op, *operandType), {}, ast.getRange());
        }
        ast.setType(operandType);
    } else if (op == TokenKind::AddressOf) {
        TRY(ensureAddressable(*ast.getExpr()))
        ast.setType(getTypeFactory().getPointer(operandType));
    } else if (op == TokenKind::Dereference) {
        if (!operandType->isPointer()) {
            return diag(diagnostics::invalidUnaryOperand(op, *operandType), {}, ast.getRange());
        }
        ast.setType(operandType->getBaseType());
    } else {
        std::unreachable();
    }

    setSuggestedType(ast.getType());
    return {};
}

// Binary expression analysis by operator category:
//
// Arithmetic / Comparison:
//   1. Analyse both operands
//   2. Coerce literals to m_suggestedType (from AS cast) if available
//   3. If types still differ, coerce the literal side to match the non-literal
//   4. Find the common type; insert implicit casts for both operands if needed
//   5. Result is the common type (arithmetic) or BOOL (comparison)
//
// Logical (AND, OR):
//   Both operands must be BOOL. Result is BOOL.
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
                TRY(coerceLiteral(*lit, m_suggestedType));
            }
            if (auto* lit = llvm::dyn_cast<AstLiteralExpr>(right)) {
                TRY(coerceLiteral(*lit, m_suggestedType));
            }
        }

        // If types still differ and one is a literal, coerce the literal to match
        if (left->getType() != right->getType()) {
            if (auto* leftLit = llvm::dyn_cast<AstLiteralExpr>(left)) {
                TRY(coerceLiteral(*leftLit, right->getType()));
            } else if (auto* rightLit = llvm::dyn_cast<AstLiteralExpr>(right)) {
                TRY(coerceLiteral(*rightLit, left->getType()));
            }
        }

        // Find common type
        const auto* commonType = left->getType()->common(right->getType());
        if (commonType == nullptr) {
            return diag(diagnostics::invalidOperands(op, *left->getType(), *right->getType()), {}, ast.getRange());
        }

        // Coerce operands to common type
        TRY_ASSIGN(left, coerce(*left, commonType));
        ast.setLeft(left);
        TRY_ASSIGN(right, coerce(*right, commonType));
        ast.setRight(right);

        if (category == TokenKind::Category::Comparison) {
            ast.setType(getTypeFactory().getBool());
        } else {
            ast.setType(commonType);
        }
    } else if (category == TokenKind::Category::Logical) {
        if (!left->getType()->isBool() || !right->getType()->isBool()) {
            return diag(diagnostics::invalidOperands(op, *left->getType(), *right->getType()), {}, ast.getRange());
        }
        ast.setType(getTypeFactory().getBool());
    }
    setSuggestedType(ast.getType());
    return {};
}

// Analyse an explicit AS cast. Sets m_suggestedType so that sibling literals
// in parent binary expressions adopt the cast's target type.
auto SemanticAnalyser::accept(AstCastExpr& ast) -> Result {
    TRY(visit(*ast.getExpr()));
    const auto* from = ast.getExpr()->getType();

    const Type* to = nullptr;
    if (const auto* type = ast.getType()) {
        to = type;
    } else {
        TRY(visit(*ast.getTypeExpr()));
        to = ast.getTypeExpr()->getType();
    }

    if (!to->castable(from)) {
        return diag(diagnostics::typeMismatch(*from, *to), {}, ast.getRange());
    }
    ast.setType(to);
    setSuggestedType(to);
    return {};
}

// Validate the callee is a function type, check argument count against
// parameter count, and analyse each argument with its parameter type as
// the implicit type for coercion.
auto SemanticAnalyser::accept(AstCallExpr& ast) -> Result {
    TRY(visit(*ast.getCallee()));

    const auto* calleeType = ast.getCallee()->getType();
    const auto* funcType = llvm::dyn_cast<TypeFunction>(calleeType);
    if (funcType == nullptr) {
        return diag(diagnostics::notCallable(), {}, ast.getCallee()->getRange());
    }

    const auto params = funcType->getParams();
    const auto args = ast.getArgs();

    if (args.size() > params.size()) {
        return diag(diagnostics::tooManyArguments(params.size(), args.size()), {}, ast.getRange());
    }
    if (args.size() < params.size()) {
        return diag(diagnostics::tooFewArguments(params.size(), args.size()), {}, ast.getRange());
    }

    for (std::size_t i = 0; i < args.size(); i++) {
        TRY_ASSIGN(args[i], expression(*args[i], params[i]));
    }

    ast.setType(funcType->getReturnType());
    setSuggestedType(funcType->getReturnType());
    return {};
}

auto SemanticAnalyser::accept(AstMemberExpr& /*ast*/) -> Result {
    return notImplemented();
}
