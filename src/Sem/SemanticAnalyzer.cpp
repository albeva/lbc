//
// Created by Albert Varaksin on 08/07/2020.
//
#include "SemanticAnalyzer.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Token.hpp"
#include "Passes/ForStmtPass.hpp"
#include "Passes/ForwardDeclPass.hpp"
#include "Passes/FuncDeclarerPass.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Type.hpp"
#include "Type/TypeProxy.hpp"
#include "Type/TypeUdt.hpp"

using namespace lbc;

SemanticAnalyzer::SemanticAnalyzer(Context& context)
: m_context{ context },
  m_constantFolder{ *this },
  m_typePass{ *this } {}

void SemanticAnalyzer::visit(AstModule& ast) {
    m_astRootModule = &ast;
    ast.symbolTable = m_context.create<SymbolTable>(nullptr);
    m_table = ast.symbolTable;

    Sem::ForwardDeclPass(*this).visit(ast);
    Sem::FuncDeclarerPass(*this).visit(ast);
    visit(*ast.stmtList);
}

void SemanticAnalyzer::visit(AstStmtList& ast) {
    for (auto& stmt : ast.stmts) {
        visit(*stmt);
    }
}

void SemanticAnalyzer::visit(AstImport& ast) {
    if (ast.module == nullptr) {
        return;
    }

    visit(*ast.module->stmtList);
}

void SemanticAnalyzer::visit(AstExprList& /*ast*/) {
    llvm_unreachable("Unhandled AstExprList&");
}

void SemanticAnalyzer::visit(AstExprStmt& ast) {
    expression(ast.expr);
}

void SemanticAnalyzer::visit(AstVarDecl& ast) {
    // m_type expr?
    TypeProxy* type = nullptr;
    if (ast.typeExpr != nullptr) {
        type = m_typePass.visit(*ast.typeExpr);
    }

    // expression?
    if (ast.expr != nullptr) {
        expression(ast.expr, type == nullptr ? nullptr : type->getType());
        if (type == nullptr) {
            type = ast.expr->typeProxy;
        }
    }

    if (type == nullptr) {
        fatalError("no type for var declaration");
    }

    // The Symbol
    auto* symbol = createNewSymbol(ast);
    symbol->getFlags().external = false;

    // create function symbol
    symbol->setTypeProxy(type);
    ast.symbol = symbol;
    auto flags = ast.symbol->getFlags();
    flags.addressable = true;
    flags.assignable = true;
    if (type->getType()->isPointer()) {
        flags.dereferencable = true;
    }
    ast.symbol->setFlags(flags);

    // alias?
    if (ast.attributes != nullptr) {
        if (auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
        }
    }
}

//----------------------------------------
// Functions
//----------------------------------------

/**
 * Analyze function declaration
 */
void SemanticAnalyzer::visit(AstFuncDecl& /*ast*/) {
    // NOOP
}

void SemanticAnalyzer::visit(AstFuncParamDecl& /*ast*/) {
    llvm_unreachable("visit");
}

void SemanticAnalyzer::visit(AstFuncStmt& ast) {
    RESTORE_ON_EXIT(m_table);
    RESTORE_ON_EXIT(m_function);
    m_function = ast.decl;
    m_table = ast.decl->symbolTable;
    visit(*ast.stmtList);
}

