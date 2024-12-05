//
// Created by Albert Varaksin on 08/07/2020.
//
#include "SemanticAnalyzer.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Lexer.hpp"
#include "Lexer/Token.hpp"
#include "Parser/Parser.hpp"
#include "Passes/ForStmtPass.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Type.hpp"
#include "Type/TypeUdt.hpp"
using namespace lbc;

namespace {
auto resolveUDT(const TypeRoot* type) -> const TypeUDT* {
    if (const auto* udt = llvm::dyn_cast<TypeUDT>(type)) {
        return udt;
    }
    if (const auto* ptr = llvm::dyn_cast<TypePointer>(type)) {
        return llvm::dyn_cast<TypeUDT>(ptr->getBase());
    }

    return nullptr;
}
} // namespace

SemanticAnalyzer::SemanticAnalyzer(Context& context)
: ErrorLogger(context.getDiag())
, m_context { context }
, m_typePass { *this }
, m_declPass { *this }
, m_constantFolder { context } { }

auto SemanticAnalyzer::visit(AstModule& ast) -> Result<void> {
    auto flags = StateFlags { .allowUseBeforDefiniation = false, .allowRecursiveSymbolLookup = true };

    ast.symbolTable = m_context.create<SymbolTable>(nullptr);
    return with(&ast, ast.symbolTable, static_cast<AstFuncDecl*>(nullptr), flags, [&]() -> Result<void> {
        for (auto* import : ast.imports) {
            TRY(visit(*import))
        }
        return visit(*ast.stmtList);
    });
}

auto SemanticAnalyzer::visit(AstStmtList& ast) -> Result<void> {
    TRY(m_declPass.declare(ast))
    for (const auto& func : ast.funcs) {
        // cppcheck-suppress useStlAlgorithm
        TRY(visit(*func))
    }

    for (const auto& stmt : ast.stmts) {
        // cppcheck-suppress useStlAlgorithm
        TRY(visit(*stmt))
    }
    return {};
}

auto SemanticAnalyzer::visit(AstImport& ast) -> Result<void> {
    if (ast.module == nullptr) {
        return {};
    }
    TRY(visit(*ast.module))
    m_table->import(ast.module->symbolTable);
    return {};
}

auto SemanticAnalyzer::visit(AstExprList& /*ast*/) -> Result<void> {
    llvm_unreachable("Unhandled AstExprList&");
}

auto SemanticAnalyzer::visit(AstExprStmt& ast) -> Result<void> {
    return expression(ast.expr);
}

auto SemanticAnalyzer::visit(AstVarDecl& ast) -> Result<void> {
    if (ast.symbol->getType() == nullptr) {
        TRY(m_declPass.define(ast))
    }
    ast.symbol->stateFlags().declared = true;
    return {};
}

//----------------------------------------
// Functions
//----------------------------------------

/**
 * Analyze function declaration
 */
auto SemanticAnalyzer::visit(AstFuncDecl& ast) -> Result<void> {
    if (ast.symbol->getType() == nullptr) {
        TRY(m_declPass.define(ast))
    }
    return {};
}

auto SemanticAnalyzer::visit(AstFuncParamDecl& /*ast*/) -> Result<void> {
    llvm_unreachable("visit");
}

auto SemanticAnalyzer::visit(AstFuncStmt& ast) -> Result<void> {
    if (ast.decl->symbol->getType() == nullptr) {
        TRY(m_declPass.define(*ast.decl))
    }

    return with(ast.decl->symbolTable, ast.decl, [&]() {
        return visit(*ast.stmtList);
    });
}

auto SemanticAnalyzer::visit(AstReturnStmt& ast) -> Result<void> {
    const TypeRoot* retType = nullptr;
    bool canOmitExpression = false;
    if (m_function == nullptr) {
        retType = TypeIntegral::fromTokenKind(TokenKind::Integer);
        canOmitExpression = true;
    } else {
        retType = llvm::cast<TypeFunction>(m_function->symbol->getType())->getReturn();
    }
    auto isVoid = retType->isVoid();

    if (ast.expr == nullptr) {
        if (!isVoid && !canOmitExpression) {
            return makeError(Diag::functionMustReturnAValue, ast);
        }
        return {};
    }

    if (isVoid) {
        return makeError(Diag::subShouldNotReturnAValue, ast.expr);
    }

    TRY(expression(ast.expr, retType))

    // const auto* base = retType->isReference() ? retType->getBase()->getPointer(m_context) : retType;
    if (ast.expr->type->compare(retType) != TypeComparison::Equal) {
        return makeError(
            Diag::invalidFunctionReturnType,
            ast.expr,
            ast.expr->type->asString(),
            retType->asString()
        );
    }

    return {};
}

