//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstExprPrinter.hpp"
#include "Lexer/TokenKind.hpp"
using namespace lbc;

auto AstExprPrinter::print(CONST_PARAM AstExpr& ast) CONST_FUNC -> std::string {
    m_output.clear();
    visit(ast);
    return m_output;
}

void AstExprPrinter::accept(const AstVariableExpr& ast) const {
    m_output += ast.getName();
}

void AstExprPrinter::accept(CONST_PARAM AstCallExpr& ast) CONST_FUNC {
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

void AstExprPrinter::accept(CONST_PARAM AstLiteralExpr& ast) CONST_FUNC {
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

void AstExprPrinter::accept(CONST_PARAM AstUnaryExpr& ast) CONST_FUNC {
    m_output += ast.getTokenKind().string();
    visit(*ast.getExpr());
}

void AstExprPrinter::accept(CONST_PARAM AstBinaryExpr& ast) CONST_FUNC {
    visit(*ast.getLeft());
    m_output += " ";
    m_output += ast.getTokenKind().string();
    m_output += " ";
    visit(*ast.getRight());
}

// void AstExprPrinter::accept(CONST_PARAM AstCastExpr& ast) CONST_FUNC {
//     visit(*ast.getExpr());
//     m_output += " AS <not-implemented>";
//
// }

void AstExprPrinter::accept(CONST_PARAM AstDereferenceExpr& ast) CONST_FUNC {
    m_output += "*";
    visit(*ast.getExpr());
}

void AstExprPrinter::accept(CONST_PARAM AstAddressOfExpr& ast) CONST_FUNC {
    m_output += "@";
    visit(*ast.getExpr());
}

void AstExprPrinter::accept(CONST_PARAM AstMemberExpr& ast) CONST_FUNC {
    visit(*ast.getExpr());
    m_output += ".";
    visit(*ast.getMember());
}

void AstExprPrinter::accept(CONST_PARAM AstExrSubLeaf& ast) CONST_FUNC {
    (void)this;
    (void)ast;
    m_output += "AstExrSubLeaf";
}
