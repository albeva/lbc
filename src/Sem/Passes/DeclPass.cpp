//
// Created by Albert on 26/02/2022.
//
#include "DeclPass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"
#include "Type/TypeUdt.hpp"
using namespace lbc;
using namespace Sem;

//----------------------------------------
// Declare symbols
//----------------------------------------

void DeclPass::declare(AstStmtList& ast) {
    for (auto* decl : ast.decl) {
        declare(*decl);
    }
}

void DeclPass::declare(AstDecl& ast) {
    auto* symbol = createNewSymbol(ast, nullptr);

    if (llvm::isa<AstFuncDecl>(&ast)) {
        symbol->valueFlags().kind = ValueFlags::Kind::function;
    } else if (llvm::isa<AstVarDecl>(&ast)) {
        symbol->valueFlags().kind = ValueFlags::Kind::variable;
    } else {
        symbol->valueFlags().kind = ValueFlags::Kind::type;
    }

    ast.symbol = symbol;
}

void DeclPass::declareAndDefine(const std::vector<AstVarDecl*>& vars) {
    for (auto* var : vars) {
        declareAndDefine(*var);
    }
}

void DeclPass::declareAndDefine(AstVarDecl& var) {
    declare(var);
    define(var.symbol);
    var.symbol->stateFlags().declared = true;
}

//----------------------------------------
// Define symbol
//----------------------------------------

void DeclPass::define(Symbol* symbol) {
    auto& state = symbol->stateFlags();
    if (state.beingDefined) {
        fatalError("Circular dependency detected on "_t + symbol->name());
    }
    state.beingDefined = true;

    DEFER {
        state.beingDefined = false;
    };

    auto* ast = symbol->getDecl();
    if (auto* alias = llvm::dyn_cast<AstTypeAlias>(ast)) {
        return defineAlias(*alias);
    }
    if (auto* udt = llvm::dyn_cast<AstUdtDecl>(ast)) {
        return defineUdt(*udt);
    }
    if (auto* func = llvm::dyn_cast<AstFuncDecl>(ast)) {
        return defineFunc(*func);
    }
    if (auto* var = llvm::dyn_cast<AstVarDecl>(ast)) {
        return defineVar(*var);
    }
    llvm_unreachable("Unknown decl type");
}

void DeclPass::defineAlias(AstTypeAlias& ast) {
    static constexpr auto getSymbol = Visitor{
        [](AstIdentExpr* ident) -> Symbol* {
            return ident->symbol;
        },
        [](AstFuncDecl* decl) -> Symbol* {
            return decl->symbol;
        },
        [](auto) -> Symbol* {
            return nullptr;
        }
    };

    auto* symbol = ast.symbol;
    symbol->setType(m_sem.getTypePass().visit(*ast.typeExpr));

    if (auto* parent = std::visit(getSymbol, ast.typeExpr->expr)) {
        symbol->valueFlags() = parent->valueFlags();
    } else {
        symbol->valueFlags().kind = ValueFlags::Kind::type;
    }
}

void DeclPass::defineUdt(AstUdtDecl& ast) {
    auto* symbol = ast.symbol;
    bool packed = false;
    if (ast.attributes != nullptr) {
        packed = ast.attributes->exists("PACKED");
    }
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable());
    TypeUDT::get(m_sem.getContext(), *symbol, *ast.symbolTable, packed);

    m_sem.with(ast.symbolTable, [&]() {
        for (auto* decl : ast.decls->decls) {
            declare(*decl);
        }
        unsigned index = 0;
        for (auto* decl : ast.decls->decls) {
            define(decl->symbol);
            decl->symbol->setIndex(index++);
            decl->symbol->stateFlags().declared = true;
        }
    });
}

void DeclPass::defineFunc(AstFuncDecl& ast) {
    auto* symbol = ast.symbol;

    // main or external?
    if (m_sem.hasImplicitMain() && symbol->name() == "MAIN" && symbol->alias().empty()) {
        symbol->setAlias("main");
        symbol->valueFlags().external = true;
    } else {
        symbol->valueFlags().external = !ast.hasImpl;
    }

    // func type
    symbol->setType(m_sem.getTypePass().visit(ast));

    // parameters
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable(), &ast);
    if (ast.params != nullptr) {
        m_sem.with(ast.symbolTable, [&]() {
            for (const auto& param : ast.params->params) {
                defineFuncParam(*param);
            }
        });
    }
}

void DeclPass::defineFuncParam(AstFuncParamDecl& ast) {
    const auto* type = ast.typeExpr->type;
    if (type->isUDT()) {
        fatalError("Passing types by values is not implemented");
    }

    auto* symbol = createNewSymbol(ast, type);
    ast.symbol = symbol;
}

void DeclPass::defineVar(AstVarDecl& ast) {
    // m_type expr?
    const TypeRoot* type = nullptr;
    if (ast.typeExpr != nullptr) {
        type = m_sem.getTypePass().visit(*ast.typeExpr);
    }

    // expression?
    if (ast.expr != nullptr) {
        MUST(m_sem.expression(ast.expr, type));
        if (type == nullptr) {
            type = ast.expr->type;
        }
    }

    if (type == nullptr) [[unlikely]] {
        fatalError("Variable declaration is missing a type");
    }

    // The Symbol
    auto* symbol = ast.symbol;
    symbol->valueFlags().external = false;
    symbol->valueFlags().assignable = true;
    symbol->setType(type);
    ast.symbol = symbol;
}

//----------------------------------------
// Utils
//----------------------------------------

Symbol* DeclPass::createNewSymbol(AstDecl& ast, const TypeRoot* type) {
    if (m_sem.getSymbolTable()->find(ast.name, false) != nullptr) {
        fatalError("Redefinition of "_t + ast.name);
    }

    auto* symbol = m_sem.getContext().create<Symbol>(
        ast.name,
        m_sem.getSymbolTable(),
        type,
        &ast);

    m_sem.getSymbolTable()->insert(symbol);

    // alias?
    bool hasAlias = false;
    if (ast.attributes != nullptr) {
        if (auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
            hasAlias = true;
        }
    }

    if (!hasAlias && ast.callingConv == CallingConv::C) {
        symbol->setAlias(ast.token.lexeme());
    }

    return symbol;
}