auto SemanticAnalyzer::visit(AstIfStmt& ast) -> Result<void> {
    RESTORE_ON_EXIT(m_table);
    for (auto& block : ast.blocks) {
        block->symbolTable = m_context.create<SymbolTable>(m_table);
    }

    for (size_t idx = 0; idx < ast.blocks.size(); idx++) {
        auto& block = ast.blocks[idx];

        m_table = block->symbolTable;
        TRY(m_declPass.declareAndDefine(block->decls))
        for (const auto& var : block->decls) {
            for (size_t next = idx + 1; next < ast.blocks.size(); next++) {
                ast.blocks[next]->symbolTable->insert(var->symbol);
            }
        }
        if (block->expr != nullptr) {
            TRY(expression(block->expr))
            if (!block->expr->type->isBoolean()) {
                return makeError(
                    Diag::noViableConversionToType,
                    block->expr,
                    block->expr->type->asString(),
                    TypeBoolean::get()->asString()
                );
            }
        }
        TRY(visit(*block->stmt))
    }
    return {};
}

auto SemanticAnalyzer::visit(AstForStmt& ast) -> Result<void> {
    TRY(Sem::ForStmtPass(*this).visit(ast))
    return {};
}

auto SemanticAnalyzer::visit(AstDoLoopStmt& ast) -> Result<void> {
    RESTORE_ON_EXIT(m_table);
    ast.symbolTable = m_context.create<SymbolTable>(m_table);
    m_table = ast.symbolTable;
    TRY(m_declPass.declareAndDefine(ast.decls))

    if (ast.expr != nullptr) {
        TRY(expression(ast.expr))
        if (!ast.expr->type->isBoolean()) {
            return makeError(
                Diag::noViableConversionToType,
                ast.expr,
                ast.expr->type->asString(),
                TypeBoolean::get()->asString()
            );
        }
    }

    TRY(visit(*ast.stmt))
    return {};
}

auto SemanticAnalyzer::visit(AstContinuationStmt& /* ast */) -> Result<void> {
    (void)this;
    return {};
}

//----------------------------------------
// Type (user defined)
//----------------------------------------

auto SemanticAnalyzer::visit(AstUdtDecl& ast) -> Result<void> {
    if (ast.symbol->getType() == nullptr) {
        TRY(m_declPass.define(ast))
    }
    return {};
}

//----------------------------------------
// Type alias
//----------------------------------------

auto SemanticAnalyzer::visit(AstTypeAlias& ast) -> Result<void> {
    if (ast.symbol->getType() == nullptr) {
        TRY(m_declPass.define(ast))
    }
    return {};
}

auto SemanticAnalyzer::visit(AstTypeOf& ast) -> Result<void> {
    if (const auto* loc = std::get_if<llvm::SMLoc>(&ast.typeExpr)) {
        Lexer lexer { m_context, m_module->fileId, *loc };
        Parser parser { m_context, lexer, false, m_table };

        const auto parsedExpression = getDiag().ignoringErrors([&]() -> bool {
            if (auto* type = parser.typeExpr().getValueOrNull()) {
                ast.typeExpr = type;
                return true;
            }

            lexer.reset(*loc);
            parser.reset();
            if (auto* expr = parser.expression().getValueOrNull()) {
                ast.typeExpr = expr;
                return true;
            }

            return false;
        });

        if (not parsedExpression) {
            return makeError(Diag::invalidTypeOfExpression, *loc, ast.getRange());
        }
    }

    using ResTy = Result<void>;
    const auto getType = Visitor {
        [](llvm::SMLoc&) -> ResTy {
            llvm_unreachable("unresolved typeof expression");
        },
        [&](AstTypeExpr* typeExpr) -> ResTy {
            TRY_ASSIGN(ast.type, m_typePass.visit(*typeExpr))
            return {};
        },
        [&](AstExpr* expr) -> ResTy {
            auto flags = m_flags;
            flags.allowUseBeforDefiniation = true;
            return with(flags, [&]() -> ResTy {
                TRY(visit(*expr))
                ast.type = expr->type;
                return {};
            });
        }
    };

    return std::visit(getType, ast.typeExpr);
}

