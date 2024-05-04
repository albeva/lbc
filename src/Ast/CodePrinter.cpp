////
//// Created by Albert Varaksin on 05/07/2020.
////
#include "CodePrinter.hpp"
#include "Ast.hpp"
#include "Lexer/Token.hpp"
#include "Type/Type.hpp"
using namespace lbc;

void CodePrinter::visit(AstModule& ast) {
    for (auto* import : ast.imports) {
        visit(*import);
        m_os << '\n';
    }
    visit(*ast.stmtList);
}

// Statements

void CodePrinter::visit(AstStmtList& ast) {
    for (auto* decl : ast.decl) {
        if (auto* func = llvm::dyn_cast<AstFuncDecl>(decl)) {
            if (not func->hasImpl) {
                visit(*func);
                m_os << '\n';
            }
        }
    }

    for (auto* stmt : ast.stmts) {
        if (stmt->kind == AstKind::FuncDecl) {
            continue;
        }
        visit(*stmt);
        m_os << '\n';
    }

    for (auto* func : ast.funcs) {
        visit(*func);
        m_os << '\n';
    }
}

void CodePrinter::visit(AstImport& ast) {
    m_os << indent() << "IMPORT " << ast.import;
}

void CodePrinter::visit(AstExprList& ast) {
    bool isFirst = true;
    for (const auto& expr : ast.exprs) {
        if (isFirst) {
            isFirst = false;
        } else {
            m_os << ", ";
        }
        visit(*expr);
    }
}

void CodePrinter::visit(AstAssignExpr& ast) {
    m_os << indent();
    visit(*ast.lhs);
    m_os << " = ";
    visit(*ast.rhs);
}

void CodePrinter::visit(AstExprStmt& ast) {
    m_os << indent();
    visit(*ast.expr);
}

// Attributes

void CodePrinter::visit(AstAttributeList& ast) {
    m_os << indent();
    m_os << '[';
    bool isFirst = true;
    for (const auto& attr : ast.attribs) {
        if (isFirst) {
            isFirst = false;
        } else {
            m_os << ", ";
        }
        visit(*attr);
    }
    m_os << "]";
}

void CodePrinter::visit(AstAttribute& ast) {
    visit(*ast.identExpr);
    if (ast.args->exprs.size() == 1) {
        m_os << " = ";
        visit(*ast.args);
    } else if (ast.args->exprs.size() > 1) {
        m_os << "(";
        visit(*ast.args);
        m_os << ")";
    }
}

// Types

void CodePrinter::visit(AstTypeExpr& ast) {
    if (const auto* type = ast.type) {
        m_os << type->asString();
        return;
    }

    const auto visitor = Visitor{
        [&](AstIdentExpr* ident) {
            visit(*ident);
        },
        [&](AstFuncDecl* decl) {
            visit(*decl);
        },
        [&](AstTypeOf* typ) {
            visit(*typ);
        },
        [&](TokenKind kind) {
            m_os << Token::description(kind);
        }
    };
    std::visit(visitor, ast.expr);

    for (int i = 0; i < ast.dereference; i++) {
        m_os << " PTR";
    }
}

void CodePrinter::visit(AstTypeOf& ast) {
    m_os << "TYPEOF(";
    const auto visitor = Visitor{
        [&](llvm::SMRange /* range */) {
            // TODO: re-lex the input and emit it
            m_os << " /' emitting SMRange not implemented '/ ";
        },
        [&](AstTypeExpr* typeExpr) {
            visit(*typeExpr);
        },
        [&](AstExpr* expr) {
            visit(*expr);
        }
    };
    std::visit(visitor, ast.typeExpr);
    m_os << ")";
}

// Declarations

void CodePrinter::visit(AstDeclList& ast) {
    for (const auto& decl : ast.decls) {
        visit(*decl);
        m_os << '\n';
    }
}

void CodePrinter::visit(AstVarDecl& ast) {
    if (ast.attributes != nullptr) {
        visit(*ast.attributes);
        m_os << " _" << '\n';
    }

    if (ast.callingConv == CallingConv::C) {
        m_os << "EXTERN \"C\" ";
    }

    m_os << indent();
    if (m_emitDimKeyword) {
        m_os << "DIM ";
    }
    m_os << ast.name;

    if (ast.typeExpr != nullptr) {
        m_os << " AS ";
        visit(*ast.typeExpr);
    }

    if (ast.expr != nullptr) {
        m_os << " = ";
        visit(*ast.expr);
    }
}

