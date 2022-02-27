//
// Created by Albert on 25/02/2022.
//
#include "TypeAliasDeclPass.hpp"
#include "Ast/Ast.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Type/Type.hpp"
#include "TypePass.hpp"
#include "Type/TypeProxy.hpp"
#include "Driver/Context.hpp"
using namespace lbc;
using namespace Sem;

void TypeAliasDeclPass::visit(AstModule& ast) const noexcept {
    for (const auto& stmt : ast.stmtList->stmts) {
        switch (stmt->kind) {
        case AstKind::TypeAlias:
            visit(static_cast<AstTypeAlias&>(*stmt));
            break;
        default:
            break;
        }
    }
}

void TypeAliasDeclPass::visit(AstTypeAlias& ast) const noexcept {
    const auto* type = m_sem.getTypePass().visit(*ast.typeExpr);

    auto* symbol = m_sem.createNewSymbol(ast);
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
    if (auto* parent = std::visit(getSymbol, ast.typeExpr->expr)) {
        symbol->setFlags(parent->getFlags());
        symbol->setParent(parent->getParent());
    } else {
        symbol->getFlags().type = true;
    }

    symbol->setTypeProxy(ast.typeExpr->typeProxy);
    ast.symbol = symbol;
}