//----------------------------------------
// Attributes
//----------------------------------------

auto SemanticAnalyzer::visit(AstAttributeList& /*ast*/) -> Result<void> {
    llvm_unreachable("visitAttributeList");
}

auto SemanticAnalyzer::visit(AstAttribute& /*ast*/) -> Result<void> {
    llvm_unreachable("visitAttribute");
}

//----------------------------------------
// Types
//----------------------------------------

auto SemanticAnalyzer::visit(AstTypeExpr& /*ast*/) -> Result<void> {
    llvm_unreachable("AstTypeExpr");
}

//----------------------------------------
// Expressions
//----------------------------------------

auto SemanticAnalyzer::expression(AstExpr*& ast, const TypeRoot* type) -> Result<void> {
    TRY(visit(*ast))

    // if (ast->type->isReference()) {
        // TRY(deref(ast));
    // } else {
        (void)m_constantFolder.fold(*ast);
    // }

    if (type != nullptr) {
        // if (type->isReference()) {
            // TRY(coerce(ast, type->getBase()))
            // TRY(addr(ast));
        // } else {
            TRY(coerce(ast, type))
        // }
    }

    if (ast->flags.constant && !ast->constantValue) {
        return makeError(Diag::mustBeConstantExpr, ast);
    }

    return {};
}

auto SemanticAnalyzer::visit(AstAssignExpr& ast) -> Result<void> {
    TRY(expression(ast.lhs))

    if (not ast.lhs->flags.assignable) {
        return makeError(
            Diag::targetNotAssignable,
            ast.lhs,
            ast.lhs->type->asString()
        );
    }

    ast.type = ast.lhs->type;
    return expression(ast.rhs, ast.type->removeReference());
}

auto SemanticAnalyzer::visit(AstIdentExpr& ast) -> Result<void> {
    auto* symbol = m_table->find(ast.name, m_flags.allowRecursiveSymbolLookup);
    if (symbol == nullptr) {
        return makeError(Diag::unknownIdentifier, ast, ast.name);
    }

    if (symbol->getType() == nullptr) {
        TRY(m_declPass.define(*symbol->getDecl()))
    }

    if (not isVariableAccessible(symbol)) {
        return makeError(Diag::useBeforeDefinition, ast, ast.name);
    }

    ast.symbol = symbol;
    ast.type = symbol->getType();
    ast.flags = symbol->valueFlags();

    return {};
}

auto SemanticAnalyzer::isVariableAccessible(Symbol* symbol) const -> bool {
    return symbol->stateFlags().declared
        || m_flags.allowUseBeforDefiniation
        || symbol->valueFlags().kind != ValueFlags::Kind::Variable
        || symbol->getSymbolTable()->getFunction() != m_function;
}

auto SemanticAnalyzer::visit(AstCallExpr& ast) -> Result<void> {
    TRY(expression(ast.callable))

    const auto* type = llvm::dyn_cast<TypeFunction>(ast.callable->type);
    if (type == nullptr) {
        return makeError(Diag::targetNotCallable, ast.callable, ast.callable->type->asString());
    }

    const auto& paramTypes = type->getParams();
    auto& args = ast.args->exprs;

    if (type->isVariadic()) {
        if (paramTypes.size() > args.size()) {
            return makeError(Diag::noMatchingSubOrFunction, ast);
        }
    } else if (paramTypes.size() != args.size()) {
        return makeError(Diag::noMatchingSubOrFunction, ast);
    }

    for (size_t index = 0; index < args.size(); index++) {
        if (index < paramTypes.size()) {
            TRY(expression(args[index], paramTypes[index]))
        } else {
            TRY(expression(args[index]))
        }
    }

    ast.type = type->getReturn();

    if (ast.type->isReference()) {
        ast.flags.assignable = true;
        ast.flags.addressable = true;
    }

    return {};
}

