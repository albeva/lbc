//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
using namespace lbc;

namespace {
[[nodiscard]] auto isAccessibleVariable(const Symbol* symbol) -> bool {
    return symbol->hasFlag(SymbolFlags::Defined);
}
} // namespace

auto SemanticAnalyser::expression(AstExpr& ast, const Type* implicitType) -> Result {
    const ValueRestorer restorer { m_implicitType, m_suggestedType };
    m_implicitType = implicitType;
    m_suggestedType = nullptr;
    TRY(visit(ast));

    // If we inferred a new type that whole expression should be, re-run the visitor.
    // This is so that types would propagate down:
    // DIM b = 1 + 2 + 3 AS Byte ' this should result in Byte type
    if (m_suggestedType != nullptr) {
        m_implicitType = m_suggestedType;
        m_suggestedType = nullptr;
        TRY(visit(ast));
    }
    return {};
}

auto SemanticAnalyser::accept(AstCastExpr& ast) -> Result {
    TRY(visit(*ast.getExpr()))
    if (ast.getImplicit() && m_implicitType != nullptr) {
        ast.setType(ast.getExpr()->getType());
        return {};
    }

    if (ast.getType() == nullptr) {
        TRY(visit(*ast.getTypeExpr()));
        ast.setType(ast.getTypeExpr()->getType());
    }
    setSuggestedType(ast.getType());
    return {};
}

auto SemanticAnalyser::accept(AstVarExpr& ast) -> Result {
    auto* symbol = m_symbolTable->find(ast.getName());
    if (symbol == nullptr) {
        return notImplemented(); // TODO: unnknown identifier
    }

    if (!isAccessibleVariable(symbol)) {
        return notImplemented(); // TODO: use before definition
    }

    ast.setSymbol(symbol);
    ast.setType(symbol->getType());
    setSuggestedType(symbol->getType());
    return {};
}

auto SemanticAnalyser::accept(AstCallExpr& ast) -> Result {
    // visit the callee expression
    auto& callee = *ast.getCallee();
    TRY(visit(callee));

    // check that callee is a function type
    const auto* type = llvm::dyn_cast<TypeFunction>(callee.getType());
    if (type == nullptr) {
        return notImplemented(); // TODO: uncallable callee
    }

    /// check that paramater and argument counts match
    const auto& params = type->getParams();
    const auto& args = ast.getArgs();
    if (params.size() != args.size()) {
        return notImplemented(); // TODO: expected argument count mismatch
    }

    // visit each argument and check its type against the parameter type
    std::size_t index = 0;
    for (auto& arg : args) {
        const auto* param = params[index++];
        TRY(expression(*arg, param));
        if (param != arg->getType()) {
            return notImplemented(); // TODO: argument type mismatch
        }
    }

    // set the type of the call expression to the return type of the function
    ast.setType(type->getReturnType());
    setSuggestedType(type->getReturnType());
    return {};
}

auto SemanticAnalyser::accept(AstLiteralExpr& ast) -> Result {
    const auto visitor = Visitor {
        [&](std::monostate) -> const Type* {
            if (m_implicitType && m_implicitType->isPointer()) {
                return m_implicitType;
            }
            return getTypeFactory().getNull();
        },
        [&](const llvm::StringRef) -> const Type* {
            return getTypeFactory().getZString();
        },
        [&](const std::uint64_t) -> const Type* {
            if (m_implicitType && m_implicitType->isIntegral()) {
                return m_implicitType;
            }
            return getTypeFactory().getInteger();
        },
        [&](const double) -> const Type* {
            if (m_implicitType && m_implicitType->isFloatingPoint()) {
                return m_implicitType;
            }
            return getTypeFactory().getDouble();
        },
        [&](const bool) -> const Type* {
            return getTypeFactory().getBool();
        }
    };
    const auto* type = std::visit(visitor, ast.getValue().storage());
    setSuggestedType(type);
    ast.setType(type);
    return {};
}

auto SemanticAnalyser::accept(AstUnaryExpr& ast) -> Result {
    TRY(visit(*ast.getExpr()));

    const auto* type = ast.getExpr()->getType();
    const auto op = ast.getOp();

    if (op == TokenKind::Negate) {
        if (!type->isNumeric()) {
            return notImplemented(); // TODO: negating non numeric type
        }
        if (type->isUnsignedIntegral()) {
            return notImplemented(); // TODO: negating non-signed type
        }
        ast.setType(type);
        return {};
    }

    if (op == TokenKind::LogicalNot) {
        if (!type->isBool()) {
            return notImplemented(); // TODO: not on non bool type
        }
        ast.setType(type);
        return {};
    }

    return notImplemented(); // TODO: unhandled unary operator
}

