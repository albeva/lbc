//
// Created by Albert Varaksin on 28/11/2024.
//
#include "AstExprEvaluator.hpp"

#include "Driver/Context.hpp"
#include "Lexer/Token.hpp"
using namespace lbc;

namespace {

//------------------------------------------------------------------
// perform helpers
//------------------------------------------------------------------

template <typename T, typename OP>
auto performBinaryOperation(const TokenKind kind, const VM::Value& lhs, const VM::Value& rhs, OP operation) -> Result<VM::Value> {
    return { operation(kind, std::get<T>(lhs), std::get<T>(rhs)) };
}

template <typename T, typename OP>
auto performUnaryOperation(const TokenKind kind, const VM::Value& operand, OP operation) -> Result<VM::Value> {
    return { operation(kind, std::get<T>(operand)) };
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
        return -operand;
    default:
        llvm_unreachable("unknown operation");
    }
}

//------------------------------------------------------------------
// Integral operations
//------------------------------------------------------------------!

auto performIntegralBinaryOperation(const TypeRoot* type, const TokenKind kind, const VM::Value& lhs, const VM::Value& rhs) -> Result<VM::Value> {
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

auto performIntegralUnaryOperation(const TypeRoot* type, const TokenKind kind, const VM::Value& operand) -> Result<VM::Value> {
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

auto performFloatingPointBinaryOperation(const TypeRoot* type, const TokenKind kind, const VM::Value& lhs, const VM::Value& rhs) -> Result<VM::Value> {
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

auto performFloatingPointUnaryOperation(const TypeRoot* type, const TokenKind kind, const VM::Value& operand) -> Result<VM::Value> {
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
auto cast(const VM::Value& value) -> Result<VM::Value> {
    if constexpr (std::is_same_v<From, llvm::StringRef> || std::is_same_v<To, llvm::StringRef>) {
        return ResultError {};
    } else {
        const auto from = std::get<From>(value);
        return { static_cast<To>(from) };
    }
}

template <typename From>
auto cast(const TypeRoot* to, const VM::Value& value) -> Result<VM::Value> {
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

auto cast(const TypeRoot* from, const TypeRoot* to, const VM::Value& value) -> Result<VM::Value> {
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

//------------------------------------------------------------------
// Conversion between Token::Value and VM::Value
//------------------------------------------------------------------

auto convert(const TypeRoot* type, const TokenValue& value) -> Result<VM::Value> {
    // clang-format off
    switch (type->getKind()) {
        #define BASIC(ID, STR, KIND, CPP, ...)  \
        case TypeKind::ID:                      \
            return { std::get<CPP>(value) };
        PRIMITIVE_TYPES(BASIC)
        #undef BASIC

        #define INTEGRAL(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                    \
                return { static_cast<CPP>(value.getIntegral()) };
        INTEGRAL_TYPES(INTEGRAL)
        #undef INTEGRAL

        #define FLOATINGP(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                     \
                return { static_cast<CPP>(value.getFloatingPoint()) };
        FLOATINGPOINT_TYPES(FLOATINGP)
        #undef FLOATINGP
    default:
        if (type->isPointer() && value.isNull()) {
            return { TokenValue::NullType{} };
        }
        return ResultError{};
    }
    // clang-format on
}

auto convert(const TypeRoot* type, const VM::Value& value) -> Result<TokenValue> {
    // clang-format off
    switch (type->getKind()) {
        #define BASIC(ID, STR, KIND, CPP, ...)  \
            case TypeKind::ID:                  \
                return { std::get<CPP>(value) };
        PRIMITIVE_TYPES(BASIC)
        #undef BASIC

        #define INTEGRAL(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                    \
                return { static_cast<TokenValue::IntegralType>(std::get<CPP>(value)) };
        INTEGRAL_TYPES(INTEGRAL)
        #undef INTEGRAL

        #define FLOATINGP(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                     \
                return { static_cast<TokenValue::FloatingPointType>(std::get<CPP>(value)) };
        FLOATINGPOINT_TYPES(FLOATINGP)
        #undef FLOATINGP
    default:
        if (type->isPointer() && std::holds_alternative<TokenValue::NullType>(value)) {
            return { TokenValue::NullType{} };
        }
        return ResultError{};
    }
    // clang-format on
}

} // namespace

auto AstExprEvaluator::evaluate(AstExpr& ast) -> Result<void> {
    if (ast.constantValue) {
        return {};
    }

    TRY_DECL(res, visit(ast))
    TRY_ASSIGN(ast.constantValue, convert(ast.type, res));
    return {};
}

auto AstExprEvaluator::expression(const AstExpr& ast) -> Result<VM::Value> {
    if (ast.constantValue) {
        return convert(ast.type, ast.constantValue.value());
    }
    return ResultError {}; // visit(ast);
}

auto AstExprEvaluator::visit(AstAssignExpr& /*ast*/) -> Result<VM::Value> {
    return ResultError {};
}

auto AstExprEvaluator::visit(AstIdentExpr& ast) -> Result<VM::Value> {
    if (const auto& value = ast.symbol->getConstantValue()) {
        return convert(ast.type, value.value());
    }
    return ResultError {};
}

auto AstExprEvaluator::visit(AstCallExpr& /*ast*/) -> Result<VM::Value> {
    return ResultError {};
}

auto AstExprEvaluator::visit(AstLiteralExpr& ast) -> Result<VM::Value> {
    return convert(ast.type, ast.getValue());
}

auto AstExprEvaluator::visit(AstUnaryExpr& ast) -> Result<VM::Value> {
    TRY_DECL(res, expression(*ast.expr))
    const auto* type = ast.expr->type;

    switch (type->getFamily()) {
    case TypeFamily::Integral:
        return performIntegralUnaryOperation(type, ast.token.getKind(), res);
    case TypeFamily::FloatingPoint:
        return performFloatingPointUnaryOperation(type, ast.token.getKind(), res);
    case TypeFamily::Boolean:
        return booleanUnaryOperation(ast.token.getKind(), std::get<bool>(res));
    default:
        return ResultError {};
    }
}

auto AstExprEvaluator::visit(AstBinaryExpr& ast) -> Result<VM::Value> {
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
        return booleanBinaryExpr(ast.token.getKind(), std::get<bool>(lhs), std::get<bool>(rhs));
    case TypeFamily::ZString:
        return stringBinaryExpr(ast.token.getKind(), std::get<TokenValue::StringType>(lhs), std::get<TokenValue::StringType>(rhs));
    default:
        return ResultError {};
    }
}

auto AstExprEvaluator::visit(AstCastExpr& ast) -> Result<VM::Value> {
    TRY_DECL(res, expression(*ast.expr))
    return cast(ast.expr->type, ast.type, res);
}

auto AstExprEvaluator::visit(AstIsExpr& ast) -> Result<VM::Value> {
    if (const auto constant = ast.constantValue) {
        return { constant.value().getBoolean() };
    }
    return ResultError {};
}

auto AstExprEvaluator::visit(AstIfExpr& ast) -> Result<VM::Value> {
    TRY_DECL(res, expression(*ast.expr))

    if (std::get<bool>(res)) {
        return expression(*ast.trueExpr);
    }

    return expression(*ast.falseExpr);
}

auto AstExprEvaluator::visit(AstDereference& /*ast*/) -> Result<VM::Value> {
    return ResultError {};
}

auto AstExprEvaluator::visit(AstAddressOf& /*ast*/) -> Result<VM::Value> {
    return ResultError {};
}

auto AstExprEvaluator::visit(AstMemberExpr& /*ast*/) -> Result<VM::Value> {
    return ResultError {};
}

//------------------------------------------------------------------
// operations
//------------------------------------------------------------------

auto AstExprEvaluator::stringBinaryExpr(const TokenKind op, const TokenValue::StringType& lhs, const TokenValue::StringType& rhs) const -> VM::Value {
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

auto AstExprEvaluator::booleanBinaryExpr(const TokenKind op, const bool lhs, const bool rhs) -> VM::Value {
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

auto AstExprEvaluator::booleanUnaryOperation(const TokenKind op, const bool operand) -> VM::Value {
    switch (op) {
    case TokenKind::LogicalNot:
        return !operand;
    default:
        llvm_unreachable("unknown operation");
    }
}