auto SemanticAnalyzer::visit(AstLiteralExpr& ast) -> Result<void> {
    static constexpr auto visitor = Visitor {
        [](TokenValue::NullType /*value*/) {
            return TokenKind::Null;
        },
        [](TokenValue::StringType /*value*/) {
            return TokenKind::ZString;
        },
        [](const TokenValue::IntegralType value) {
            if (value > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
                return TokenKind::Long;
            }
            return TokenKind::Integer;
        },
        [](TokenValue::FloatingPointType /*value*/) {
            return TokenKind::Double;
        },
        [](bool /*value*/) {
            return TokenKind::Bool;
        }
    };
    const auto typeKind = std::visit(visitor, ast.getValue());
    ast.type = TypeRoot::fromTokenKind(typeKind);

    return {};
}

//------------------------------------------------------------------
// Unary Expressions
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstUnaryExpr& ast) -> Result<void> {
    TRY(expression(ast.expr))
    const auto* type = ast.expr->type;

    switch (ast.token.getKind()) {
    case TokenKind::LogicalNot:
        if (type->isBoolean()) {
            ast.type = type;
            break;
        }
        return makeError(Diag::cannotUseTypeAsBoolean, ast.expr, type->asString());
    case TokenKind::Negate:
        if (type->isSignedIntegral() || type->isFloatingPoint()) {
            ast.type = type;
            break;
        }
        return makeError(Diag::unaryOperatorAppledToType, ast.expr, '-', type->asString());
    default:
        llvm_unreachable("unknown unary operator");
    }

    return {};
}

//------------------------------------------------------------------
// Dereference
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstDereference& ast) -> Result<void> {
    TRY(expression(ast.expr))

    if (const auto* type = llvm::dyn_cast<TypePointer>(ast.expr->type)) {
        ast.type = type->getBase();
        ast.flags = ast.expr->flags;
    } else {
        return makeError(Diag::dereferencingNonPointerType, ast.expr, ast.expr->type->asString());
    }

    return {};
}

//------------------------------------------------------------------
// AddressOf
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstAddressOf& ast) -> Result<void> {
    TRY(expression(ast.expr))
    if (not ast.expr->flags.addressable) {
        return makeError(Diag::cannotTakeAddressOf, ast.expr, ast.expr->type->asString());
    }
    ast.type = TypePointer::get(m_context, ast.expr->type);
    ast.flags = ast.expr->flags;
    return {};
}

//------------------------------------------------------------------
// AlignOf
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstAlignOfExpr& ast) -> Result<void> {
    TRY_DECL(type, m_typePass.visit(*ast.typeExpr))
    ast.type = TypeRoot::fromTokenKind(TokenKind::SizeOf);
    ast.constantValue = TokenValue::from(type->getAlignment(m_context));
    return {};
}

//------------------------------------------------------------------
// SizeOf
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstSizeOfExpr& ast) -> Result<void> {
    TRY_DECL(type, m_typePass.visit(*ast.typeExpr))
    ast.type = TypeRoot::fromTokenKind(TokenKind::SizeOf);
    ast.constantValue = TokenValue::from(type->getSize(m_context));
    return {};
}

//------------------------------------------------------------------
// MemberExpr
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstMemberExpr& ast) -> Result<void> {
    TRY(visit(*ast.base))
    if (const auto* ref = llvm::dyn_cast<TypeReference>(ast.base->type)) {
        ast.base->type = ref->convertToPointer(m_context);
    }

    const TypeUDT* udt = resolveUDT(ast.base->type);
    if (udt == nullptr) {
        return makeError(Diag::accessingMemberOnNonUDTType, ast.token.getRange().Start, ast.getRange(), ast.base->type->asString());
    }

    auto flags = m_flags;
    flags.allowRecursiveSymbolLookup = false;
    TRY(with(&udt->getSymbolTable(), flags, [&] {
        return visit(*ast.member); // TODO: should this go through expression() ?
    }))

    ast.type = ast.member->type;
    ast.flags = ast.member->flags;

    return {};
}

