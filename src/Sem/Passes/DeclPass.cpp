//
// Created by Albert on 26/02/2022.
//
#include "DeclPass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/ConstantFolder.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"
#include "Type/TypeUdt.hpp"
using namespace lbc;
using namespace Sem;

//----------------------------------------
// Declare symbols
//----------------------------------------

auto DeclPass::declare(const AstStmtList& ast) const -> Result<void> {
    for (auto* decl : ast.decl) {
        TRY(declare(*decl))
    }
    return {};
}

auto DeclPass::declare(AstDecl& ast) const -> Result<void> {
    TRY_DECL(symbol, createNewSymbol(ast, nullptr))

    if (llvm::isa<AstFuncDecl>(&ast)) {
        symbol->valueFlags().kind = ValueFlags::Kind::Function;
    } else if (llvm::isa<AstVarDecl>(&ast)) {
        symbol->valueFlags().kind = ValueFlags::Kind::Variable;
    } else {
        symbol->valueFlags().kind = ValueFlags::Kind::Type;
    }

    ast.symbol = symbol;
    return {};
}

auto DeclPass::declareAndDefine(const std::vector<AstVarDecl*>& vars) -> Result<void> {
    for (auto* var : vars) {
        TRY(declareAndDefine(*var))
    }
    return {};
}

auto DeclPass::declareAndDefine(AstVarDecl& var) -> Result<void> {
    TRY(declare(var))
    TRY(define(var))
    var.symbol->stateFlags().declared = true;
    return {};
}

//----------------------------------------
// Define symbol
//----------------------------------------

auto DeclPass::define(AstDecl& ast) -> Result<void> {
    auto& state = ast.symbol->stateFlags();
    if (state.beingDefined) {
        return m_sem.makeError(
            Diag::circularTypeDependency,
            ast.token.getRange().Start,
            ast.getRange(),
            ast.symbol->name()
        );
    }
    state.beingDefined = true;

    DEFER {
        state.beingDefined = false;
    };

    if (const auto* alias = llvm::dyn_cast<AstTypeAlias>(&ast)) {
        return defineAlias(*alias);
    }
    if (auto* udt = llvm::dyn_cast<AstUdtDecl>(&ast)) {
        return defineUdt(*udt);
    }
    if (auto* func = llvm::dyn_cast<AstFuncDecl>(&ast)) {
        return defineFunc(*func);
    }
    if (auto* var = llvm::dyn_cast<AstVarDecl>(&ast)) {
        return defineVar(*var);
    }
    llvm_unreachable("Unknown decl type");
}

auto DeclPass::defineAlias(const AstTypeAlias& ast) const -> Result<void> {
    static constexpr auto getSymbol = Visitor {
        [](const AstIdentExpr* ident) -> Symbol* {
            return ident->symbol;
        },
        [](const AstFuncDecl* decl) -> Symbol* {
            return decl->symbol;
        },
        [](auto) -> Symbol* {
            return nullptr;
        }
    };

    auto* symbol = ast.symbol;
    TRY_DECL(type, m_sem.getTypePass().visit(*ast.typeExpr))
    symbol->setType(type);

    if (auto* parent = std::visit(getSymbol, ast.typeExpr->expr)) {
        symbol->valueFlags() = parent->valueFlags();
    } else {
        symbol->valueFlags().kind = ValueFlags::Kind::Type;
    }

    return {};
}

auto DeclPass::defineUdt(AstUdtDecl& ast) -> Result<void> {
    auto* symbol = ast.symbol;
    bool packed = false;
    if (ast.attributes != nullptr) {
        packed = ast.attributes->exists("PACKED");
    }
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable());
    TypeUDT::get(m_sem.getContext(), *symbol, *ast.symbolTable, packed);

    TRY(m_sem.with(ast.symbolTable, [&]() -> Result<void> {
        for (auto* decl : ast.decls->decls) {
            TRY(declare(*decl));
        }
        unsigned index = 0;
        for (auto* decl : ast.decls->decls) {
            TRY(define(*decl));
            decl->symbol->setIndex(index++);
            decl->symbol->stateFlags().declared = true;
        }

        return {};
    }))

    return {};
}

auto DeclPass::defineFunc(AstFuncDecl& ast) const -> Result<void> {
    auto* symbol = ast.symbol;

    // main or external?
    if (m_sem.hasImplicitMain() && symbol->name() == "MAIN" && symbol->alias().empty()) {
        symbol->setAlias("main");
        symbol->setVisibility(SymbolVisibility::External);
    } else {
        symbol->setVisibility(ast.visibility);
    }
    symbol->valueFlags().addressable = true;

    // func type
    TRY_DECL(type, m_sem.getTypePass().visit(ast))
    symbol->setType(type);

    // parameters
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable(), &ast);
    if (ast.params != nullptr) {
        TRY(m_sem.with(ast.symbolTable, [&]() -> Result<void> {
            for (const auto& param : ast.params->params) {
                TRY(defineFuncParam(*param));
            }
            return {};
        }))
    }

    return {};
}

auto DeclPass::defineFuncParam(AstFuncParamDecl& ast) const -> Result<void> {
    const auto* type = ast.typeExpr->type;
    TRY_ASSIGN(ast.symbol, createNewSymbol(ast, type))

    // TODO function params should be readonly/const by default
    ast.symbol->valueFlags().assignable = true;

    return {};
}

auto DeclPass::defineVar(AstVarDecl& ast) const -> Result<void> {
    // m_type expr?
    const TypeRoot* type = nullptr;
    if (ast.typeExpr != nullptr) {
        TRY_ASSIGN(type, m_sem.getTypePass().visit(*ast.typeExpr))
    }

    // expression?
    if (ast.expr != nullptr) {
        TRY(m_sem.expression(ast.expr, type))
        if (type == nullptr) {
            type = ast.expr->type;
        }
        ast.expr->type = type;

        if (ast.constant) {
            if (!ast.expr->constantValue.has_value()) {
                return m_sem.makeError(Diag::constantRequiresAConstantExpr, ast.token);
            }
            ast.symbol->setConstantValue(ast.expr->constantValue);
        }
    }

    if (type == nullptr) [[unlikely]] {
        llvm::errs() << "Variable declaration is missing a type\n";
        return ResultError {};
    }

    // The Symbol
    auto* symbol = ast.symbol;
    symbol->setVisibility(SymbolVisibility::Private);
    symbol->valueFlags().assignable = !ast.constant;
    symbol->valueFlags().addressable = !ast.constant;
    symbol->setType(type);
    ast.symbol = symbol;

    return {};
}

//----------------------------------------
// Utils
//----------------------------------------

auto DeclPass::createNewSymbol(AstDecl& ast, const TypeRoot* type) const -> Result<Symbol*> {
    if (m_sem.getSymbolTable()->find(ast.name, false) != nullptr) {
        return m_sem.makeError(Diag::symbolAlreadyDefined, ast.token, ast.name);
    }

    auto* symbol = m_sem.getContext().create<Symbol>(
        ast.name,
        m_sem.getSymbolTable(),
        type,
        &ast
    );

    m_sem.getSymbolTable()->insert(symbol);

    // alias?
    bool hasAlias = false;
    if (ast.attributes != nullptr) {
        if (const auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
            hasAlias = true;
        }
    }

    if (!hasAlias && ast.callingConv == CallingConv::C) {
        symbol->setAlias(ast.token.lexeme());
    }

    return symbol;
}