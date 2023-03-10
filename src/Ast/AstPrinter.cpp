//
// Created by Albert Varaksin on 22/07/2020.
//
#include "AstPrinter.hpp"
#include "Ast.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Token.hpp"
#include "Type/Type.hpp"
using namespace lbc;

AstPrinter::AstPrinter(Context& context, llvm::raw_ostream& os)
: m_context{ context }, m_json{ os, 4 } {
}

void AstPrinter::visit(AstModule& ast) {
    m_json.object([&] {
        writeHeader(ast);
        if (not ast.imports.empty()) {
            m_json.attributeBegin("imports");
            m_json.array([&] {
                for (auto* import : ast.imports) {
                    visit(*import);
                }
            });
            m_json.attributeEnd();
        }
        writeStmts(ast.stmtList);
    });
}

void AstPrinter::visit(AstStmtList& ast) {
    m_json.array([&] {
        for (auto* decl : ast.decl) {
            if (auto* func = llvm::dyn_cast<AstFuncDecl>(decl)) {
                if (not func->hasImpl) {
                    visit(*func);
                }
            }
        }
        for (auto* stmt : ast.stmts) {
            visit(*stmt);
        }
        for (auto* func : ast.funcs) {
            visit(*func);
        }
    });
}

void AstPrinter::visit(AstImport& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attribute("import", ast.import);
    });
}

void AstPrinter::visit(AstExprList& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attributeArray("exprs", [&] {
            for (const auto& expr : ast.exprs) {
                visit(*expr);
            }
        });
    });
}

void AstPrinter::visit(AstAssignExpr& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeExpr(ast.lhs, "lhs");
        writeExpr(ast.rhs, "rhs");
    });
}

void AstPrinter::visit(AstExprStmt& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeExpr(ast.expr);
    });
}

void AstPrinter::visit(AstDeclList& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attributeArray("decls", [&] {
            for (const auto& decl : ast.decls) {
                visit(*decl);
            }
        });
    });
}

void AstPrinter::visit(AstVarDecl& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeAttributes(ast.attributes);
        m_json.attribute("id", ast.name);
        writeType(ast.typeExpr);
        writeExpr(ast.expr);
    });
}

void AstPrinter::visit(AstFuncDecl& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attribute("id", ast.name);
        writeAttributes(ast.attributes);

        if (ast.params != nullptr) {
            m_json.attributeArray("params", [&] {
                for (const auto& param : ast.params->params) {
                    visit(*param);
                }
            });
        }

        writeType(ast.retTypeExpr);
    });
}

void AstPrinter::visit(AstFuncParamDecl& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeAttributes(ast.attributes);
        m_json.attribute("id", ast.name);
        writeType(ast.typeExpr);
    });
}

void AstPrinter::visit(AstFuncStmt& ast) {
    m_json.object([&] {
        writeHeader(ast);

        m_json.attributeBegin("decl");
        visit(*ast.decl);
        m_json.attributeEnd();

        writeStmts(ast.stmtList);
    });
}

void AstPrinter::visit(AstReturnStmt& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeExpr(ast.expr);
    });
}

//----------------------------------------
// Type (user defined)
//----------------------------------------

void AstPrinter::visit(AstUdtDecl& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeAttributes(ast.attributes);
        m_json.attribute("id", ast.name);

        m_json.attributeBegin("members");
        visit(*ast.decls);
        m_json.attributeEnd();
    });
}

//----------------------------------------
// Type alias
//----------------------------------------

void AstPrinter::visit(AstTypeAlias& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeAttributes(ast.attributes);
        m_json.attribute("id", ast.name);

        m_json.attributeBegin("type");
        visit(*ast.typeExpr);
        m_json.attributeEnd();
    });
}

//----------------------------------------
// IF statement
//----------------------------------------

void AstPrinter::visit(AstIfStmt& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attributeArray("blocks", [&] {
            for (const auto& block : ast.blocks) {
                m_json.object([&] {
                    if (!block->decls.empty()) {
                        m_json.attributeArray("decls", [&] {
                            for (const auto& decl : block->decls) {
                                visit(*decl);
                            }
                        });
                    }

                    writeExpr(block->expr);

                    if (auto* list = llvm::dyn_cast<AstStmtList>(block->stmt)) {
                        writeStmts(list);
                    } else {
                        m_json.attributeBegin("stmt");
                        visit(*block->stmt);
                        m_json.attributeEnd();
                    }
                });
            }
        });
    });
}

