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

void AstExprPrinter::accept(const AstVariableExpr& ast) {
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
            m_output += std::to_string(value);
        },
        [&](const llvm::StringRef& value) {
            m_output += value;
        }
    };
    std::visit(visitor, ast.getValue());
}

void AstExprPrinter::accept(const AstUnaryExpr& ast) {
    m_output += ast.getTokenKind().string();
    visit(*ast.getExpr());
}

void AstExprPrinter::accept(const AstBinaryExpr& ast) {
    visit(*ast.getLeft());
    m_output += " ";
    m_output += ast.getTokenKind().string();
    m_output += " ";
    visit(*ast.getRight());
}

// void AstExprPrinter::accept(const AstCastExpr& ast) {
//     visit(*ast.getExpr());
//     m_output += " AS <not-implemented>";
//
// }

void AstExprPrinter::accept(const AstDereferenceExpr& ast) {
    m_output += "*";
    visit(*ast.getExpr());
}

void AstExprPrinter::accept(const AstAddressOfExpr& ast) {
    m_output += "@";
    visit(*ast.getExpr());
}

void AstExprPrinter::accept(const AstMemberExpr& ast) {
    visit(*ast.getExpr());
    m_output += ".";
    visit(*ast.getMember());
}

void AstExprPrinter::accept(const AstExrSubLeaf& ast) {
    (void)this;
    (void)ast;
    m_output += "AstExrSubLeaf";
}
