//
// Created by Albert Varaksin on 05/05/2021.
//
#include "ConstantFoldingPass.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Sem;

namespace {
template<typename BASE, typename T>
constexpr inline BASE castLiteral(const AstLiteralExpr& ast) {
    constexpr auto visitor = [](const auto& val) -> T {
        using R = std::decay_t<decltype(val)>;
        if constexpr (std::is_convertible_v<R, T>) {
            return static_cast<T>(val);
        } else {
            llvm_unreachable("Unsupported type conversion");
        }
    };
    return static_cast<BASE>(std::visit(visitor, ast.value));
}
} // namespace

void ConstantFoldingPass::fold(AstExpr*& ast) {
    //    if (m_context.getOptimizationLevel() == Context::OptimizationLevel::O0) {
    //        return;
    //    }
    AstExpr* replace = nullptr;
    switch (ast->kind) {
    case AstKind::UnaryExpr:
        replace = visitUnaryExpr(static_cast<AstUnaryExpr&>(*ast));
        break;
    case AstKind::BinaryExpr:
        replace = visitBinaryExpr(static_cast<AstBinaryExpr&>(*ast));
        break;
    case AstKind::CastExpr:
        replace = visitCastExpr(static_cast<AstCastExpr&>(*ast));
        break;
    case AstKind::IfExpr:
        replace = visitIfExpr(static_cast<AstIfExpr&>(*ast));
        break;
    default:
        return;
    }
    if (replace != nullptr) {
        ast = replace;
    }
}

AstExpr* ConstantFoldingPass::visitUnaryExpr(const AstUnaryExpr& ast) {
    auto* literal = llvm::dyn_cast<AstLiteralExpr>(ast.expr);
    if (literal == nullptr) {
        return nullptr;
    }

    auto value = unary(ast.tokenKind, *literal);
    auto* repl = m_sem.getContext().create<AstLiteralExpr>(ast.range, value);
    repl->type = ast.type;
    return repl;
}

AstLiteralExpr::Value ConstantFoldingPass::unary(TokenKind op, const AstLiteralExpr& ast) {
    switch (op) {
    case TokenKind::Negate: {
        static constexpr auto visitor = Visitor{
            [](uint64_t value) -> AstLiteralExpr::Value {
                return static_cast<uint64_t>(-static_cast<int64_t>(value));
            },
            [](double value) -> AstLiteralExpr::Value {
                return -value;
            },
            [](auto /*value*/) -> AstLiteralExpr::Value {
                llvm_unreachable("Non supported type");
            }
        };
        return std::visit(visitor, ast.value);
    }
    case TokenKind::LogicalNot: {
        static constexpr auto visitor = Visitor{
            [](bool value) -> AstLiteralExpr::Value {
                return !value;
            },
            [](auto /*value*/) -> AstLiteralExpr::Value {
                llvm_unreachable("Non supported type");
            }
        };
        return std::visit(visitor, ast.value);
    }
    default:
        llvm_unreachable("Unsupported unary operation");
    }
}


AstExpr* ConstantFoldingPass::visitIfExpr(AstIfExpr& ast) {
    if (auto* expr = llvm::dyn_cast<AstLiteralExpr>(ast.expr)) {
        if (std::get<bool>(expr->value)) {
            return ast.trueExpr;
        }
        return ast.falseExpr;
    }

    if (auto* repl = optimizeIifToCast(ast)) {
        return repl;
    }

    return nullptr;
}

AstExpr* ConstantFoldingPass::optimizeIifToCast(AstIfExpr& ast) {
    auto* lhs = llvm::dyn_cast<AstLiteralExpr>(ast.trueExpr);
    if (lhs == nullptr) {
        return nullptr;
    }
    const auto* lval = std::get_if<uint64_t>(&lhs->value);
    if (lval == nullptr) {
        return nullptr;
    }

    auto* rhs = llvm::dyn_cast<AstLiteralExpr>(ast.falseExpr);
    if (rhs == nullptr) {
        return nullptr;
    }
    const auto* rval = std::get_if<uint64_t>(&rhs->value);
    if (rval == nullptr) {
        return nullptr;
    }

    if (*lval == 1 && *rval == 0) {
        auto* cast = m_sem.getContext().create<AstCastExpr>(
            ast.range,
            ast.expr,
            nullptr,
            true);
        cast->type = ast.type;
        return cast;
    }

    if (*lval == 0 && *rval == 1) {
        auto* unary = m_sem.getContext().create<AstUnaryExpr>(
            ast.range,
            TokenKind::LogicalNot,
            ast.expr);

        auto* cast = m_sem.getContext().create<AstCastExpr>(
            ast.range,
            unary,
            nullptr,
            true);
        cast->type = ast.type;
        return cast;
    }

    return nullptr;
}

AstExpr* ConstantFoldingPass::visitBinaryExpr(AstBinaryExpr& /*ast*/) {
    // TODO
    return nullptr;
}

AstExpr* ConstantFoldingPass::visitCastExpr(const AstCastExpr& ast) {
    auto* literal = llvm::dyn_cast<AstLiteralExpr>(ast.expr);
    if (literal == nullptr) {
        return nullptr;
    }

    auto value = cast(ast.getType(), *literal);
    auto* repl = m_sem.getContext().create<AstLiteralExpr>(ast.range, value);
    repl->type = ast.type;
    return repl;
}

AstLiteralExpr::Value ConstantFoldingPass::cast(const TypeRoot* type, const AstLiteralExpr& ast) {
    // clang-format off
    if (const auto* integral = llvm::dyn_cast<TypeIntegral>(type)) {
        #define INTEGRAL(ID, STR, KIND, BITS, SIGNED, TYPE)                          \
            if (integral->getBits() == (BITS) && integral->isSigned() == (SIGNED)) { \
                return castLiteral<uint64_t, TYPE>(ast);                         \
            }
        INTEGRAL_TYPES(INTEGRAL)
        #undef INTEGRAL
    } else if (const auto* fp = llvm::dyn_cast<TypeFloatingPoint>(type)) {
        #define FLOATINGPOINT(ID, STR, KIND, BITS, TYPE)   \
            if (fp->getBits() == (BITS)) {                 \
                return castLiteral<double, TYPE>(ast); \
            }
        FLOATINGPOINT_TYPES(FLOATINGPOINT)
        #undef INTEGRAL
    } else if (type->isBoolean()) {
        return castLiteral<bool, bool>(ast);
    } else if (ast.getType()->isAnyPointer()) {
        return ast.value;
    }
    // clang-format on
    llvm_unreachable("Unsupported castLiteral");
}