void AstPrinter::visit(AstForStmt& ast) {
    m_json.object([&] {
        writeHeader(ast);
        if (!ast.decls.empty()) {
            m_json.attributeArray("decls", [&] {
                for (const auto& decl : ast.decls) {
                    visit(*decl);
                }
            });
        }

        m_json.attributeBegin("iter");
        visit(*ast.iterator);
        m_json.attributeEnd();
        writeExpr(ast.limit, "limit");
        writeExpr(ast.step, "step");

        if (auto* list = llvm::dyn_cast<AstStmtList>(ast.stmt)) {
            writeStmts(list);
        } else {
            m_json.attributeBegin("stmt");
            visit(*ast.stmt);
            m_json.attributeEnd();
        }

        if (!ast.next.empty()) {
            m_json.attribute("next", ast.next);
        }
    });
}

void AstPrinter::visit(AstDoLoopStmt& ast) {
    m_json.object([&] {
        writeHeader(ast);

        switch (ast.condition) {
        case AstDoLoopStmt::Condition::None:
            break;
        case AstDoLoopStmt::Condition::PreWhile:
            m_json.attribute("makeCondition", "PreWhile");
            break;
        case AstDoLoopStmt::Condition::PreUntil:
            m_json.attribute("makeCondition", "PreUntil");
            break;
        case AstDoLoopStmt::Condition::PostWhile:
            m_json.attribute("makeCondition", "PostWhile");
            break;
        case AstDoLoopStmt::Condition::PostUntil:
            m_json.attribute("makeCondition", "PostUntil");
            break;
        }
        writeExpr(ast.expr);

        if (auto* list = llvm::dyn_cast<AstStmtList>(ast.stmt)) {
            writeStmts(list);
        } else {
            m_json.attributeBegin("stmt");
            visit(*ast.stmt);
            m_json.attributeEnd();
        }
    });
}

void AstPrinter::visit(AstContinuationStmt& ast) {
    m_json.object([&] {
        writeHeader(ast);

        m_json.attributeBegin("op");
        switch (ast.action) {
        case AstContinuationStmt::Action::Exit:
            m_json.value("EXIT");
            break;
        case AstContinuationStmt::Action::Continue:
            m_json.value("CONTINUE");
            break;
        }
        m_json.attributeEnd();

        if (!ast.destination.empty()) {
            m_json.attributeArray("dest", [&] {
                for (auto target : ast.destination) {
                    switch (target) {
                    case ControlFlowStatement::For:
                        m_json.value("FOR");
                        continue;
                    case ControlFlowStatement::Do:
                        m_json.value("DO");
                        continue;
                    }
                }
            });
        }
    });
}

void AstPrinter::visit(AstAttributeList& ast) {
    m_json.array([&] {
        for (const auto& attr : ast.attribs) {
            visit(*attr);
        }
    });
}

void AstPrinter::visit(AstAttribute& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeIdent(ast.identExpr);
        m_json.attributeBegin("args");
        visit(*ast.args);
        m_json.attributeEnd();
    });
}

void AstPrinter::visit(AstTypeExpr& ast) {
    m_json.object([&] {
        writeHeader(ast);
        std::string name;
        if (const auto* type = ast.type) {
            name = type->asString();
        } else {
            static constexpr auto visitor = Visitor{
                [](AstIdentExpr* ident) -> llvm::StringRef {
                    return ident->name;
                },
                [](AstFuncDecl* /* decl */) -> llvm::StringRef {
                    return "PROC PTR (not implemented)";
                },
                [](AstTypeOf* /* ast */) -> llvm::StringRef {
                    return "TYPEOF (not implemented)";
                },
                [](TokenKind kind) -> llvm::StringRef {
                    return Token::description(kind);
                }
            };
            name = std::visit(visitor, ast.expr);

            for (int i = 0; i < ast.dereference; i++) {
                name += " PTR";
            }
        }
        m_json.attribute("id", name);
    });
}

void AstPrinter::visit(AstTypeOf& ast) {
    m_json.object([&] {
        writeHeader(ast);
        const auto visitor = Visitor{
            [&](std::vector<Token>& tokens) {
                m_json.attributeArray("tokens", [&]() {
                    for (auto& tkn : tokens) {
                        m_json.value(tkn.lexeme());
                    }
                });
            },
            [&](AstTypeExpr* typeExpr) {
                writeType(typeExpr);
            },
            [&](AstExpr* expr) {
                writeExpr(expr);
            }
        };
        std::visit(visitor, ast.typeExpr);
    });
}

void AstPrinter::visit(AstIdentExpr& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attribute("id", ast.name);
    });
}

void AstPrinter::visit(AstCallExpr& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeExpr(ast.callable, "callable");

        m_json.attributeBegin("args");
        visit(*ast.args);
        m_json.attributeEnd();
    });
}