void CodePrinter::visit(AstFuncDecl& ast) {
    if (ast.attributes != nullptr) {
        visit(*ast.attributes);
        m_os << " _" << '\n';
    }

    m_os << indent();

    if (ast.callingConv == CallingConv::C) {
        m_os << "EXTERN \"C\" ";
    }

    if (!ast.hasImpl) {
        m_os << "DECLARE ";
    }

    if (ast.retTypeExpr != nullptr) {
        m_os << "FUNCTION ";
    } else {
        m_os << "SUB ";
    }
    m_os << ast.name;

    if (ast.params != nullptr) {
        m_os << "(";
        bool isFirst = true;
        for (const auto& param : ast.params->params) {
            if (isFirst) {
                isFirst = false;
            } else {
                m_os << ", ";
            }
            visit(*param);
        }
        m_os << ")";
    }

    if (ast.retTypeExpr != nullptr) {
        m_os << " AS ";
        visit(*ast.retTypeExpr);
    }
}

void CodePrinter::visit(AstFuncParamDecl& ast) {
    m_os << ast.name;
    m_os << " AS ";
    visit(*ast.typeExpr);
}

void CodePrinter::visit(AstFuncStmt& ast) {
    visit(*ast.decl);
    m_os << '\n';
    m_indent++;
    visit(*ast.stmtList);
    m_indent--;

    m_os << indent();
    m_os << "END " << (ast.decl->retTypeExpr != nullptr ? "FUNCTION" : "SUB");
}

void CodePrinter::visit(AstReturnStmt& ast) {
    m_os << indent() << "RETURN";
    if (ast.expr != nullptr) {
        m_os << " ";
        visit(*ast.expr);
    }
}

//----------------------------------------
// Type (user defined)
//----------------------------------------

void CodePrinter::visit(AstUdtDecl& ast) {
    RESTORE_ON_EXIT(m_emitDimKeyword);
    m_emitDimKeyword = false;

    if (ast.attributes != nullptr) {
        visit(*ast.attributes);
        m_os << " _" << '\n';
    }

    if (ast.callingConv == CallingConv::C) {
        m_os << "EXTERN \"C\" ";
    }

    m_os << indent() << "TYPE " << ast.name << '\n';
    if (ast.decls != nullptr) {
        m_indent++;
        visit(*ast.decls);
        m_indent--;
    }
    m_os << indent() << "END TYPE";
}

//----------------------------------------
// Type (user defined)
//----------------------------------------

void CodePrinter::visit(AstTypeAlias& ast) {
    if (ast.attributes != nullptr) {
        visit(*ast.attributes);
        m_os << " _" << '\n';
    }

    m_os << indent() << "TYPE " << ast.name << " = ";
    visit(*ast.typeExpr);
}

//----------------------------------------
// IF statement
//----------------------------------------

void CodePrinter::visit(AstIfStmt& ast) {
    bool isFirst = true;
    for (const auto& block : ast.blocks) {
        m_os << indent();
        if (!isFirst) {
            m_os << "ELSE";
        }
        if (block->expr) {
            RESTORE_ON_EXIT(m_indent);
            m_indent = 0;
            if (!isFirst) {
                m_os << " ";
            }
            m_os << "IF ";
            for (const auto& var : block->decls) {
                visit(*var);
                m_os << ", ";
            }
            visit(*block->expr);
            m_os << " THEN\n";
        } else {
            m_os << "\n";
        }
        m_indent++;
        visit(*block->stmt);
        if (block->stmt->kind != AstKind::StmtList) {
            m_os << '\n';
        }
        m_indent--;
        isFirst = false;
    }
    m_os << indent() << "END IF";
}

void CodePrinter::visit(AstForStmt& ast) {
    m_os << indent() << "FOR ";

    for (const auto& decl : ast.decls) {
        visit(*decl);
        m_os << ", ";
    }

    m_os << ast.iterator->name;
    if (ast.iterator->typeExpr) {
        m_os << " AS ";
        visit(*ast.iterator->typeExpr);
    }

    m_os << " = ";
    visit(*ast.iterator->expr);
    m_os << " TO ";
    visit(*ast.limit);
    if (ast.step) {
        m_os << " STEP ";
        visit(*ast.step);
    }

    if (ast.stmt->kind == AstKind::StmtList) {
        m_os << '\n';
        m_indent++;
        m_controlStack.with(ControlFlowStatement::For, [&] {
            visit(*ast.stmt);
        });
        m_indent--;
        m_os << indent() << "NEXT";
        if (!ast.next.empty()) {
            m_os << " " << ast.next;
        }
    } else {
        m_os << " DO ";
        m_controlStack.with(ControlFlowStatement::For, [&] {
            visit(*ast.stmt);
        });
    }
}

