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

SemanticAnalyzer::SemanticAnalyzer(Context& context)
: ErrorLogger(context.getDiag()),
  m_context{ context },
  m_constantFolder{ *this },
  m_typePass{ *this },
  m_declPass{ *this } {
}

Result<void> SemanticAnalyzer::visit(AstModule& ast) {
    auto flags = StateFlags{ .allowUseBeforDefiniation = false, .allowRecursiveSymbolLookup = true };

    ast.symbolTable = m_context.create<SymbolTable>(nullptr);
    return with(&ast, ast.symbolTable, static_cast<AstFuncDecl*>(nullptr), flags, [&]() -> Result<void> {
        for (auto* import : ast.imports) {
            // cppcheck-suppress useStlAlgorithm
            TRY(visit(*import))
        }
        return visit(*ast.stmtList);
    });
}

Result<void> SemanticAnalyzer::visit(AstStmtList& ast) {
    TRY(m_declPass.declare(ast))
    for (auto& func : ast.funcs) {
        // cppcheck-suppress useStlAlgorithm
        TRY(visit(*func))
    }

    for (auto& stmt : ast.stmts) {
        // cppcheck-suppress useStlAlgorithm
        TRY(visit(*stmt))
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstImport& ast) {
    if (ast.module == nullptr) {
        return {};
    }
    TRY(visit(*ast.module))
    m_table->import(ast.module->symbolTable);
    return {};
}

Result<void> SemanticAnalyzer::visit(AstExprList& /*ast*/) {
    llvm_unreachable("Unhandled AstExprList&");
}

Result<void> SemanticAnalyzer::visit(AstExprStmt& ast) {
    return expression(ast.expr);
}

Result<void> SemanticAnalyzer::visit(AstVarDecl& ast) {
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
Result<void> SemanticAnalyzer::visit(AstFuncDecl& ast) {
    if (ast.symbol->getType() == nullptr) {
        TRY(m_declPass.define(ast))
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstFuncParamDecl& /*ast*/) {
    llvm_unreachable("visit");
}

Result<void> SemanticAnalyzer::visit(AstFuncStmt& ast) {
    if (ast.decl->symbol->getType() == nullptr) {
        TRY(m_declPass.define(*ast.decl))
    }

    return with(ast.decl->symbolTable, ast.decl, [&]() {
        return visit(*ast.stmtList);
    });
}

Result<void> SemanticAnalyzer::visit(AstReturnStmt& ast) {
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

    TRY(expression(ast.expr))

    if (ast.expr->type != retType) {
        return makeError(
            Diag::invalidFunctionReturnType,
            ast.expr,
            ast.expr->type->asString(),
            retType->asString()
        );
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstIfStmt& ast) {
    RESTORE_ON_EXIT(m_table);
    for (auto& block : ast.blocks) {
        block->symbolTable = m_context.create<SymbolTable>(m_table);
    }

    for (size_t idx = 0; idx < ast.blocks.size(); idx++) {
        auto& block = ast.blocks[idx];

        m_table = block->symbolTable;
        TRY(m_declPass.declareAndDefine(block->decls))
        for (auto& var : block->decls) {
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

Result<void> SemanticAnalyzer::visit(AstForStmt& ast) {
    TRY(Sem::ForStmtPass(*this).visit(ast))
    return {};
}

Result<void> SemanticAnalyzer::visit(AstDoLoopStmt& ast) {
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

Result<void> SemanticAnalyzer::visit(AstContinuationStmt& /* ast */) {
    (void)this;
    return {};
}

//----------------------------------------
// Type (user defined)
//----------------------------------------

Result<void> SemanticAnalyzer::visit(AstUdtDecl& ast) {
    if (ast.symbol->getType() == nullptr) {
        TRY(m_declPass.define(ast))
    }
    return {};
}

//----------------------------------------
// Type alias
//----------------------------------------

Result<void> SemanticAnalyzer::visit(AstTypeAlias& ast) {
    if (ast.symbol->getType() == nullptr) {
        TRY(m_declPass.define(ast))
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstTypeOf& ast) {
    if (auto* range = std::get_if<llvm::SMRange>(&ast.typeExpr)) {
        Lexer lexer{ m_context, m_module->fileId, *range };
        Parser parser{ m_context, lexer, false, m_table };

        auto parsedExpression = getDiag().ignoringErrors([&]() -> bool {
            if (auto* type = parser.typeExpr().getValueOrNull()) {
                ast.typeExpr = type;
                return true;
            }

            lexer.reset(*range);
            parser.reset();
            if (auto* expr = parser.expression().getValueOrNull()) {
                ast.typeExpr = expr;
                return true;
            }

            return false;
        });

        if (not parsedExpression) {
            return makeError(Diag::invalidTypeOfExpression, range->Start, *range);
        }

        if (not parser.getToken().is(TokenKind::EndOfStmt)) {
            return makeError(Diag::unexpectedTokenInTypeOf, parser.getToken());
        }
    }

    using ResTy = Result<void>;
    const auto getType = Visitor{
        [](llvm::SMRange&) -> ResTy {
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

Result<void> SemanticAnalyzer::visit(AstAttributeList& /*ast*/) {
    llvm_unreachable("visitAttributeList");
}

Result<void> SemanticAnalyzer::visit(AstAttribute& /*ast*/) {
    llvm_unreachable("visitAttribute");
}

//----------------------------------------
// Types
//----------------------------------------

Result<void> SemanticAnalyzer::visit(AstTypeExpr& /*ast*/) {
    llvm_unreachable("AstTypeExpr");
}

//----------------------------------------
// Expressions
//----------------------------------------

Result<void> SemanticAnalyzer::expression(AstExpr*& ast, const TypeRoot* type) {
    TRY(visit(*ast))
    m_constantFolder.fold(ast);
    if (type != nullptr) {
        TRY(coerce(ast, type))
        m_constantFolder.fold(ast);
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstAssignExpr& ast) {
    TRY(visit(*ast.lhs))
    if (not ast.lhs->flags.assignable) {
        return makeError(
            Diag::targetNotAssignable,
            ast.lhs,
            ast.lhs->type->asString()
        );
    }
    return expression(ast.rhs, ast.lhs->type);
}

Result<void> SemanticAnalyzer::visit(AstIdentExpr& ast) {
    auto* symbol = m_table->find(ast.name, m_flags.allowRecursiveSymbolLookup);
    if (symbol == nullptr) {
        // TODO: Generate better error messages when evaluating UDT lookup
        return makeError(Diag::unknownIdentifier, ast, ast.name);
    }

    if (symbol->getType() == nullptr) {
        TRY(m_declPass.define(*symbol->getDecl()))
    }

    if (not isVariableAccessible(symbol)) {
        return makeError(Diag::useBeforeDefinition, ast, ast.name);
    }

    ast.type = symbol->getType();
    ast.symbol = symbol;
    ast.flags = symbol->valueFlags();

    return {};
}

bool SemanticAnalyzer::isVariableAccessible(Symbol* symbol) const {
    return symbol->stateFlags().declared
        || m_flags.allowUseBeforDefiniation
        || symbol->valueFlags().kind != ValueFlags::Kind::variable
        || symbol->getSymbolTable()->getFunction() != m_function;
}

Result<void> SemanticAnalyzer::visit(AstCallExpr& ast) {
    TRY(visit(*ast.callable))

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
    return {};
}

Result<void> SemanticAnalyzer::visit(AstLiteralExpr& ast) {
    static constexpr auto visitor = Visitor{
        [](const std::monostate& /*value*/) {
            return TokenKind::Null;
        },
        [](llvm::StringRef /*value*/) {
            return TokenKind::ZString;
        },
        [](uint64_t value) {
            if (value > static_cast<uint64_t>(std::numeric_limits<int32_t>::max())) {
                return TokenKind::Long;
            }
            return TokenKind::Integer;
        },
        [](double /*value*/) {
            return TokenKind::Double;
        },
        [](bool /*value*/) {
            return TokenKind::Bool;
        }
    };
    auto typeKind = std::visit(visitor, ast.value);
    ast.type = TypeRoot::fromTokenKind(typeKind);

    return {};
}

//------------------------------------------------------------------
// Unary Expressions
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstUnaryExpr& ast) {
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
        if (type->isNumeric()) {
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

Result<void> SemanticAnalyzer::visit(AstDereference& ast) {
    TRY(visit(*ast.expr))

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

Result<void> SemanticAnalyzer::visit(AstAddressOf& ast) {
    TRY(visit(*ast.expr))
    ast.type = TypePointer::get(m_context, ast.expr->type);
    ast.flags = ast.expr->flags;
    return {};
}

//------------------------------------------------------------------
// MemberExpr
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstMemberExpr& ast) {
    TRY(visit(*ast.base))

    const TypeUDT* udt = nullptr;
    if (const auto* type = ast.base->type; type->isUDT()) {
        udt = static_cast<const TypeUDT*>(type);
    } else if (const auto* ptr = llvm::dyn_cast<TypePointer>(type); ptr != nullptr && ptr->getBase()->isUDT()) {
        udt = static_cast<const TypeUDT*>(ptr->getBase());
    } else {
        return makeError(Diag::accessingMemberOnNonUDTType, ast.token.getRange().Start, ast.getRange(), type->asString());
    }

    auto flags = m_flags;
    flags.allowRecursiveSymbolLookup = false;
    TRY(with(&udt->getSymbolTable(), flags, [&] {
        return visit(*ast.member);
    }))

    ast.type = ast.member->type;
    ast.flags = ast.member->flags;

    return {};
}

//------------------------------------------------------------------
// Binary Expressions
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstBinaryExpr& ast) {
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

Result<void> SemanticAnalyzer::arithmetic(AstBinaryExpr& ast) {
    const auto* left = ast.lhs->type;
    const auto* right = ast.rhs->type;

    // error: invalid operands to binary expression 'Foo' and 'BAR'
    if (!left->isNumeric() || !right->isNumeric()) {
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
        m_constantFolder.fold(expr);
        ast.type = ty;
        return {};
    };

    switch (left->compare(right)) {
    case TypeComparison::Downcast:
        return castTo(ast.rhs, left);
    case TypeComparison::Equal:
        ast.type = left;
        return {};
    case TypeComparison::Upcast:
        return castTo(ast.lhs, right);
    default:
        llvm_unreachable("unknown comparison result");
    }
}

Result<void> SemanticAnalyzer::logical(AstBinaryExpr& ast) {
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

Result<void> SemanticAnalyzer::comparison(AstBinaryExpr& ast) {
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

    const auto castTo = [&](AstExpr*& expr, const TypeRoot* ty) -> Result<void> {
        TRY(cast(expr, ty))
        m_constantFolder.fold(expr);
        ast.type = TypeBoolean::get();
        return {};
    };

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
    case TypeComparison::Downcast:
        return castTo(ast.rhs, left);
    case TypeComparison::Equal:
        ast.type = TypeBoolean::get();
        return {};
    case TypeComparison::Upcast:
        return castTo(ast.lhs, right);
    default:
        llvm_unreachable("unknown comparison result");
    }
}

bool SemanticAnalyzer::canPerformBinary(TokenKind op, const TypeRoot* left, const TypeRoot* right) const {
    (void)this;
    if (left->isBoolean() && right->isBoolean()) {
        return op == TokenKind::Equal || op == TokenKind::NotEqual;
    }

    if (left->isPointer() && right->isPointer()) {
        return op == TokenKind::Equal || op == TokenKind::NotEqual;
    }

    return left->isNumeric() && right->isNumeric();
}

//------------------------------------------------------------------
// Casting
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstCastExpr& ast) {
    TRY_ASSIGN(ast.type, m_typePass.visit(*ast.typeExpr))
    TRY(expression(ast.expr))

    if (ast.expr->type->compare(ast.type) == TypeComparison::Incompatible) {
        return makeError(Diag::invalidCast, ast, ast.expr->type->asString(), ast.type->asString());
    }

    ast.flags = ast.expr->flags;
    return {};
}

Result<void> SemanticAnalyzer::convert(AstExpr*& ast, const TypeRoot* type) {
    TRY(cast(ast, type))
    m_constantFolder.fold(ast);
    return {};
}

Result<void> SemanticAnalyzer::coerce(AstExpr*& ast, const TypeRoot* type) {
    if (ast->type == type) {
        return {};
    }

    switch (ast->type->compare(type)) {
    case TypeComparison::Incompatible:
        return makeError(Diag::invalidImplicitConversion, ast, ast->type->asString(), type->asString());
    case TypeComparison::Downcast:
    case TypeComparison::Upcast:
        return cast(ast, type);
    case TypeComparison::Equal:
        return {};
    default:
        llvm_unreachable("unknown comparison result");
    }
}

Result<void> SemanticAnalyzer::cast(AstExpr*& ast, const TypeRoot* type) {
    auto category = ast->flags;
    auto* cast = m_context.create<AstCastExpr>(
        ast->range,
        ast,
        nullptr,
        true
    );
    cast->type = type;
    cast->flags = category;
    ast = cast; // NOLINT
    return {};
}

//------------------------------------------------------------------
// IfExpr
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstIfExpr& ast) {
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
