//
// Created by Albert Varaksin on 28/11/2024.
//
#include "ConstantFolder.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Token.hpp"
#include <type_traits>
using namespace lbc;

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

namespace {

//------------------------------------------------------------------
// perform helpers
//------------------------------------------------------------------

template <typename T, typename OP>
auto performBinaryOperation(const TokenKind kind, const TokenValue& lhs, const TokenValue& rhs, OP operation) -> Result<TokenValue> {
    TokenValue res;
    res.set(operation(kind, lhs.get<T>(), rhs.get<T>()));
    return res;
}

template <typename T, typename OP>
auto performUnaryOperation(const TokenKind kind, const TokenValue& operand, OP operation) -> Result<TokenValue> {
    TokenValue res;
    res.set(operation(kind, operand.get<T>()));
    return res;
}

//------------------------------------------------------------------
// Primitive operations
//------------------------------------------------------------------

template <typename T>
auto binaryArithmetic(const TokenKind op, const T lhs, const T rhs) -> T {
    switch (op) {
    case TokenKind::Multiply:
        return lhs * rhs;
    case TokenKind::Divide:
        return lhs / rhs;
    case TokenKind::Modulus:
        if constexpr (std::is_integral_v<T>) {
            return lhs % rhs;
        } else {
            llvm_unreachable("modulus operation is only supported for integral types");
        }
    case TokenKind::Plus:
        return lhs + rhs;
    case TokenKind::Minus:
        return lhs - rhs;
    default:
        llvm_unreachable("Unsupported arithmetic operation");
    }
}

template <typename T>
auto binaryComparison(const TokenKind op, const T lhs, const T rhs) -> bool {
    switch (op) {
    case TokenKind::Equal:
        return lhs == rhs;
    case TokenKind::NotEqual:
        return lhs != rhs;
    case TokenKind::LessThan:
        return lhs < rhs;
    case TokenKind::LessOrEqual:
        return lhs <= rhs;
    case TokenKind::GreaterThan:
        return lhs > rhs;
    case TokenKind::GreaterOrEqual:
        return lhs >= rhs;
    default:
        llvm_unreachable("Unsupported comparison operation");
    }
}

template <typename T>
auto unaryArithmetic(const TokenKind op, const T operand) -> T {
    switch (op) {
    case TokenKind::Negate:
        if constexpr (std::is_signed_v<T> || std::is_floating_point_v<T>) {
            return -operand;
        } else {
            llvm_unreachable("Negating unsupported type");
        }
    default:
        llvm_unreachable("unknown operation");
    }
}

//------------------------------------------------------------------
// Integral operations
//------------------------------------------------------------------!

auto performIntegralBinaryOperation(const TypeRoot* type, const TokenKind kind, const TokenValue& lhs, const TokenValue& rhs) -> Result<TokenValue> {
    // clang-format off
    switch (type->getKind()) {
    #define INTEGRAL(ID, STR, KIND, CPP, ...)                                              \
        case TypeKind::ID:                                                                 \
            switch (Token::getOperatorType(kind)) {                                        \
            case OperatorType::Arithmetic:                                                 \
                return performBinaryOperation<CPP>(kind, lhs, rhs, binaryArithmetic<CPP>); \
            case OperatorType::Comparison:                                                 \
                return performBinaryOperation<CPP>(kind, lhs, rhs, binaryComparison<CPP>); \
            default:                                                                       \
                llvm_unreachable("invalid operation");                                     \
            }
    INTEGRAL_TYPES(INTEGRAL)
    #undef INTEGRAL
    default:
        llvm_unreachable("invalid type");
    }
    // clang-format on
}

auto performIntegralUnaryOperation(const TypeRoot* type, const TokenKind kind, const TokenValue& operand) -> Result<TokenValue> {
    // clang-format off
    switch (type->getKind()) {
        #define INTEGRAL(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                    \
                return performUnaryOperation<CPP>(kind, operand, unaryArithmetic<CPP>);
        INTEGRAL_TYPES(INTEGRAL)
        #undef INTEGRAL
        default:
            llvm_unreachable("invalid type");
    }
    // clang-format on
}

//------------------------------------------------------------------
// Floating-point operations
//------------------------------------------------------------------

auto performFloatingPointBinaryOperation(const TypeRoot* type, const TokenKind kind, const TokenValue& lhs, const TokenValue& rhs) -> Result<TokenValue> {
    // clang-format off
    switch (type->getKind()) {
    #define FLOATINGPOINT(ID, STR, KIND, CPP, ...)                                         \
        case TypeKind::ID:                                                                 \
            switch (Token::getOperatorType(kind)) {                                        \
            case OperatorType::Arithmetic:                                                 \
                return performBinaryOperation<CPP>(kind, lhs, rhs, binaryArithmetic<CPP>); \
            case OperatorType::Comparison:                                                 \
                return performBinaryOperation<CPP>(kind, lhs, rhs, binaryComparison<CPP>); \
            default:                                                                       \
                llvm_unreachable("invalid operation");                                     \
            }
    FLOATINGPOINT_TYPES(FLOATINGPOINT)
    #undef FLOATINGPOINT
    default:
        llvm_unreachable("invalid type");
    }
    // clang-format on
}

auto performFloatingPointUnaryOperation(const TypeRoot* type, const TokenKind kind, const TokenValue& operand) -> Result<TokenValue> {
    // clang-format off
    switch (type->getKind()) {
        #define FLOATINGPOINT(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                         \
                return performUnaryOperation<CPP>(kind, operand, unaryArithmetic<CPP>);
        FLOATINGPOINT_TYPES(FLOATINGPOINT)
        #undef FLOATINGPOINT
        default:
            llvm_unreachable("invalid type");
    }
    // clang-format on
}

//------------------------------------------------------------------
// Cast operations
//------------------------------------------------------------------

template <typename From, typename To>
auto cast(const TokenValue& value) -> Result<TokenValue> {
    if constexpr (std::is_same_v<From, llvm::StringRef> || std::is_same_v<To, llvm::StringRef>) {
        return ResultError {};
    } else {
        const auto from = value.get<From>();
        TokenValue res;
        res.set<To>(static_cast<To>(from));
        return res;
    }
}

template <typename From>
auto cast(const TypeRoot* to, const TokenValue& value) -> Result<TokenValue> {
    switch (to->getKind()) {
        // clang-format off
        #define TYPE(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                \
                return cast<From, CPP>(value);
        ALL_TYPES(TYPE)
        #undef TYPE
        // clang-format on
    default:
        return ResultError {};
    }
}

auto cast(const TypeRoot* from, const TypeRoot* to, const TokenValue& value) -> Result<TokenValue> {
    if (from == to) {
        return value;
    }

    switch (from->getKind()) {
        // clang-format off
        #define TYPE(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                \
                return cast<CPP>(to, value);
        ALL_TYPES(TYPE)
        #undef TYPE
        // clang-format on
    default:
        return ResultError {};
    }
}

} // namespace