//------------------------------------------------------------------
// Binary Expressions
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstBinaryExpr& ast) -> Result<void> {
    TRY(expression(ast.lhs))
    TRY(expression(ast.rhs))

    switch (Token::getOperatorType(ast.token.getKind())) {
    case OperatorType::Arithmetic:
        return arithmetic(ast);
    case OperatorType::Comparison:
        return comparison(ast);
    case OperatorType::Logical:
        return logical(ast);
    default:
        llvm_unreachable("invalid operator");
    }
}

auto SemanticAnalyzer::arithmetic(AstBinaryExpr& ast) -> Result<void> {
    const auto* left = ast.lhs->type;
    const auto* right = ast.rhs->type;

    // error: invalid operands to binary expression 'Foo' and 'BAR'
    if (!left->isNumeric() || !right->isNumeric()) {
        if (left == right && left->isZString()) {
            if (ast.token.getKind() == TokenKind::Plus) {
                ast.type = left;
                ast.flags.constant = true;
                return {};
            }
            return makeError(
                Diag::invalidBinaryExprOperands,
                ast.token.getRange().Start,
                ast.getRange(),
                ast.token.asString(),
                left->asString(),
                right->asString()
            );
        }

        return makeError(
            Diag::invalidBinaryExprOperands,
            ast.token.getRange().Start,
            ast.getRange(),
            ast.token.asString(),
            left->asString(),
            right->asString()
        );
    }

    const auto castTo = [&](AstExpr*& expr, const TypeRoot* ty) -> Result<void> {
        TRY(cast(expr, ty))
        ast.type = ty;
        return {};
    };

    switch (left->compare(right)) {
    case TypeComparison::Incompatible:
        llvm_unreachable("Unexpected incompatible types");
    case TypeComparison::Equal:
        ast.type = left;
        return {};
    case TypeComparison::Downcast:
        return castTo(ast.rhs, left);
    case TypeComparison::Upcast:
        return castTo(ast.lhs, right);
    default:
        fatalError("Unhandled type comparison result");
    }
}

auto SemanticAnalyzer::logical(AstBinaryExpr& ast) -> Result<void> {
    (void)this;
    const auto* left = ast.lhs->type;
    const auto* right = ast.rhs->type;

    if (!left->isBoolean() || !right->isBoolean()) {
        return makeError(
            Diag::invalidBinaryExprOperands,
            ast.token.getRange().Start,
            ast.getRange(),
            ast.token.asString(),
            left->asString(),
            right->asString()
        );
    }

    ast.type = left;
    return {};
}

auto SemanticAnalyzer::comparison(AstBinaryExpr& ast) -> Result<void> {
    const auto* left = ast.lhs->type;
    const auto* right = ast.rhs->type;

    if (!canPerformBinary(ast.token.getKind(), left, right)) {
        return makeError(
            Diag::invalidCompareExprOperands,
            ast.token.getRange().Start,
            ast.getRange(),
            ast.token.asString(),
            left->asString(),
            right->asString()
        );
    }

    if (left->isZString()) {
        ast.flags.constant = true;
    }

    switch (left->compare(right)) {
    case TypeComparison::Incompatible:
        return makeError(
            Diag::invalidCompareExprOperands,
            ast.token.getRange().Start,
            ast.getRange(),
            ast.token.asString(),
            left->asString(),
            right->asString()
        );
    case TypeComparison::Equal:
        ast.type = TypeBoolean::get();
        return {};
    case TypeComparison::Downcast:
        TRY(cast(ast.rhs, left))
        ast.type = TypeBoolean::get();
        return {};
    case TypeComparison::Upcast:
        TRY(cast(ast.lhs, right))
        ast.type = TypeBoolean::get();
        return {};
    default:
        llvm_unreachable("Unhandled type comparison result");
    }
}