auto SemanticAnalyser::accept(AstBinaryExpr& ast) -> Result {
    TRY(visit(*ast.getLeft()));
    TRY(visit(*ast.getRight()));

    // const auto [common, coerce] = findCommonType(ast.getLeft(), ast.getRight());
    const auto* common = ast.getLeft()->getType()->common(ast.getRight()->getType());
    if (common == nullptr) {
        return notImplemented(); // TODO: incompatible types for binary expression
    }

    const auto op = ast.getOp();
    if (common->isBool()) {
        if (!op.isComparison() && !op.isLogical()) {
            return notImplemented(); // TODO: invalid binary operation on bool type
        }
    } else if (common->isNumeric()) {
        if (op.isLogical() || (op == TokenKind::Modulus && !common->isIntegral())) {
            return notImplemented(); // TODO: invalid binary operation on a numeric type
        }
    } else {
        return notImplemented(); // TODO: Invalid Binary Expression type
    }

    // update left side if needed
    TRY_DECL(newLhs, castOrCoerce(ast.getLeft(), common))
    ast.setLeft(newLhs);

    // update right side if needed
    TRY_DECL(newRhs, castOrCoerce(ast.getRight(), common))
    ast.setRight(newRhs);

    // Set result
    if (op.isComparison() || op.isLogical()) {
        ast.setType(getTypeFactory().getBool());
    } else {
        ast.setType(common);
    }

    return {};
}

auto SemanticAnalyser::accept(AstMemberExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::coerce(AstExpr* ast, const Type* targetType) -> DiagResult<AstExpr*> {
    switch (const auto res = ast->getType()->compare(targetType); res.result) {
    case TypeComparisonResult::Incompatible:
        if (auto* literal = llvm::dyn_cast<AstLiteralExpr>(ast)) {
            TRY(coerceLiteral(literal, targetType));
            return nullptr;
        }
        return notImplemented(); // TODO: implicit conversion error
    case TypeComparisonResult::Identical:
        return nullptr;
    case TypeComparisonResult::Convertible:
        return castOrCoerce(ast, targetType);
    default:
        std::unreachable();
    }
}

auto SemanticAnalyser::castOrCoerce(AstExpr* ast, const Type* targetType) -> DiagResult<AstExpr*> {
    auto* castExpr = llvm::dyn_cast<AstCastExpr>(ast); // NOLINT

    // If this was implicit or a useless cast, then drop it.
    if (castExpr != nullptr) {
        if (castExpr->getImplicit() || castExpr->getType() == castExpr->getExpr()->getType()) {
            ast = castExpr->getExpr();
        }
    }

    if (ast->getType() == targetType) {
        return ast;
    }

    // We can coerce literals to the target type
    if (auto* lit = llvm::dyn_cast<AstLiteralExpr>(ast)) {
        TRY(coerceLiteral(lit, targetType))
        return ast;
    }

    // We can reuse the cast expression.
    if (castExpr != nullptr) {
        castExpr->setType(targetType);
        castExpr->setImplicit(true);
    } else {
        castExpr = getContext().create<AstCastExpr>(ast->getRange(), ast, nullptr, true);
        castExpr->setType(targetType);
    }

    // done
    return castExpr;
}

auto SemanticAnalyser::coerceLiteral(AstLiteralExpr* ast, const Type* targetType) -> Result {
    const auto& value = ast->getValue();
    if (targetType->isIntegral() && value.isIntegral()) {
        ast->setType(targetType);
        return {};
    }
    if (targetType->isFloatingPoint() && value.isFloatingPoint()) {
        ast->setType(targetType);
        return {};
    }
    return notImplemented(); // TODO: return implicit conversion error
}

void SemanticAnalyser::setSuggestedType(const Type* implicitType) {
    if (m_implicitType != nullptr) {
        return;
    }

    if (m_suggestedType == nullptr) {
        m_suggestedType = implicitType;
    } else {
        m_suggestedType = m_suggestedType->common(implicitType);
    }
}