auto ConstantFolder::fold(AstExpr& ast) -> Result<void> {
    if (ast.constantValue) {
        return {};
    }

    TRY_ASSIGN(ast.constantValue, visit(ast))
    return {};
}

auto ConstantFolder::expression(const AstExpr& ast) -> Result<TokenValue> {
    if (ast.constantValue) {
        return ast.constantValue.value();
    }
    return ResultError {}; // visit(ast);
}

auto ConstantFolder::visit(AstAssignExpr& /*ast*/) -> Result<TokenValue> {
    return ResultError {};
}

auto ConstantFolder::visit(AstIdentExpr& ast) -> Result<TokenValue> {
    if (const auto& value = ast.symbol->getConstantValue()) {
        return value.value();
    }
    return ResultError {};
}

auto ConstantFolder::visit(AstCallExpr& /*ast*/) -> Result<TokenValue> {
    return ResultError {};
}

auto ConstantFolder::visit(AstLiteralExpr& ast) -> Result<TokenValue> {
    return ast.constantValue.value();
}

auto ConstantFolder::visit(AstUnaryExpr& ast) -> Result<TokenValue> {
    TRY_DECL(res, expression(*ast.expr))
    const auto* type = ast.expr->type;

    switch (type->getFamily()) {
    case TypeFamily::Integral:
        return performIntegralUnaryOperation(type, ast.token.getKind(), res);
    case TypeFamily::FloatingPoint:
        return performFloatingPointUnaryOperation(type, ast.token.getKind(), res);
    case TypeFamily::Boolean:
        return booleanUnaryOperation(ast.token.getKind(), res.get<bool>());
    default:
        return ResultError {};
    }
}