void AstPrinter::visit(AstLiteralExpr& ast) {
    using Ret = std::pair<TokenKind, std::string>;
    const auto visitor = Visitor{
        [](std::monostate /*value*/) -> Ret {
            return { TokenKind::NullLiteral, "null" };
        },
        [](llvm::StringRef value) -> Ret {
            return { TokenKind::StringLiteral, value.str() };
        },
        [&](uint64_t value) -> Ret {
            const auto* type = ast.type;
            if (type == nullptr || type->isSignedIntegral()) {
                auto sval = static_cast<int64_t>(value);
                return { TokenKind::IntegerLiteral, std::to_string(sval) };
            }
            return { TokenKind::IntegerLiteral, std::to_string(value) };
        },
        [](double value) -> Ret {
            return { TokenKind::FloatingPointLiteral, std::to_string(value) };
        },
        [](bool value) -> Ret {
            return { TokenKind::BooleanLiteral, value ? "TRUE" : "FALSE" };
        }
    };

    m_json.object([&] {
        writeHeader(ast);
        auto [kind, value] = std::visit(visitor, ast.value);
        m_json.attribute("kind", Token::description(kind));
        m_json.attribute("value", value);
        if (ast.type != nullptr) {
            m_json.attribute("type", ast.type->asString());
        }
    });
}

void AstPrinter::visit(AstUnaryExpr& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attribute("op", Token::description(ast.tokenKind));
        writeExpr(ast.expr);
    });
}

void AstPrinter::visit(AstDereference& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeExpr(ast.expr);
    });
}

void AstPrinter::visit(AstAddressOf& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeExpr(ast.expr);
    });
}

void AstPrinter::visit(AstMemberAccess& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attributeArray("exprs", [&]() {
            for (auto& expr : ast.exprs) {
                visit(*expr);
            }
        });
    });
}

void AstPrinter::visit(AstBinaryExpr& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attribute("op", Token::description(ast.tokenKind));
        writeExpr(ast.lhs, "lhs");
        writeExpr(ast.rhs, "rhs");
    });
}

void AstPrinter::visit(AstCastExpr& ast) {
    m_json.object([&] {
        writeHeader(ast);
        m_json.attribute("implicit", ast.implicit);
        writeType(ast.typeExpr);
        writeExpr(ast.expr);
    });
}

void AstPrinter::visit(AstIfExpr& ast) {
    m_json.object([&] {
        writeHeader(ast);
        writeExpr(ast.expr, "expr");
        writeExpr(ast.trueExpr, "true");
        writeExpr(ast.falseExpr, "false");
    });
}

void AstPrinter::writeHeader(AstRoot& ast) {
    m_json.attribute("class", ast.getClassName());
    m_json.attributeBegin("loc");
    writeLocation(ast);
    m_json.attributeEnd();
}

void AstPrinter::writeLocation(AstRoot& ast) {
    auto [startLine, startCol] = m_context.getSourceMrg().getLineAndColumn(ast.range.Start);
    auto [endLine, endCol] = m_context.getSourceMrg().getLineAndColumn(ast.range.End);

    if (startLine == endLine) {
        m_json.value(llvm::formatv("{0}:{1} - {2}", startLine, startCol, endCol));
    } else {
        m_json.value(llvm::formatv("{0}:{1} - {2}:3", startLine, startCol, endLine, endCol));
    }
}

void AstPrinter::writeAttributes(AstAttributeList* ast) {
    if (ast == nullptr || ast->attribs.empty()) {
        return;
    }
    m_json.attributeBegin("attrs");
    visit(*ast);
    m_json.attributeEnd();
}

void AstPrinter::writeStmts(AstStmtList* ast) {
    if (ast == nullptr || ast->stmts.empty()) {
        return;
    }
    m_json.attributeBegin("stmts");
    visit(*ast);
    m_json.attributeEnd();
}

void AstPrinter::writeExpr(AstExpr* ast, llvm::StringRef name) {
    if (ast == nullptr) {
        return;
    }
    m_json.attributeBegin(name);
    visit(*ast);
    m_json.attributeEnd();
}

void AstPrinter::writeIdent(AstIdentExpr* ast) {
    if (ast == nullptr) {
        return;
    }
    m_json.attributeBegin("ident");
    visit(*ast);
    m_json.attributeEnd();
}

void AstPrinter::writeType(AstTypeExpr* ast) {
    if (ast == nullptr) {
        return;
    }
    m_json.attributeBegin("type");
    visit(*ast);
    m_json.attributeEnd();
}