void CodePrinter::visit(AstDoLoopStmt& ast) {
    m_os << indent() << "DO";

    if (!ast.decls.empty()) {
        bool first = true;
        for (const auto& decl : ast.decls) {
            if (first) {
                first = false;
                m_os << " ";
            } else {
                m_os << ", ";
            }
            visit(*decl);
        }
    }

    if (ast.condition == AstDoLoopStmt::Condition::PreWhile) {
        m_os << " WHILE ";
        visit(*ast.expr);
    } else if (ast.condition == AstDoLoopStmt::Condition::PreUntil) {
        m_os << " UNTIL ";
        visit(*ast.expr);
    }

    if (ast.stmt->kind == AstKind::StmtList) {
        m_os << "\n";
        m_indent++;
        m_controlStack.with(ControlFlowStatement::Do, [&] {
            visit(*ast.stmt);
        });
        m_indent--;
        m_os << indent() << "LOOP";

        if (ast.condition == AstDoLoopStmt::Condition::PostWhile) {
            m_os << " WHILE ";
            visit(*ast.expr);
        } else if (ast.condition == AstDoLoopStmt::Condition::PostUntil) {
            m_os << " UNTIL ";
            visit(*ast.expr);
        }
    } else {
        m_os << " DO ";
        m_controlStack.with(ControlFlowStatement::Do, [&] {
            visit(*ast.stmt);
        });
    }
}

void CodePrinter::visit(AstContinuationStmt& ast) {
    m_os << indent();
    switch (ast.action) {
    case AstContinuationAction::Continue:
        m_os << "CONTINUE";
        break;
    case AstContinuationAction::Exit:
        m_os << "EXIT";
        break;
    }

    std::for_each(m_controlStack.begin(), m_controlStack.after(ast.destination), [&](const auto& entry) {
        switch (entry.first) {
        case ControlFlowStatement::For:
            m_os << " FOR";
            break;
        case ControlFlowStatement::Do:
            m_os << " DO";
            break;
        }
    });
}

// Expressions

void CodePrinter::visit(AstIdentExpr& ast) {
    m_os << ast.name;
}

void CodePrinter::visit(AstCallExpr& ast) {
    visit(*ast.callable);
    m_os << "(";
    visit(*ast.args);
    m_os << ")";
}

void CodePrinter::visit(AstLiteralExpr& ast) {
    const auto visitor = Visitor{
        [](std::monostate /*value*/) -> std::string {
            return "NULL";
        },
        [](llvm::StringRef value) -> std::string {
            std::string result;
            llvm::raw_string_ostream str{ result };
            llvm::printEscapedString(value, str);
            return '"' + result + '"';
        },
        [&](uint64_t value) -> std::string {
            const auto* type = ast.type;
            if (type == nullptr || type->isSignedIntegral()) {
                auto sval = static_cast<int64_t>(value);
                return std::to_string(sval);
            }
            return std::to_string(value);
        },
        [](double value) -> std::string {
            return std::to_string(value);
        },
        [](bool value) -> std::string {
            return value ? "TRUE" : "FALSE";
        }
    };

    m_os << std::visit(visitor, ast.value);
}

void CodePrinter::visit(AstUnaryExpr& ast) {
    m_os << "(";
    Token token;
    token.set(ast.tokenKind, ast.range);
    if (token.isRightToLeft()) {
        visit(*ast.expr);
        m_os << " " << token.description();
    } else {
        m_os << token.description() << " ";
        visit(*ast.expr);
    }
    m_os << ")";
}

void CodePrinter::visit(AstDereference& ast) {
    m_os << "*(";
    visit(*ast.expr);
    m_os << ")";
}

void CodePrinter::visit(AstAddressOf& ast) {
    m_os << "@(";
    visit(*ast.expr);
    m_os << ")";
}

void CodePrinter::visit(AstMemberAccess& ast) {
    for (size_t i = 0; i < ast.exprs.size(); i++) {
        if (i != 0) {
            m_os << '.';
        }
        visit(*ast.exprs[i].second);
    }
}

void CodePrinter::visit(AstBinaryExpr& ast) {
    m_os << "(";
    visit(*ast.lhs);

    Token token;
    token.set(ast.tokenKind, ast.range);
    m_os << " " << token.description() << " ";

    visit(*ast.rhs);
    m_os << ")";
}

void CodePrinter::visit(AstCastExpr& ast) {
    m_os << "(";
    visit(*ast.expr);
    m_os << " AS ";
    if (ast.implicit) {
        const auto* type = ast.type;
        if (type != nullptr) {
            m_os << type->asString();
        } else {
            m_os << "ANY";
        }
        m_os << " /' implicit '/";
    } else {
        visit(*ast.typeExpr);
    }
    m_os << ")";
}

void CodePrinter::visit(AstIfExpr& ast) {
    m_os << "(IF ";
    visit(*ast.expr);
    m_os << " THEN ";
    visit(*ast.trueExpr);
    m_os << " ELSE ";
    visit(*ast.falseExpr);
    m_os << ")";
}

std::string CodePrinter::indent() const {
    return std::string(m_indent * SPACES, ' ');
}
