//
// Created by Albert on 26/02/2022.
//
#include "DeclPass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Type/Type.hpp"
#include "Type/TypeUdt.hpp"
using namespace lbc;
using namespace Sem;

void DeclPass::visit(AstModule& ast) noexcept {
    declare(*ast.stmtList);
}

//----------------------------------------
// Declare symbol
//----------------------------------------

void DeclPass::declare(AstStmtList& ast) noexcept {
    for (auto& stmt : ast.stmts) {
        if (auto* decl = llvm::dyn_cast<AstDecl>(stmt)) {
            declare(*decl);
            continue;
        }
        if (auto* import = llvm::dyn_cast<AstImport>(stmt)) {
            declare(*import->module->stmtList);
            continue;
        }
        if (auto* func = llvm::dyn_cast<AstFuncStmt>(stmt)) {
            declare(*func->decl);
            continue;
        }
    }
}

void DeclPass::declare(AstDecl& ast) noexcept {
    if (not llvm::isa<AstUdtDecl, AstTypeAlias, AstFuncDecl>(ast)) {
        return;
    }

    auto* symbol = m_sem.createNewSymbol(ast);
    symbol->setDecl(&ast);

    if (auto* func = llvm::dyn_cast<AstFuncDecl>(&ast)) {
        symbol->valueFlags().callable = true;
        symbol->valueFlags().addressable = true;
    } else {
        symbol->valueFlags().isType = true;
    }

    ast.symbol = symbol;
}

//----------------------------------------
// Define symbol
//----------------------------------------

void DeclPass::define(Symbol* symbol) noexcept {
    auto& state = symbol->stateFlags();
    if (state.beingDefined) {
        fatalError("Circular dependency detected on "_t + symbol->name());
    }
    state.beingDefined = true;
    SCOPE_GAURD(state.beingDefined = false);

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
    llvm_unreachable("Unknown decl type");
}

void DeclPass::defineAlias(AstTypeAlias& ast) noexcept {
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
    if (symbol->stateFlags().defined) {
        return;
    }

    symbol->setType(m_sem.getTypePass().visit(*ast.typeExpr));

    if (auto* parent = std::visit(getSymbol, ast.typeExpr->expr)) {
        symbol->valueFlags() = parent->valueFlags();
        symbol->setParent(parent->getParent());
    } else {
        symbol->valueFlags().isType = true;
    }

    symbol->stateFlags().defined = true;
}

void DeclPass::defineUdt(AstUdtDecl& ast) noexcept {
    auto* symbol = ast.symbol;
    if (symbol->stateFlags().defined) {
        return;
    }

    bool packed = false;
    if (ast.attributes != nullptr) {
        packed = ast.attributes->exists("PACKED");
    }
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable());
    const auto* udt = TypeUDT::get(m_sem.getContext(), *symbol, *ast.symbolTable, packed);

    m_sem.with(ast.symbolTable, [&]() {
        for (auto& decl : ast.decls->decls) {
            m_sem.visit(*decl);
            decl->symbol->setParent(symbol);
        }
    });

    symbol->stateFlags().defined = true;
}

void DeclPass::defineFunc(AstFuncDecl& ast) noexcept {
    auto* symbol = ast.symbol;
    if (symbol->stateFlags().defined) {
        return;
    }

    // alias?
    if (ast.attributes != nullptr) {
        if (auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
        }
    }

    // main or external?
    if (symbol->name() == "MAIN" && symbol->alias().empty()) {
        symbol->setAlias("main");
        symbol->valueFlags().isExternal = true;
    } else {
        symbol->valueFlags().isExternal = !ast.hasImpl;
    }

    // func type
    symbol->setType(m_sem.getTypePass().visit(ast));

    // parameters
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable());
    if (ast.params != nullptr) {
        m_sem.with(ast.symbolTable, [&]() {
            for (auto& param : ast.params->params) {
                defineFuncParam(*param);
            }
        });
    }

    // mark defined
    symbol->stateFlags().defined = true;
}

void DeclPass::defineFuncParam(AstFuncParamDecl& ast) noexcept {
    const auto* type = ast.typeExpr->type;
    if (type->isUDT()) {
        fatalError("Passing types by values is not implemented");
    }

    auto* symbol = createParamSymbol(ast);
    symbol->setType(type);
    symbol->stateFlags().defined = true;
    ast.symbol = symbol;
}

Symbol* DeclPass::createParamSymbol(AstFuncParamDecl& ast) noexcept {
    const auto& name = ast.name;
    if (m_sem.getSymbolTable()->find(name, false) != nullptr) {
        fatalError("Redefinition of "_t + name);
    }
    auto* symbol = m_sem.getSymbolTable()->insert(m_sem.getContext(), name);

    // alias?
    if (ast.attributes != nullptr) {
        if (auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
        }
    }

    return symbol;
}