void SemanticAnalyzer::visit(AstReturnStmt& ast) {
    const TypeRoot* retType = nullptr;
    bool canOmitExpression = false;
    if (m_function == nullptr) {
        if (!m_astRootModule->hasImplicitMain) {
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
        return;
    }

    if (isVoid) {
        fatalError("Unexpected expression for SUB");
    }

    expression(ast.expr);

    if (ast.expr->getType() != retType) {
        fatalError(
            "Return expression type mismatch."_t
            + " Expected (" + retType->asString() + ")"
            + " got (" + ast.expr->getType()->asString() + ")");
    }
}

void SemanticAnalyzer::visit(AstIfStmt& ast) {
    RESTORE_ON_EXIT(m_table);
    for (auto& block : ast.blocks) {
        block->symbolTable = m_context.create<SymbolTable>(m_table);
    }

    for (size_t idx = 0; idx < ast.blocks.size(); idx++) {
        auto& block = ast.blocks[idx];

        m_table = block->symbolTable;
        for (auto& var : block->decls) {
            visit(*var);
            for (size_t next = idx + 1; next < ast.blocks.size(); next++) {
                ast.blocks[next]->symbolTable->addReference(var->symbol);
            }
        }
        if (block->expr != nullptr) {
            expression(block->expr);
            if (!block->expr->getType()->isBoolean()) {
                fatalError("type '"_t
                    + block->expr->getType()->asString()
                    + "' cannot be used as boolean");
            }
        }
        visit(*block->stmt);
    }
}

void SemanticAnalyzer::visit(AstForStmt& ast) {
    Sem::ForStmtPass(*this).visit(ast);
}

void SemanticAnalyzer::visit(AstDoLoopStmt& ast) {
    RESTORE_ON_EXIT(m_table);
    ast.symbolTable = m_context.create<SymbolTable>(m_table);
    m_table = ast.symbolTable;

    for (auto& var : ast.decls) {
        visit(*var);
    }

    if (ast.expr != nullptr) {
        expression(ast.expr);
        if (!ast.expr->getType()->isBoolean()) {
            fatalError("type '"_t
                + ast.expr->getType()->asString()
                + "' cannot be used as boolean");
        }
    }

    m_controlStack.push(ControlFlowStatement::Do);
    visit(*ast.stmt);
    m_controlStack.pop();
}

void SemanticAnalyzer::visit(AstContinuationStmt& ast) {
    if (m_controlStack.find(ast.destination) == m_controlStack.cend()) {
        fatalError("control statement not found");
    }
}

//----------------------------------------
// Type (user defined)
//----------------------------------------

void SemanticAnalyzer::visit(AstUdtDecl& /* ast */) {
    // NO OP
}

//----------------------------------------
// Type alias
//----------------------------------------

void SemanticAnalyzer::visit(AstTypeAlias& /* ast */) {
    // NO OP
}

//----------------------------------------
// Attributes
//----------------------------------------

void SemanticAnalyzer::visit(AstAttributeList& /*ast*/) {
    llvm_unreachable("visitAttributeList");
}

void SemanticAnalyzer::visit(AstAttribute& /*ast*/) {
    llvm_unreachable("visitAttribute");
}

//----------------------------------------
// Types
//----------------------------------------

void SemanticAnalyzer::visit(AstTypeExpr& /*ast*/) {
    llvm_unreachable("AstTypeExpr");
}

//----------------------------------------
// Expressions
//----------------------------------------

void SemanticAnalyzer::expression(AstExpr*& ast, const TypeRoot* type) {
    visit(*ast);
    m_constantFolder.fold(ast);
    if (type != nullptr) {
        coerce(ast, type);
        m_constantFolder.fold(ast);
    }
}

void SemanticAnalyzer::visit(AstAssignExpr& ast) {
    visit(*ast.lhs);
    if (!ast.lhs->flags.assignable) {
        fatalError("Cannot assign");
    }
    expression(ast.rhs, ast.lhs->getType());
}

void SemanticAnalyzer::visit(AstIdentExpr& ast) {
    auto* symbol = m_table->find(ast.name);
    if (symbol == nullptr) {
        fatalError("Unknown identifier "_t + ast.name);
    }

    auto* proxy = symbol->getTypeProxy();
    if (proxy == nullptr) {
        fatalError("Identifier "_t + ast.name + " has unresolved type");
    }

    ast.typeProxy = proxy;
    ast.symbol = symbol;
    ast.flags = symbol->getFlags();
}

void SemanticAnalyzer::visit(AstCallExpr& ast) {
    visit(*ast.callable);

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
            expression(args[index], paramTypes[index]);
        } else {
            expression(args[index]);
        }
    }

    ast.typeProxy = type->getReturn()->getProxy();
}

