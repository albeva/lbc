//
// Created by Albert Varaksin on 28/11/2024.
//
#include "AstExprEvaluator.hpp"
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
// Boolean operations
//------------------------------------------------------------------

/**
 * Perform a binary operation on boolean types.
 *
 * @param op The token kind representing the operation.
 * @param lhs The left-hand side operand.
 * @param rhs The right-hand side operand.
 * @return The result of the binary operation, or std::nullopt if the operation is not supported.
 */
auto booleanBinaryOperation(const TokenKind op, const bool lhs, const bool rhs) -> bool {
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

/**
 * Perform a unary operation on boolean value.
 *
 * @param op The token kind representing the operation.
 * @param val The operand.
 * @return The result of the binary operation, or std::nullopt if the operation is not supported.
 */
auto booleanUnaryOperation(const TokenKind op, const bool val) -> bool {
    switch (op) {
    case TokenKind::LogicalNot:
        return !val;
    default:
        llvm_unreachable("unknown operation");
    }
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

auto convert(const TypeRoot* type, const Token::Value& value) -> Result<VM::Value> {
    // clang-format off
    switch (type->getKind()) {
        #define BASIC(ID, STR, KIND, CPP, ...)  \
        case TypeKind::ID:                      \
            return { std::get<CPP>(value) };
        PRIMITIVE_TYPES(BASIC)
        #undef BASIC

        #define INTEGRAL(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                    \
                return { static_cast<CPP>(std::get<uint64_t>(value)) };
        INTEGRAL_TYPES(INTEGRAL)
        #undef INTEGRAL

        #define FLOATINGP(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                     \
                return { static_cast<CPP>(std::get<double>(value)) };
        FLOATINGPOINT_TYPES(FLOATINGP)
        #undef FLOATINGP
    default:
        if (type->isPointer() && std::holds_alternative<std::monostate>(value)) {
            return { std::monostate{} };
        }
        return ResultError{};
    }
    // clang-format on
}

auto convert(const TypeRoot* type, const VM::Value& value) -> Result<Token::Value> {
    // clang-format off
    switch (type->getKind()) {
        #define BASIC(ID, STR, KIND, CPP, ...)  \
            case TypeKind::ID:                  \
                return { std::get<CPP>(value) };
        PRIMITIVE_TYPES(BASIC)
        #undef BASIC

        #define INTEGRAL(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                    \
                return { static_cast<uint64_t>(std::get<CPP>(value)) };
        INTEGRAL_TYPES(INTEGRAL)
        #undef INTEGRAL

        #define FLOATINGP(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                     \
                return { static_cast<double>(std::get<CPP>(value)) };
        FLOATINGPOINT_TYPES(FLOATINGP)
        #undef FLOATINGP
    default:
        if (type->isPointer() && std::holds_alternative<std::monostate>(value)) {
            return { std::monostate{} };
        }
        return ResultError{};
    }
    // clang-format on
}

} // namespace

AstExprEvaluator::AstExprEvaluator() = default;
AstExprEvaluator::~AstExprEvaluator() = default;

auto AstExprEvaluator::evaluate(AstExpr& ast) -> Result<void> {
    if (ast.constantValue) {
        return {};
    }

    TRY_DECL(res, visit(ast))
    TRY_ASSIGN(ast.constantValue, convert(ast.type, res));

    return {};
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
    TRY_DECL(res, visit(*ast.expr))
    const auto* type = ast.expr->type;

    switch (type->getFamily()) {
    case TypeFamily::Integral:
        return performIntegralUnaryOperation(type, ast.token.getKind(), res);
    case TypeFamily::FloatingPoint:
        return performFloatingPointUnaryOperation(type, ast.token.getKind(), res);
    case TypeFamily::Boolean:
        return performUnaryOperation<bool>(ast.token.getKind(), res, booleanUnaryOperation);
    default:
        return ResultError {};
    }
}

auto AstExprEvaluator::visit(AstBinaryExpr& ast) -> Result<VM::Value> {
    TRY_DECL(lhs, visit(*ast.lhs))
    TRY_DECL(rhs, visit(*ast.rhs))
    const auto* type = ast.lhs->type;
    assert(type == ast.rhs->type && "Binary expression requires operands of the same type");

    switch (type->getFamily()) {
    case TypeFamily::Integral:
        return performIntegralBinaryOperation(type, ast.token.getKind(), lhs, rhs);
    case TypeFamily::FloatingPoint:
        return performFloatingPointBinaryOperation(type, ast.token.getKind(), lhs, rhs);
    case TypeFamily::Boolean:
        return performBinaryOperation<bool>(ast.token.getKind(), lhs, rhs, booleanBinaryOperation);
    default:
        return ResultError {};
    }
}

auto AstExprEvaluator::visit(AstCastExpr& ast) -> Result<VM::Value> {
    TRY_DECL(res, visit(*ast.expr))
    return cast(ast.expr->type, ast.type, res);
}

auto AstExprEvaluator::visit(AstIfExpr& /* ast */) -> Result<VM::Value> {
    return ResultError {};
    // TRY(expr(*ast.expr))
    // if (m_stack.pop<bool>()) {
    //     return expr(*ast.trueExpr);
    // }
    // return expr(*ast.falseExpr);
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
