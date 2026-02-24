//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstCodePrinter.hpp"
#include "Lexer/TokenKind.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace std::string_literals;

void AstCodePrinter::print(const AstRoot& ast) {
    visit(ast);
}

void AstCodePrinter::accept(const AstModule& ast) {
    accept(*ast.getStmtList());
}

void AstCodePrinter::accept(const AstBuiltInType& ast) {
    if (const auto* type = ast.getType()) {
        m_output << type->string();
    } else {
        m_output << ast.getTokenKind().string();
    }
}

void AstCodePrinter::accept(const AstPointerType& ast) {
    emitType(ast);
}

void AstCodePrinter::accept(const AstReferenceType& ast) {
    emitType(ast);
}

void AstCodePrinter::accept(const AstStmtList& ast) {
    for (const auto* stmt : ast.getStmts()) {
        space();
        visit(*stmt);
        m_output << '\n';
    }
}

void AstCodePrinter::accept(const AstExprStmt& ast) {
    visit(*ast.getExpr());
}

void AstCodePrinter::accept(const AstDeclareStmt& ast) {
    m_output << "DECLARE ";
    visit(*ast.getDecl());
}

void AstCodePrinter::accept(const AstFuncStmt& ast) {
    visit(*ast.getDecl());
    m_output << "\n";
    m_indent++;
    accept(*ast.getStmtList());
    m_indent--;
    m_output << "END "s + (ast.getDecl()->getRetTypeExpr() == nullptr ? "SUB" : "FUNCTION");
}

void AstCodePrinter::accept(const AstReturnStmt& ast) {
    m_output << "RETURN";
    if (const auto* expr = ast.getExpr()) {
        m_output << " ";
        visit(*expr);
    }
}

void AstCodePrinter::accept(const AstDimStmt& ast) {
    m_output << "DIM ";
    bool first = true;
    for (const auto* var : ast.getDecls()) {
        if (first) {
            first = false;
        } else {
            m_output << ", ";
        }
        accept(*var);
    }
}

void AstCodePrinter::accept(const AstAssignStmt& ast) {
    visit(*ast.getAssignee());
    m_output << " = ";
    visit(*ast.getExpr());
}

void AstCodePrinter::accept(const AstIfStmt& ast) {
    m_output << "IF ";
    visit(*ast.getCondition());
    m_output << " THEN\n";
    m_indent += 1;
    visit(*ast.getThenStmt());
    m_indent -= 1;

    if (const auto* elseStmt = ast.getElseStmt()) {
        m_output << "ELSE";
        if (const auto* elseIfStmt = llvm::dyn_cast<AstIfStmt>(elseStmt)) {
            m_output << " ";
            visit(*elseIfStmt);
            return;
        }
        m_output << '\n';
        m_indent += 1;
        visit(*elseStmt);
        m_indent -= 1;
    }
    m_output << "END IF";
}

void AstCodePrinter::accept(const AstVarDecl& ast) {
    m_output << ast.getName();
    m_output << " AS ";
    emitType(ast);
    if (const auto* expr = ast.getExpr()) {
        m_output << " = ";
        visit(*expr);
    }
}

void AstCodePrinter::accept(const AstFuncDecl& ast) {
    const bool isSub = ast.getRetTypeExpr() == nullptr;
    m_output << (isSub ? "SUB " : "FUNCTION ");

    if (not ast.getName().empty()) {
        m_output << " ";
        m_output << ast.getName();
    }

    m_output << "(";
    bool first = true;
    for (const auto* param : ast.getParams()) {
        if (first) {
            first = false;
        } else {
            m_output << ", ";
        }
        accept(*param);
    }
    m_output << ")";

    if (!isSub) {
        m_output << " A ";
        visit(*ast.getRetTypeExpr());
    }
}

void AstCodePrinter::accept(const AstFuncParamDecl& ast) {
    m_output << ast.getName();
    m_output << " AS ";
    emitType(ast);
}

void AstCodePrinter::accept(const AstCastExpr& ast) {
    m_output << "(";
    visit(*ast.getExpr());
    m_output << " AS ";
    if (ast.getImplicit()) {
        m_output << "/'implicit'/";
    }
    emitType(ast);
    m_output << ")";
}

void AstCodePrinter::accept(const AstVarExpr& ast) {
    m_output << ast.getName();
}

void AstCodePrinter::accept(const AstCallExpr& ast) {
    visit(*ast.getCallee());
    m_output << "(";
    bool first = true;
    for (const auto& arg : ast.getArgs()) {
        if (first) {
            first = false;
        } else {
            m_output << ", ";
        }
        visit(*arg);
    }
    m_output << ")";
}

void AstCodePrinter::accept(const AstLiteralExpr& ast) {
    const auto visitor = Visitor {
        [&](const std::monostate& /* value */) {
            m_output << "null";
        },
        [&](const double& value) {
            m_output << std::to_string(value);
        },
        [&](const std::uint64_t& value) {
            m_output << std::to_string(value);
        },
        [&](const bool& value) {
            m_output << (value ? "true" : "false");
        },
        [&](const llvm::StringRef& value) {
            m_output << '\"';
            for (const char ch : value) {
                switch (ch) {
                case '"':
                    m_output << "\"";
                    break;
                case '\n':
                    m_output << "\\n";
                    break;
                case '\r':
                    m_output << "\\r";
                    break;
                case '\0':
                    m_output << "\\0";
                    break;
                default:
                    m_output << ch;
                    break;
                }
            }
            m_output << '\"';
        }
    };
    std::visit(visitor, ast.getValue().storage());
}

void AstCodePrinter::accept(const AstUnaryExpr& ast) {
    m_output << "(";
    m_output << ast.getOp().string();
    visit(*ast.getExpr());
    m_output << ")";
}

void AstCodePrinter::accept(const AstBinaryExpr& ast) {
    m_output << "(";
    visit(*ast.getLeft());
    m_output << " ";
    m_output << ast.getOp().string();
    m_output << " ";
    visit(*ast.getRight());
    m_output << ")";
}

void AstCodePrinter::accept(const AstMemberExpr& ast) {
    visit(*ast.getLeft());
    m_output << ".";
    visit(*ast.getRight());
}

void AstCodePrinter::space() {
    m_output << std::string(m_indent * 4, ' ');
}