auto ConstantFolder::visit(AstBinaryExpr& ast) -> Result<TokenValue> {
    TRY_DECL(lhs, expression(*ast.lhs))
    TRY_DECL(rhs, expression(*ast.rhs))
    const auto* type = ast.lhs->type;
    assert(type == ast.rhs->type && "Binary expression requires operands of the same type");

    switch (type->getFamily()) {
    case TypeFamily::Integral:
        return performIntegralBinaryOperation(type, ast.token.getKind(), lhs, rhs);
    case TypeFamily::FloatingPoint:
        return performFloatingPointBinaryOperation(type, ast.token.getKind(), lhs, rhs);
    case TypeFamily::Boolean:
        return booleanBinaryExpr(ast.token.getKind(), lhs.getBoolean(), rhs.getBoolean());
    case TypeFamily::ZString:
        return stringBinaryExpr(ast.token.getKind(), lhs.getString(), rhs.getString());
    default:
        return ResultError {};
    }
}

auto ConstantFolder::visit(AstCastExpr& ast) -> Result<TokenValue> {
    TRY_DECL(res, expression(*ast.expr))
    return cast(ast.expr->type, ast.type, res);
}

auto ConstantFolder::visit(AstIsExpr& ast) -> Result<TokenValue> {
    if (const auto constant = ast.constantValue) {
        return { constant.value() };
    }
    return ResultError {};
}

auto ConstantFolder::visit(AstIfExpr& ast) -> Result<TokenValue> {
    TRY_DECL(res, expression(*ast.expr))

    if (res.getBoolean()) {
        return expression(*ast.trueExpr);
    }

    return expression(*ast.falseExpr);
}

auto ConstantFolder::visit(AstDereference& /*ast*/) -> Result<TokenValue> {
    return ResultError {};
}

auto ConstantFolder::visit(AstAddressOf& /*ast*/) -> Result<TokenValue> {
    return ResultError {};
}

auto ConstantFolder::visit(AstAlignOfExpr& ast) -> Result<TokenValue> {
    if (const auto constant = ast.constantValue) {
        return { constant.value() };
    }
    return ResultError {};
}

auto ConstantFolder::visit(AstSizeOfExpr& ast) -> Result<TokenValue> {
    if (const auto constant = ast.constantValue) {
        return { constant.value() };
    }
    return ResultError {};
}

auto ConstantFolder::visit(AstMemberExpr& /*ast*/) -> Result<TokenValue> {
    return ResultError {};
}

//------------------------------------------------------------------
// operations
//------------------------------------------------------------------

auto ConstantFolder::stringBinaryExpr(const TokenKind op, const TokenValue::StringType& lhs, const TokenValue::StringType& rhs) const -> TokenValue {
    switch (op) {
    case TokenKind::Plus:
        return m_context.retainCopy(lhs.str() + rhs.str());
    case TokenKind::Equal:
        return lhs == rhs;
    case TokenKind::NotEqual:
        return lhs != rhs;
    default:
        llvm_unreachable("Unsupported string operation");
    }
}

auto ConstantFolder::booleanBinaryExpr(const TokenKind op, const bool lhs, const bool rhs) -> TokenValue {
    switch (op) {
    case TokenKind::Equal:
        return lhs == rhs;
    case TokenKind::NotEqual:
        return lhs != rhs;
    case TokenKind::LogicalAnd:
        return lhs && rhs;
    case TokenKind::LogicalOr:
        return lhs || rhs;
    default:
        llvm_unreachable("unknown operation");
    }
}

auto ConstantFolder::booleanUnaryOperation(const TokenKind op, const bool operand) -> TokenValue {
    switch (op) {
    case TokenKind::LogicalNot:
        return !operand;
    default:
        llvm_unreachable("unknown operation");
    }
}
