//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstExprPrinter.hpp"
#include "Lexer/TokenKind.hpp"
using namespace lbc;

auto AstExprPrinter::print(const AstExpr& ast) -> std::string {
    m_output.clear();
    visit(ast);
    return m_output;
}

void AstExprPrinter::accept(const AstCastExpr& ast) {
    m_output += "(";
    accept(*ast.getExpr());
    m_output += " AS ";
    accept(*ast.getTypeExpr());
    m_output += ")";
}

void AstExprPrinter::accept(const AstVarExpr& ast) {
    m_output += ast.getName();
}

void AstExprPrinter::accept(const AstCallExpr& ast) {
    visit(*ast.getCallee());
    m_output += "(";
    bool first = true;
    for (const auto& arg : ast.getArgs()) {
        if (first) {
            first = false;
        } else {
            m_output += ", ";
        }
        visit(*arg);
    }
    m_output += ")";
}

void AstExprPrinter::accept(const AstLiteralExpr& ast) {
    const auto visitor = Visitor {
        [&](const std::monostate& /* value */) {
            m_output += "null";
        },
        [&](const double& value) {
            m_output += std::to_string(value);
        },
        [&](const std::uint64_t& value) {
            m_output += std::to_string(value);
        },
        [&](const bool& value) {
            m_output += value ? "true" : "false";
        },
        [&](const llvm::StringRef& value) {
            m_output += value;
        }
    };
    std::visit(visitor, ast.getValue().storage());
}

void AstExprPrinter::accept(const AstUnaryExpr& ast) {
    m_output += "(";
    m_output += ast.getOp().string();
    visit(*ast.getExpr());
    m_output += ")";
}

void AstExprPrinter::accept(const AstBinaryExpr& ast) {
    m_output += "(";
    visit(*ast.getLeft());
    m_output += " ";
    m_output += ast.getOp().string();
    m_output += " ";
    visit(*ast.getRight());
    m_output += ")";
}

void AstExprPrinter::accept(const AstMemberExpr& ast) {
    visit(*ast.getLeft());
    m_output += ".";
    visit(*ast.getRight());
}