void SemanticAnalyzer::visit(AstLiteralExpr& ast) {
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
    ast.typeProxy = TypeRoot::fromTokenKind(typeKind)->getProxy();
}

//------------------------------------------------------------------
// Unary Expressions
//------------------------------------------------------------------

void SemanticAnalyzer::visit(AstUnaryExpr& ast) {
    expression(ast.expr);
    auto* type = ast.expr->typeProxy;

    switch (ast.tokenKind) {
    case TokenKind::LogicalNot:
        if (type->getType()->isBoolean()) {
            ast.typeProxy = type;
            return;
        }
        fatalError("Applying unary NOT to non bool type");
    case TokenKind::Negate:
        if (type->getType()->isNumeric()) {
            ast.typeProxy = type;
            return;
        }
        fatalError("Applying unary negate to non-numeric type");
    default:
        llvm_unreachable("unknown unary operator");
    }
}

//------------------------------------------------------------------
// Dereference
//------------------------------------------------------------------

void SemanticAnalyzer::visit(AstDereference& ast) {
    // TODO dereference needs to return a reference to value, NOT value itself

    visit(*ast.expr);
    if (const auto* type = llvm::dyn_cast<TypePointer>(ast.expr->getType())) {
        ast.typeProxy = type->getBase()->getProxy();
    } else {
        fatalError("dereferencing a non pointer");
    }

    ast.flags = ast.expr->flags;
    if (!ast.getType()->isPointer()) {
        ast.flags.dereferencable = false;
    }
}

//------------------------------------------------------------------
// AddressOf
//------------------------------------------------------------------

void SemanticAnalyzer::visit(AstAddressOf& ast) {
    visit(*ast.expr);
    if (!ast.expr->flags.addressable) {
        fatalError("Cannot take address");
    }
    ast.typeProxy = TypePointer::get(m_context, ast.expr->getType())->getProxy();
    ast.flags = ast.expr->flags;
    ast.flags.dereferencable = true;
}

//------------------------------------------------------------------
// Member Access
//------------------------------------------------------------------

