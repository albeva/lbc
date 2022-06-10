//
// Created by Albert Varaksin on 08/07/2020.
//
#include "SemanticAnalyzer.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Token.hpp"
#include "Lexer/TokenProvider.hpp"
#include "Parser/Parser.hpp"
#include "Passes/DeclPass.hpp"
#include "Passes/ForStmtPass.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Type.hpp"
#include "Type/TypeUdt.hpp"

using namespace lbc;

SemanticAnalyzer::SemanticAnalyzer(Context& context)
: m_context{ context },
  m_constantFolder{ *this },
  m_typePass{ *this },
  m_declPass{ *this } {}

Result<void> SemanticAnalyzer::visit(AstModule& ast) {
    ast.symbolTable = m_context.create<SymbolTable>(nullptr);
    return with(&ast, ast.symbolTable, static_cast<AstFuncDecl*>(nullptr), StateFlags{}, [&]() -> Result<void> {
        for (auto* import : ast.imports) {
            TRY(visit(*import));
        }
        return visit(*ast.stmtList);
    });
}

Result<void> SemanticAnalyzer::visit(AstStmtList& ast) {
    m_declPass.declare(ast);
    for (auto& func : ast.funcs) {
        TRY(visit(*func));
    }
    for (auto& stmt : ast.stmts) {
        TRY(visit(*stmt));
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstImport& ast) {
    if (ast.module == nullptr) {
        return {};
    }
    TRY(visit(*ast.module));
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
        m_declPass.define(ast.symbol);
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
        m_declPass.define(ast.symbol);
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstFuncParamDecl& /*ast*/) {
    llvm_unreachable("visit");
}

Result<void> SemanticAnalyzer::visit(AstFuncStmt& ast) {
    if (ast.decl->symbol->getType() == nullptr) {
        m_declPass.define(ast.decl->symbol);
    }

    return with(ast.decl->symbolTable, ast.decl, [&]() {
        return visit(*ast.stmtList);
    });
}

Result<void> SemanticAnalyzer::visit(AstReturnStmt& ast) {
    const TypeRoot* retType = nullptr;
    bool canOmitExpression = false;
    if (m_function == nullptr) {
        if (!m_module->hasImplicitMain) {
            fatalError("Return statement outside SUB / FUNCTION or a main module");
        }
        retType = TypeIntegral::fromTokenKind(TokenKind::Integer);
        canOmitExpression = true;
    } else {
        retType = llvm::cast<TypeFunction>(m_function->symbol->getType())->getReturn();
    }
    auto isVoid = retType->isVoid();
    if (ast.expr == nullptr) {
        if (!isVoid && !canOmitExpression) {
            fatalError("Expected expression");
        }
        return {};
    }

    if (isVoid) {
        fatalError("Unexpected expression for SUB");
    }

    TRY(expression(ast.expr));

    if (ast.expr->getType() != retType) {
        fatalError(
            "Return expression type mismatch."_t
            + " Expected (" + retType->asString() + ")"
            + " got (" + ast.expr->getType()->asString() + ")");
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
        m_declPass.declareAndDefine(block->decls);
        for (auto& var : block->decls) {
            for (size_t next = idx + 1; next < ast.blocks.size(); next++) {
                ast.blocks[next]->symbolTable->addReference(var->symbol);
            }
        }
        if (block->expr != nullptr) {
            TRY(expression(block->expr));
            if (!block->expr->getType()->isBoolean()) {
                fatalError("type '"_t
                    + block->expr->getType()->asString()
                    + "' cannot be used as boolean");
            }
        }
        TRY(visit(*block->stmt));
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstForStmt& ast) {
    Sem::ForStmtPass(*this).visit(ast);
    return {};
}

Result<void> SemanticAnalyzer::visit(AstDoLoopStmt& ast) {
    RESTORE_ON_EXIT(m_table);
    ast.symbolTable = m_context.create<SymbolTable>(m_table);
    m_table = ast.symbolTable;
    m_declPass.declareAndDefine(ast.decls);

    if (ast.expr != nullptr) {
        TRY(expression(ast.expr));
        if (!ast.expr->getType()->isBoolean()) {
            fatalError("type '"_t
                + ast.expr->getType()->asString()
                + "' cannot be used as boolean");
        }
    }

    m_controlStack.push(ControlFlowStatement::Do);
    TRY(visit(*ast.stmt));
    m_controlStack.pop();
    return {};
}

Result<void> SemanticAnalyzer::visit(AstContinuationStmt& ast) {
    if (m_controlStack.find(ast.destination) == m_controlStack.cend()) {
        fatalError("control statement not found");
    }
    return {};
}

//----------------------------------------
// Type (user defined)
//----------------------------------------

Result<void> SemanticAnalyzer::visit(AstUdtDecl& ast) {
    if (ast.symbol->getType() == nullptr) {
        m_declPass.define(ast.symbol);
    }
    return {};
}

//----------------------------------------
// Type alias
//----------------------------------------

Result<void> SemanticAnalyzer::visit(AstTypeAlias& ast) {
    if (ast.symbol->getType() == nullptr) {
        m_declPass.define(ast.symbol);
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstTypeOf& ast) {
    if (auto* tokens = std::get_if<std::vector<Token>>(&ast.typeExpr)) {
        // let provider take ownership of tokens
        TokenProvider provider{ m_module->fileId, std::move(*tokens) };
        Parser parser{ m_context, provider, /* isMain */ false, getSymbolTable() };

        if (auto* type = parser.typeExpr().getPointer()) {
            ast.typeExpr = type;
        } else if (!ast.allowExpr) {
            fatalError("Expression not allowed in typeof");
        } else {
            provider.reset();
            parser.reset();
            if (auto* expr = parser.expression().getPointer()) {
                ast.typeExpr = expr;
            } else {
                fatalError("Failed to parse expression");
            }
        }

        if (not parser.getToken().is(TokenKind::EndOfStmt)) {
            fatalError("Unexpected token: "_t + parser.getToken().description());
        }
    }

    const auto getType = Visitor{
        [](std::vector<Token>&) -> const TypeRoot* {
            fatalError("Unresolved typeof()");
        },
        [&](AstTypeExpr* typeExpr) -> const TypeRoot* {
            return m_typePass.visit(*typeExpr);
        },
        [&](AstExpr* expr) -> const TypeRoot* {
            auto flags = m_flags;
            flags.allowUseBeforDefiniation = true;
            return with(flags, [&]() {
                TRY_FATAL(visit(*expr));
                return expr->type;
            });
        }
    };

    ast.type = std::visit(getType, ast.typeExpr);
    return {};
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
    TRY(visit(*ast));
    m_constantFolder.fold(ast);
    if (type != nullptr) {
        TRY(coerce(ast, type));
        m_constantFolder.fold(ast);
    }
    return {};
}

Result<void> SemanticAnalyzer::visit(AstAssignExpr& ast) {
    TRY(visit(*ast.lhs));
    if (!ast.lhs->flags.assignable) {
        fatalError("Cannot assign");
    }
    return expression(ast.rhs, ast.lhs->getType());
}

Result<void> SemanticAnalyzer::visit(AstIdentExpr& ast) {
    auto* symbol = m_table->find(ast.name);
    if (symbol == nullptr) {
        fatalError("Unknown identifier "_t + ast.name);
    }

    if (symbol->getType() == nullptr) {
        m_declPass.define(symbol);
    }

    const auto* type = symbol->getType();
    if (type == nullptr) [[unlikely]] {
        fatalError("Identifier "_t + ast.name + " has unresolved type");
    }

    if (not isVariableAccessible(symbol)) {
        fatalError("Use of '"_t + symbol->name() + "' before definition");
    }

    ast.type = type;
    ast.symbol = symbol;
    ast.flags = symbol->valueFlags();

    return {};
}

bool SemanticAnalyzer::isVariableAccessible(Symbol* symbol) const noexcept {
    return symbol->stateFlags().declared
        || m_flags.allowUseBeforDefiniation
        || symbol->valueFlags().kind != ValueFlags::Kind::variable
        || symbol->getSymbolTable()->getFunction() != m_function;
}

Result<void> SemanticAnalyzer::visit(AstCallExpr& ast) {
    TRY(visit(*ast.callable));

    const auto* type = llvm::dyn_cast<TypeFunction>(ast.callable->getType());
    if (type == nullptr) {
        fatalError("Trying to call a non callable");
    }

    const auto& paramTypes = type->getParams();
    auto& args = ast.args->exprs;

    if (type->isVariadic()) {
        if (paramTypes.size() > args.size()) {
            fatalError("Argument count mismatch");
        }
    } else if (paramTypes.size() != args.size()) {
        fatalError("Argument count mismatch");
    }

    for (size_t index = 0; index < args.size(); index++) {
        if (index < paramTypes.size()) {
            TRY(expression(args[index], paramTypes[index]));
        } else {
            TRY(expression(args[index]));
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
    TRY(expression(ast.expr));
    const auto* type = ast.expr->type;

    switch (ast.tokenKind) {
    case TokenKind::LogicalNot:
        if (type->isBoolean()) {
            ast.type = type;
            break;
        }
        fatalError("Applying unary NOT to non bool type");
    case TokenKind::Negate:
        if (type->isNumeric()) {
            ast.type = type;
            break;
        }
        fatalError("Applying unary negate to non-numeric type");
    default:
        llvm_unreachable("unknown unary operator");
    }

    return {};
}

//------------------------------------------------------------------
// Dereference
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstDereference& ast) {
    // TODO dereference needs to return a reference to value, NOT value itself

    TRY(visit(*ast.expr));
    if (const auto* type = llvm::dyn_cast<TypePointer>(ast.expr->getType())) {
        ast.type = type->getBase();
    } else {
        fatalError("dereferencing a non pointer");
    }

    ast.flags = ast.expr->flags;
    if (!ast.getType()->isPointer()) {
        ast.flags.dereferencable = false;
    }

    return {};
}

//------------------------------------------------------------------
// AddressOf
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstAddressOf& ast) {
    TRY(visit(*ast.expr));
    if (!ast.expr->flags.addressable) {
        fatalError("Cannot take address");
    }
    ast.type = TypePointer::get(m_context, ast.expr->getType());
    ast.flags = ast.expr->flags;
    ast.flags.dereferencable = true;
    return {};
}

//------------------------------------------------------------------
// Member Access
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstMemberAccess& ast) {
    RESTORE_ON_EXIT(m_table);

    for (size_t i = 0; i < ast.exprs.size(); i++) {
        auto* expr = ast.exprs[i];
        TRY(visit(*expr));
        const auto* type = expr->type;

        if (i == (ast.exprs.size() - 1)) {
            ast.type = type;
            ast.flags = expr->flags;
        } else {
            const TypeUDT* udt = nullptr;
            if (type->isUDT()) {
                udt = static_cast<const TypeUDT*>(type);
            } else if (const auto* ptr = llvm::dyn_cast<TypePointer>(type)) {
                if (ptr->getBase()->isUDT()) {
                    udt = static_cast<const TypeUDT*>(ptr->getBase());
                }
            }

            if (udt == nullptr) {
                fatalError("Accessing member of non UDT type");
            }

            m_table = &udt->getSymbolTable();
        }
    }

    return {};
}

//------------------------------------------------------------------
// Binary Expressions
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstBinaryExpr& ast) {
    TRY(expression(ast.lhs));
    TRY(expression(ast.rhs));

    switch (Token::getOperatorType(ast.tokenKind)) {
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

    if (!left->isNumeric() || !right->isNumeric()) {
        fatalError("Applying artithmetic operation to non numeric type");
    }

    const auto convert = [&](AstExpr*& expr, const TypeRoot* ty) -> Result<void> {
        TRY(cast(expr, ty));
        m_constantFolder.fold(expr);
        ast.type = ty;
        return {};
    };

    switch (left->compare(right)) {
    case TypeComparison::Incompatible:
        fatalError("Operator on incompatible types");
    case TypeComparison::Downcast:
        return convert(ast.rhs, left);
    case TypeComparison::Equal:
        ast.type = left;
        return {};
    case TypeComparison::Upcast:
        return convert(ast.lhs, right);
    }
}

Result<void> SemanticAnalyzer::logical(AstBinaryExpr& ast) {
    const auto* left = ast.lhs->type;
    const auto* right = ast.rhs->type;

    if (!left->isBoolean() || !right->isBoolean()) {
        fatalError("Applying logical operator to non boolean type");
    }
    ast.type = left;
    return {};
}

Result<void> SemanticAnalyzer::comparison(AstBinaryExpr& ast) {
    const auto* left = ast.lhs->type;
    const auto* right = ast.rhs->type;

    if (!canPerformBinary(ast.tokenKind, left, right)) {
        fatalError("Cannot apply operationg to types");
    }

    const auto convert = [&](AstExpr*& expr, const TypeRoot* ty) -> Result<void> {
        TRY(cast(expr, ty));
        m_constantFolder.fold(expr);
        ast.type = TypeBoolean::get();
        return {};
    };

    switch (left->compare(right)) {
    case TypeComparison::Incompatible:
        fatalError("Operator on incompatible types");
    case TypeComparison::Downcast:
        return convert(ast.rhs, left);
    case TypeComparison::Equal:
        ast.type = TypeBoolean::get();
        return {};
    case TypeComparison::Upcast:
        return convert(ast.lhs, right);
    }
}

bool SemanticAnalyzer::canPerformBinary(TokenKind op, const TypeRoot* left, const TypeRoot* right) const noexcept {
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
    ast.type = m_typePass.visit(*ast.typeExpr);
    TRY(expression(ast.expr));

    if (ast.expr->getType()->compare(ast.getType()) == TypeComparison::Incompatible) {
        fatalError("Incompatible cast");
    }

    ast.flags = ast.expr->flags;
    return {};
}

Result<void> SemanticAnalyzer::convert(AstExpr*& ast, const TypeRoot* type) {
    TRY(cast(ast, type));
    m_constantFolder.fold(ast);
    return {};
}

Result<void> SemanticAnalyzer::coerce(AstExpr*& ast, const TypeRoot* type) {
    if (ast->getType() == type) {
        return {};
    }
    const auto* src = type;
    const auto* dst = ast->getType();

    switch (src->compare(dst)) {
    case TypeComparison::Incompatible:
        fatalError(
            "Type mismatch."_t
            + " Expected '" + type->asString() + "'"
            + " got '" + ast->getType()->asString() + "'");
    case TypeComparison::Downcast:
    case TypeComparison::Upcast:
        return cast(ast, type);
    case TypeComparison::Equal:
        return {};
    }
}

Result<void> SemanticAnalyzer::cast(AstExpr*& ast, const TypeRoot* type) {
    auto category = ast->flags;
    auto* cast = m_context.create<AstCastExpr>(
        ast->range,
        ast,
        nullptr,
        true);
    cast->type = type;
    cast->flags = category;
    ast = cast; // NOLINT
    return {};
}

//------------------------------------------------------------------
// IfExpr
//------------------------------------------------------------------

Result<void> SemanticAnalyzer::visit(AstIfExpr& ast) {
    TRY(expression(ast.expr, TypeBoolean::get()));
    TRY(expression(ast.trueExpr));
    TRY(expression(ast.falseExpr));

    const auto convert = [&](AstExpr*& expr, const TypeRoot* ty) -> Result<void> {
        TRY(cast(expr, ty));
        m_constantFolder.fold(expr);
        ast.type = ty;
        return {};
    };

    const auto* left = ast.trueExpr->type;
    const auto* right = ast.falseExpr->type;
    switch (left->compare(right)) {
    case TypeComparison::Incompatible:
        fatalError("Incompatible types");
    case TypeComparison::Downcast:
        return convert(ast.falseExpr, left);
    case TypeComparison::Equal:
        ast.type = left;
        return {};
    case TypeComparison::Upcast:
        return convert(ast.trueExpr, right);
    }
}
