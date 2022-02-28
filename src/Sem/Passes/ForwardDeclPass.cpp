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
        }
    }
}

void ForwardDeclPass::declare(AstDeclList& ast) noexcept {
    for (auto& decl : ast.decls) {
        declare(*decl);
    }
}

void ForwardDeclPass::declare(AstDecl& ast) noexcept {
    if (not llvm::isa<AstUdtDecl, AstTypeAlias>(ast)) {
        return;
    }

    auto* symbol = m_sem.createNewSymbol(ast);
    symbol->setDecl(&ast);
    symbol->getFlags().type = true;
    symbol->setTypeProxy(m_sem.getContext().create<TypeProxy>());
    ast.symbol = symbol;
    m_nodes.emplace_back(&ast);
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
        [](TokenKind) -> Symbol* {
            return nullptr;
        }
    };

    auto* symbol = ast.symbol;
    auto* proxy = symbol->getTypeProxy();
    auto* aliasedProxy = m_sem.getTypePass().visit(*ast.typeExpr, proxy);
    if (isCircularAlias(proxy, aliasedProxy)) {
        fatalError("Circular reference for '"_t + symbol->name() + "'");
    }
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

//----------------------------------------
// Utils
//----------------------------------------
bool ForwardDeclPass::isCircularAlias(TypeProxy* proxy, TypeProxy* aliased) const noexcept {
    do {
        if (proxy == aliased) {
            return true;
        }
        aliased = aliased->getNestedProxy();
    } while (aliased != nullptr);
    return false;
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