void SemanticAnalyzer::visit(AstMemberAccess& ast) {
    RESTORE_ON_EXIT(m_table);

    for (size_t i = 0; i < ast.exprs.size(); i++) {
        auto* expr = ast.exprs[i];
        visit(*expr);
        auto* type = expr->typeProxy;

        if (i == (ast.exprs.size() - 1)) {
            ast.typeProxy = type;
            ast.flags = expr->flags;
        } else {
            const TypeUDT* udt = nullptr;
            if (type->getType()->isUDT()) {
                udt = static_cast<const TypeUDT*>(type->getType());
            } else if (const auto* ptr = llvm::dyn_cast<TypePointer>(type->getType())) {
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
}

//------------------------------------------------------------------
// Binary Expressions
//------------------------------------------------------------------

void SemanticAnalyzer::visit(AstBinaryExpr& ast) {
    expression(ast.lhs);
    expression(ast.rhs);

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

void SemanticAnalyzer::arithmetic(AstBinaryExpr& ast) {
    auto* left = ast.lhs->typeProxy;
    auto* right = ast.rhs->typeProxy;

    if (!left->getType()->isNumeric() || !right->getType()->isNumeric()) {
        fatalError("Applying artithmetic operation to non numeric type");
    }

    const auto convert = [&](AstExpr*& expr, const TypeRoot* ty) {
        cast(expr, ty);
        m_constantFolder.fold(expr);
        ast.typeProxy = ty->getProxy();
    };

    switch (left->getType()->compare(right->getType())) {
    case TypeComparison::Incompatible:
        fatalError("Operator on incompatible types");
    case TypeComparison::Downcast:
        return convert(ast.rhs, left->getType());
    case TypeComparison::Equal:
        ast.typeProxy = left;
        return;
    case TypeComparison::Upcast:
        return convert(ast.lhs, right->getType());
    }
}

void SemanticAnalyzer::logical(AstBinaryExpr& ast) {
    auto* left = ast.lhs->typeProxy;
    auto* right = ast.rhs->typeProxy;

    if (!left->getType()->isBoolean() || !right->getType()->isBoolean()) {
        fatalError("Applying logical operator to non boolean type");
    }
    ast.typeProxy = left;
}

void SemanticAnalyzer::comparison(AstBinaryExpr& ast) {
    auto* left = ast.lhs->typeProxy;
    auto* right = ast.rhs->typeProxy;

    if (!canPerformBinary(ast.tokenKind, left->getType(), right->getType())) {
        fatalError("Cannot apply operationg to types");
    }

    const auto convert = [&](AstExpr*& expr, const TypeRoot* ty) {
        cast(expr, ty);
        m_constantFolder.fold(expr);
        ast.typeProxy = TypeBoolean::get()->getProxy();
    };

    switch (left->getType()->compare(right->getType())) {
    case TypeComparison::Incompatible:
        fatalError("Operator on incompatible types");
    case TypeComparison::Downcast:
        return convert(ast.rhs, left->getType());
    case TypeComparison::Equal:
        ast.typeProxy = TypeBoolean::get()->getProxy();
        return;
    case TypeComparison::Upcast:
        return convert(ast.lhs, right->getType());
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

void SemanticAnalyzer::visit(AstCastExpr& ast) {
    ast.typeProxy = m_typePass.visit(*ast.typeExpr);
    expression(ast.expr);

    if (ast.expr->getType()->compare(ast.getType()) == TypeComparison::Incompatible) {
        fatalError("Incompatible cast");
    }

    ast.flags = ast.expr->flags;
}

void SemanticAnalyzer::convert(AstExpr*& ast, const TypeRoot* type) {
    cast(ast, type);
    m_constantFolder.fold(ast);
}

void SemanticAnalyzer::coerce(AstExpr*& ast, const TypeRoot* type) {
    if (ast->getType() == type) {
        return;
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
        return;
    }
}

void SemanticAnalyzer::cast(AstExpr*& ast, const TypeRoot* type) {
    auto category = ast->flags;
    auto* cast = m_context.create<AstCastExpr>(
        ast->range,
        ast,
        nullptr,
        true);
    cast->typeProxy = type->getProxy();
    cast->flags = category;
    ast = cast; // NOLINT
}

//------------------------------------------------------------------
// IfExpr
//------------------------------------------------------------------

void SemanticAnalyzer::visit(AstIfExpr& ast) {
    expression(ast.expr, TypeBoolean::get());
    expression(ast.trueExpr);
    expression(ast.falseExpr);

    const auto convert = [&](AstExpr*& expr, const TypeRoot* ty) {
        cast(expr, ty);
        m_constantFolder.fold(expr);
        ast.typeProxy = ty->getProxy();
    };

    auto* left = ast.trueExpr->typeProxy;
    auto* right = ast.falseExpr->typeProxy;
    switch (left->getType()->compare(right->getType())) {
    case TypeComparison::Incompatible:
        fatalError("Incompatible types");
    case TypeComparison::Downcast:
        return convert(ast.falseExpr, left->getType());
    case TypeComparison::Equal:
        ast.typeProxy = left;
        return;
    case TypeComparison::Upcast:
        return convert(ast.trueExpr, right->getType());
    }
}

//------------------------------------------------------------------
// Utils
//------------------------------------------------------------------

Symbol* SemanticAnalyzer::createNewSymbol(AstDecl& ast) {
    if (m_table->find(ast.name, false) != nullptr) {
        fatalError("Redefinition of "_t + ast.name);
    }
    auto* symbol = m_table->insert(m_context, ast.name);

    // alias?
    if (ast.attributes != nullptr) {
        if (auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
        }
    }

    return symbol;
}