auto SemanticAnalyzer::canPerformBinary(TokenKind op, const TypeRoot* left, const TypeRoot* right) -> bool {
    if (left->isReference()) {
        left = left->getBase();
    }

    if (right->isReference()) {
        right = right->getBase();
    }

    if (left->isBoolean() && right->isBoolean()) {
        return op == TokenKind::Equal || op == TokenKind::NotEqual;
    }

    if (left->isPointer() && right->isPointer()) {
        return op == TokenKind::Equal || op == TokenKind::NotEqual;
    }

    if (left == right && left->isZString()) {
        return op == TokenKind::Equal || op == TokenKind::NotEqual;
    }

    return left->isNumeric() && right->isNumeric();
}

//------------------------------------------------------------------
// Casting
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstCastExpr& ast) -> Result<void> {
    TRY(expression(ast.expr))
    TRY_ASSIGN(ast.type, m_typePass.visit(*ast.typeExpr))

    if (ast.expr->type->compare(ast.type) == TypeComparison::Incompatible) {
        return makeError(Diag::invalidCast, ast, ast.expr->type->asString(), ast.type->asString());
    }

    ast.flags = ast.expr->flags;
    return {};
}

auto SemanticAnalyzer::visit(AstIsExpr& ast) -> Result<void> {
    TRY_DECL(lhs, m_typePass.visit(*ast.lhs))
    TRY_DECL(rhs, m_typePass.visit(*ast.rhs))
    ast.type = TypeBoolean::get();
    ast.constantValue = (lhs->compare(rhs) == TypeComparison::Equal) && !ast.isNot;
    return {};
}

auto SemanticAnalyzer::convert(AstExpr*& ast, const TypeRoot* type) -> Result<void> {
    TRY(cast(ast, type))
    return {};
}

auto SemanticAnalyzer::coerce(AstExpr*& ast, const TypeRoot* type) -> Result<void> {
    if (ast->type == type) {
        return {};
    }

    switch (ast->type->compare(type)) {
    case TypeComparison::Incompatible:
        return makeError(Diag::invalidImplicitConversion, ast, ast->type->asString(), type->asString());
    case TypeComparison::Equal:
        return {};
    case TypeComparison::Downcast:
    case TypeComparison::Upcast:
        return cast(ast, type);
    default:
        fatalError("Unhandled type comparison result");
    }
}

auto SemanticAnalyzer::cast(AstExpr*& ast, const TypeRoot* type) -> Result<void> {
    const auto flags = ast->flags;
    auto* cast = m_context.create<AstCastExpr>(
        ast->range,
        ast,
        nullptr,
        true
    );
    cast->type = type;
    cast->flags = flags;
    ast = cast; // NOLINT
    (void)m_constantFolder.fold(*ast);
    return {};
}

/// Create dereference "*expr" ast node
auto SemanticAnalyzer::deref(AstExpr*& /* ast */) const -> Result<void> {
    // auto* deref = m_context.create<AstDereference>(ast->range, ast);
    // deref->type = ast->type->getBase();
    // deref->flags = ast->flags;
    // deref->constantValue = std::nullopt;
    // ast = deref;
    return {};
}

/// Create address "&expr" ast node
auto SemanticAnalyzer::addr(AstExpr*& /* ast */) const -> Result<void> {
    // auto* addr = m_context.create<AstAddressOf>(ast->range, ast);
    // addr->type = ast->type->getBase()->getPointer(m_context);
    // addr->flags = ast->flags;
    // addr->flags.assignable = false;
    // addr->flags.addressable = false;
    // addr->flags.constant = false;
    // ast = addr;
    return {};
}

//------------------------------------------------------------------
// IfExpr
//------------------------------------------------------------------

auto SemanticAnalyzer::visit(AstIfExpr& ast) -> Result<void> {
    TRY(expression(ast.expr))

    if (!ast.expr->type->isBoolean()) {
        return makeError(
            Diag::noViableConversionToType,
            ast.expr,
            ast.expr->type->asString(),
            TypeBoolean::get()->asString()
        );
    }

    TRY(expression(ast.trueExpr))
    TRY(expression(ast.falseExpr))

    const auto* left = ast.trueExpr->type;
    const auto* right = ast.falseExpr->type;
    if (left->compare(right) != TypeComparison::Equal) {
        return makeError(
            Diag::mismatchingIfExprBranchTypes,
            ast,
            left->asString(),
            right->asString()
        );
    }
    ast.type = left;
    return {};
}
