//
// Created by Albert on 26/02/2022.
//
#include "ForwardDeclPass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Type/Type.hpp"
#include "Type/TypeProxy.hpp"
#include "Type/TypeUdt.hpp"
using namespace lbc;
using namespace Sem;

void ForwardDeclPass::visit(AstModule& ast) noexcept {
    declare(*ast.stmtList);

    for (auto* node : m_nodes) {
        define(*node);
    }

    for (auto* udt : m_udts) {
        implement(*udt);
    }

    for (auto* func : m_funcs) {
        implement(*func);
    }
}

//----------------------------------------
// Declare symbol
//----------------------------------------

void ForwardDeclPass::declare(AstStmtList& ast) noexcept {
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

void ForwardDeclPass::declare(AstDecl& ast) noexcept {
    if (not llvm::isa<AstUdtDecl, AstTypeAlias, AstFuncDecl>(ast)) {
        return;
    }

    auto* symbol = m_sem.createNewSymbol(ast);
    symbol->setTypeProxy(m_sem.getContext().create<TypeProxy>());
    symbol->setDecl(&ast);

    if (auto* func = llvm::dyn_cast<AstFuncDecl>(&ast)) {
        symbol->getFlags().callable = true;
        symbol->getFlags().addressable = true;
        m_funcs.push_back(func);
    } else {
        symbol->getFlags().type = true;
        m_nodes.emplace_back(&ast);
    }

    ast.symbol = symbol;
}

//----------------------------------------
// Define symbol type
//----------------------------------------

void ForwardDeclPass::define(AstDecl& ast) noexcept {
    if (auto* alias = llvm::dyn_cast<AstTypeAlias>(&ast)) {
        return define(*alias);
    }
    if (auto* udt = llvm::dyn_cast<AstUdtDecl>(&ast)) {
        return define(*udt);
    }
}

void ForwardDeclPass::define(AstTypeAlias& ast) noexcept {
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
    auto* proxy = symbol->getTypeProxy();
    auto* aliasedProxy = m_sem.getTypePass().visit(*ast.typeExpr, proxy);
    checkCircularAlias(proxy, aliasedProxy);
    proxy->setNestedProxy(aliasedProxy);

    if (auto* parent = std::visit(getSymbol, ast.typeExpr->expr)) {
        symbol->setFlags(parent->getFlags());
        symbol->setParent(parent->getParent());
    } else {
        symbol->getFlags().type = true;
    }
}

void ForwardDeclPass::define(AstUdtDecl& ast) noexcept {
    auto* symbol = ast.symbol;
    bool packed = false;
    if (ast.attributes != nullptr) {
        packed = ast.attributes->exists("PACKED");
    }
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable());
    TypeUDT::get(m_sem.getContext(), *symbol, *ast.symbolTable, packed);
    m_udts.push_back(&ast);
}

//----------------------------------------
// Implement
//----------------------------------------

void ForwardDeclPass::implement(AstUdtDecl& ast) noexcept {
    const auto* udt = ast.symbol->getType();
    m_sem.with(ast.symbolTable, [&]() {
        for (auto& decl : ast.decls->decls) {
            m_sem.visit(*decl);
            decl->symbol->setParent(ast.symbol);
            const auto* nested = decl->symbol->getType();
            if (nested->isUDT()) {
                checkCircularDependency(udt, nested);
            }
        }
    });
}

void ForwardDeclPass::implement(AstFuncDecl& ast) noexcept {
    auto* symbol = ast.symbol;

    // alias?
    if (ast.attributes != nullptr) {
        if (auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
        }
    }

    // main or external?
    if (symbol->name() == "MAIN" && symbol->alias().empty()) {
        symbol->setAlias("main");
        symbol->getFlags().external = true;
    } else {
        symbol->getFlags().external = !ast.hasImpl;
    }

    // func type
    symbol->getTypeProxy()->setNestedProxy(m_sem.getTypePass().visit(ast));

    // parameters
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable());
    if (ast.params != nullptr) {
        m_sem.with(ast.symbolTable, [&]() {
            for (auto& param : ast.params->params) {
                implement(*param);
            }
        });
    }
}

void ForwardDeclPass::implement(AstFuncParamDecl& ast) noexcept {
    auto* proxy = ast.typeExpr->typeProxy;
    if (proxy->getType()->isUDT()) {
        fatalError("Passing types by values is not implemented");
    }

    auto* symbol = createParamSymbol(ast);
    symbol->setTypeProxy(proxy);
    ast.symbol = symbol;
}

//----------------------------------------
// Utils
//----------------------------------------
void ForwardDeclPass::checkCircularAlias(TypeProxy* proxy, TypeProxy* aliased) const noexcept {
    do {
        if (proxy == aliased) {
            fatalError("Circular type alias");
        }
        aliased = aliased->getNestedProxy();
    } while (aliased != nullptr);
}

void ForwardDeclPass::checkCircularDependency(const TypeRoot* udt, const TypeRoot* nested) noexcept {
    const auto* lower = std::min(udt, nested);
    const auto* higher = std::max(udt, nested);
    auto key = RelKey{ lower, higher };

    auto result = m_typeRelations.try_emplace(key, udt);
    if (result.first->getSecond() != udt) {
        fatalError("Nested type declarations");
    }
}

Symbol* ForwardDeclPass::createParamSymbol(AstFuncParamDecl& ast) noexcept {
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
