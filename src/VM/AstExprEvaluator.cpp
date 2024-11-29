//
// Created by Albert Varaksin on 28/11/2024.
//
#include "AstExprEvaluator.hpp"
#include "Lexer/Token.hpp"
using namespace lbc;

namespace {

/**
 * Push the value onto the stack.
 *
 * @param stack The value stack to push the value onto.
 * @param value The value to push.
 * @param type The type of the value.
 * @return Result error if the value could not be pushed onto the stack.
 */
auto push(VariableStack& stack, const Token::Value& value, const TypeRoot* type) -> Result<void> {
    // clang-format off
    switch (type->getKind()) {
    #define BASIC(ID, STR, KIND, CPP, ...)  \
    case TypeKind::ID:                      \
        stack.push(std::get<CPP>(value));   \
        return {};
        PRIMITIVE_TYPES(BASIC)
    #undef BASIC

    #define INTEGRAL(ID, STR, KIND, CPP, ...) \
    case TypeKind::ID:                    \
        stack.push(static_cast<CPP>(std::get<uint64_t>(value))); \
        return {};
        INTEGRAL_TYPES(INTEGRAL)
    #undef INTEGRAL

    #define FLOATINGP(ID, STR, KIND, CPP, ...) \
    case TypeKind::ID:                     \
        stack.push(static_cast<CPP>(std::get<double>(value))); \
        return {};
        FLOATINGPOINT_TYPES(FLOATINGP)
    #undef FLOATINGP

    default:
        if (type->getFamily() == TypeFamily::Pointer && std::holds_alternative<std::monostate>(value)) {
            stack.push(std::monostate{});
            return {};
        }
        return ResultError{};
    }
    // clang-format on
}

/**
 * Pops the value from the stack and returns as a token value
 *
 * @param stack The value to pop from.
 * @param type The type of the value.
 * @return The value popped from the stack, or std::nullopt if the value could not be popped.
 */
auto pop(VariableStack& stack, const TypeRoot* type) -> std::optional<Token::Value> {
    // clang-format off
    switch (type->getKind()) {
    #define BASIC(ID, STR, KIND, CPP, ...) \
        case TypeKind::ID:                 \
            return stack.pop<CPP>();
        PRIMITIVE_TYPES(BASIC)
    #undef BASIC

    #define INTEGRAL(ID, STR, KIND, CPP, ...) \
    case TypeKind::ID:                    \
        return static_cast<uint64_t>(stack.pop<CPP>());
        INTEGRAL_TYPES(INTEGRAL)
    #undef INTEGRAL

    #define FLOATINGP(ID, STR, KIND, CPP, ...) \
    case TypeKind::ID:                         \
        return static_cast<double>(stack.pop<CPP>());
        FLOATINGPOINT_TYPES(FLOATINGP)
    #undef FLOATINGP

    default:
        if (type->getFamily() == TypeFamily::Pointer) {
            return stack.pop<std::monostate>();
        }
        return {};
    }
    // clang-format on
}

/**
 * Peeks the value on the stack and returns as a token value
 *
 * @param stack The stack to peek value from
 * @param type The type of the value.
 * @return The value peeked from the stack, or std::nullopt
 */
auto peek(VariableStack& stack, const TypeRoot* type) -> std::optional<Token::Value> {
    // clang-format off
    switch (type->getKind()) {
        #define BASIC(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                 \
                return stack.peek<CPP>();
        PRIMITIVE_TYPES(BASIC)
    #undef BASIC

    #define INTEGRAL(ID, STR, KIND, CPP, ...) \
    case TypeKind::ID:                    \
    return static_cast<uint64_t>(stack.peek<CPP>());
        INTEGRAL_TYPES(INTEGRAL)
    #undef INTEGRAL

    #define FLOATINGP(ID, STR, KIND, CPP, ...) \
    case TypeKind::ID:                         \
    return static_cast<double>(stack.peek<CPP>());
        FLOATINGPOINT_TYPES(FLOATINGP)
    #undef FLOATINGP

    default:
        if (type->getFamily() == TypeFamily::Pointer) {
            return stack.peek<std::monostate>();
        }
        return {};
    }
    // clang-format on
}

/**
 * Perform binary operation on two operands.
 *
 * Two operands will be consumed and result will be pushed onto the stack.
 *
 * @tparam T The type to cast the operands to.
 * @tparam OP The operation to perform.
 * @param stack The value stack containing the operands.
 * @param kind The token kind representing the operation.
 * @param operation callback function that performs the operation.
 *
 * @return Error state if failed to perform the operation.
 */
template <typename T, typename OP>
auto performBinaryOperation(VariableStack& stack, const TokenKind kind, OP operation) -> Result<void> {
    const auto rhs = stack.pop<T>();
    const auto lhs = stack.pop<T>();

    if (auto result = operation(kind, lhs, rhs)) {
        const auto value = result.value();
        if (const auto cat = Token::getOperatorType(kind); cat == OperatorType::Logical || cat == OperatorType::Comparison) {
            stack.push(static_cast<bool>(value != T{}));
        } else {
            stack.push(value);
        }
        return {};
    }

    return ResultError {};
}

/**
 * Perform unary operation on one operand.
 *
 * One operand will be consumed and result will be pushed onto the stack.
 *
 * @tparam T The type to cast the operand to.
 * @tparam OP The operation to perform.
 * @param stack The value stack containing the operand.
 * @param kind The token kind representing the operation.
 * @param operation The operation to perform.
 *
 * @return Error state if failed to perform the operation.
 */
template <typename T, typename OP>
auto performUnaryOperation(VariableStack& stack, const TokenKind kind, OP operation) -> Result<void> {
    const auto rhs = stack.pop<T>();

    if (auto result = operation(kind, rhs)) {
        const auto value = result.value();
        if (const auto cat = Token::getOperatorType(kind); cat == OperatorType::Logical || cat == OperatorType::Comparison) {
            stack.push(static_cast<bool>(value != T{}));
        } else {
            stack.push(value);
        }
        return {};
    }

    return ResultError{};
}

//------------------------------------------------------------------
// Integral operations
//------------------------------------------------------------------

/**
 * Perform binary operation on integral types
 *
 * @tparam T The integral type
 * @param op The token kind representing the operation
 * @param lhs The left-hand side operand
 * @param rhs The right-hand side operand
 * @return The results of the binary operation, or std::nullopt if the operation is not supported
 */
template <std::integral T>
auto integralBinaryOperation(const TokenKind op, const T lhs, const T rhs) -> std::optional<T> {
    switch (op) {
    case TokenKind::Multiply:
        return lhs * rhs;
    case TokenKind::Divide:
        return lhs / rhs;
    case TokenKind::Modulus:
        return lhs % rhs;
    case TokenKind::Plus:
        return lhs + rhs;
    case TokenKind::Minus:
        return lhs - rhs;
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
    case TokenKind::LogicalAnd:
        return lhs && rhs;
    case TokenKind::LogicalOr:
        return lhs || rhs;
    default:
        return {};
    }
}

/**
 * Perform unary operation on integral types.
 *
 * @tparam T The integral type.
 * @param op The token kind representing the operation.
 * @param rhs The operand.
 * @return The result of the unary operation, or std::nullopt if the operation is not supported.
 */
template<typename T>
auto integralUnaryOperation(const TokenKind op, const T rhs) -> std::optional<T> {
    switch (op) {
    case TokenKind::LogicalNot:
        return !rhs;
    case TokenKind::Negate:
        return -rhs;
    default:
        return {};
    }
}

/**
 * Perform a binary operation on integral types.
 *
 * @param stack The value stack containing the operands.
 * @param type The type of the operands.
 * @param kind The token kind representing the operation.
 * @return True if the operation was successful, false otherwise.
 */
auto performIntegralBinaryOperation(VariableStack& stack, const TypeRoot* type, const TokenKind kind) -> Result<void> {
    // clang-format off
    switch (type->getKind()) {
        #define INTEGRAL(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                    \
                return performBinaryOperation<CPP>(stack, kind, integralBinaryOperation<CPP>);
        INTEGRAL_TYPES(INTEGRAL)
        #undef INTEGRAL
        default:
            llvm_unreachable("invalid type");
    }
    // clang-format on
}

/**
 * Perform a unary operation on integral types.
 *
 * @param stack The value stack containing the operand.
 * @param type The type of the operand.
 * @param kind The token kind representing the operation.
 * @return Error state if operation was not successful.
 */
auto performIntegralUnaryOperation(VariableStack& stack, const TypeRoot* type, const TokenKind kind) -> Result<void> {
    // clang-format off
    switch (type->getKind()) {
        #define INTEGRAL(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                    \
                return performUnaryOperation<CPP>(stack, kind, integralUnaryOperation<CPP>);
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
auto booleanBinaryOperation(const TokenKind op, const bool lhs, const bool rhs) -> std::optional<bool> {
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
        return {};
    }
}

/**
 * Perform a unary operation on boolean value.
 *
 * @param op The token kind representing the operation.
 * @param val The operand.
 * @return The result of the binary operation, or std::nullopt if the operation is not supported.
 */
auto booleanUnaryOperation(const TokenKind op, const bool val) -> std::optional<bool> {
    switch (op) {
    case TokenKind::LogicalNot:
        return !val;
    default:
        return {};
    }
}

//------------------------------------------------------------------
// Floating-point operations
//------------------------------------------------------------------

/**
 * Perform a binary operation on floating-point types.
 *
 * @param op The token kind representing the operation.
 * @param lhs The left-hand side operand.
 * @param rhs The right-hand side operand.
 * @return The result of the binary operation, or std::nullopt if the operation is not supported.
 */
template <std::floating_point T>
auto floatingPointBinaryOperation(const TokenKind op, const T lhs, const T rhs) -> std::optional<T> {
    switch (op) {
    case TokenKind::Multiply:
        return lhs * rhs;
    case TokenKind::Divide:
        return lhs / rhs;
    case TokenKind::Plus:
        return lhs + rhs;
    case TokenKind::Minus:
        return lhs - rhs;
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
        return {};
    }
}

/**
 * Perform a unary operation on floating-point type.
 *
 * @param op The token kind representing the operation.
 * @param val The operand.
 * @return The result of the unary operation, or std::nullopt if the operation is not supported.
 */
template <std::floating_point T>
auto floatingPointUnaryOperation(const TokenKind op, const T val) -> std::optional<T> {
    switch (op) {
    case TokenKind::Negate:
        return -val;
    default:
        return {};
    }
}

/**
 * Perform a binary operation on floating-point types.
 *
 * @param stack The value stack containing the operands.
 * @param type The type of the operands.
 * @param kind The token kind representing the operation.
 * @return Result error if the operation could not be performed.
 */
auto performFloatingPointBinaryOperation(VariableStack& stack, const TypeRoot* type, const TokenKind kind) -> Result<void> {
    // clang-format off
    switch (type->getKind()) {
        #define FLOATINGPOINT(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                         \
                return performBinaryOperation<CPP>(stack, kind, floatingPointBinaryOperation<CPP>);
        FLOATINGPOINT_TYPES(FLOATINGPOINT)
        #undef FLOATINGPOINT
        default:
            llvm_unreachable("invalid type");
    }
    // clang-format on
}

/**
 * Perform a unary operation on a floating-point type.
 *
 * @param stack The value stack containing the operand.
 * @param type The type of the operand.
 * @param kind The token kind representing the operation.
 * @return Result error if the operation could not be performed.
 */
auto performFloatingPointUnaryOperation(VariableStack& stack, const TypeRoot* type, const TokenKind kind) -> Result<void> {
    // clang-format off
    switch (type->getKind()) {
        #define FLOATINGPOINT(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                         \
                return performUnaryOperation<CPP>(stack, kind, floatingPointUnaryOperation<CPP>);
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

/**
 * Cast the value on the stack from one type to another.
 *
 * @tparam From The type to cast from.
 * @tparam To The type to cast to.
 * @param stack The value stack containing the value to cast.
 * @return Result error if the value could not be cast.
 */
template<typename From, typename To>
auto cast(VariableStack& stack) -> Result<void> {
    if constexpr (std::is_same_v<From, llvm::StringRef> || std::is_same_v<To, llvm::StringRef>) {
        return ResultError{};
    } else {
        const auto value = stack.pop<From>();
        stack.push(static_cast<To>(value));
        return {};
    }
}

/**
 * Cast the value on the stack from one type to another.
 *
 * @param stack The value stack containing the value to cast.
 * @param to The type to cast to.
 * @return Result error if the value could not be cast.
 */
template<typename From>
auto cast(VariableStack& stack, const TypeRoot* to) -> Result<void> {
    switch (to->getKind()) {
        // clang-format off
        #define TYPE(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                \
                return cast<From, CPP>(stack);
        ALL_TYPES(TYPE)
        #undef TYPE
        // clang-format on
    default:
        return ResultError{};
    }
}

/**
 * Cast the value on the stack from one type to another.
 *
 * @param stack The value stack containing the value to cast.
 * @param from The type to cast from.
 * @param to The type to cast to.
 * @return Result error if the value could not be cast.
 */
auto cast(VariableStack& stack, const TypeRoot* from, const TypeRoot* to) -> Result<void> {
    if (from == to) {
        return {};
    }

    switch (from->getKind()) {
        // clang-format off
        #define TYPE(ID, STR, KIND, CPP, ...) \
            case TypeKind::ID:                \
                return cast<CPP>(stack, to);
        ALL_TYPES(TYPE)
        #undef TYPE
        // clang-format on
    default:
        return ResultError{};
    }
}

} // namespace

AstExprEvaluator::AstExprEvaluator() = default;
AstExprEvaluator::~AstExprEvaluator() = default;

auto AstExprEvaluator::evaluate(AstExpr& ast) -> Result<void> {
    if (ast.constantValue) {
        return {};
    }

    TRY(visit(ast));

    if (const auto result = pop(m_stack, ast.type)) {
        ast.constantValue = result;
    } else {
        return ResultError {};
    }

    return {};
}

auto AstExprEvaluator::expr(AstExpr& ast) -> Result<void> {
    if (const auto value = ast.constantValue) {
        TRY(push(m_stack, value.value(), ast.type));
        return {};
    }

    TRY(visit(ast));

    if (const auto result = peek(m_stack, ast.type)) {
        ast.constantValue = result;
    } else {
        return ResultError {};
    }

    return {};
}

auto AstExprEvaluator::visit(AstAssignExpr& /*ast*/) -> Result<void> {
    return ResultError{};
}

auto AstExprEvaluator::visit(AstIdentExpr& ast) -> Result<void> {
    if (const auto& value = ast.symbol->getConstantValue()) {
        return push(m_stack, value.value(), ast.type);
    }
    return ResultError{};
}

auto AstExprEvaluator::visit(AstCallExpr& /*ast*/) -> Result<void> {
    return ResultError{};
}

auto AstExprEvaluator::visit(AstLiteralExpr& ast) -> Result<void> {
    return push(m_stack, ast.getValue(), ast.type);
}

auto AstExprEvaluator::visit(AstUnaryExpr& ast) -> Result<void> {
    TRY(expr(*ast.expr))
    const auto* type = ast.expr->type;

    switch (type->getFamily()) {
    case TypeFamily::Integral:
        return performIntegralUnaryOperation(m_stack, type, ast.token.getKind());
    case TypeFamily::FloatingPoint:
        return performFloatingPointUnaryOperation(m_stack, type, ast.token.getKind());
    case TypeFamily::Boolean:
        return performUnaryOperation<bool>(m_stack, ast.token.getKind(), booleanUnaryOperation);
    default:
        return ResultError {};
    }
}

auto AstExprEvaluator::visit(AstBinaryExpr& ast) -> Result<void> {
    TRY(expr(*ast.lhs))
    TRY(expr(*ast.rhs))
    const auto* type = ast.lhs->type;
    assert(type == ast.rhs->type && "Binary expression requires operands of the same type");

    switch (type->getFamily()) {
    case TypeFamily::Integral:
        return performIntegralBinaryOperation(m_stack, type, ast.token.getKind());
    case TypeFamily::FloatingPoint:
        return performFloatingPointBinaryOperation(m_stack, type, ast.token.getKind());
    case TypeFamily::Boolean:
        return performBinaryOperation<bool>(m_stack, ast.token.getKind(), booleanBinaryOperation);
    default:
        return ResultError {};
    }
}

auto AstExprEvaluator::visit(AstCastExpr& ast) -> Result<void> {
    TRY(expr(*ast.expr))
    return cast(m_stack, ast.expr->type, ast.type);
}

auto AstExprEvaluator::visit(AstIfExpr& ast) -> Result<void> {
    TRY(expr(*ast.expr))
    if (m_stack.pop<bool>()) {
        return expr(*ast.trueExpr);
    }
    return expr(*ast.falseExpr);
}

auto AstExprEvaluator::visit(AstDereference& /*ast*/) -> Result<void> {
    return ResultError{};
}

auto AstExprEvaluator::visit(AstAddressOf& /*ast*/) -> Result<void> {
    return ResultError{};
}

auto AstExprEvaluator::visit(AstMemberExpr& /*ast*/) -> Result<void> {
    return ResultError{};
}
