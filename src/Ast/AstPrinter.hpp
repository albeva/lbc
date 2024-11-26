//
// Created by Albert Varaksin on 22/07/2020.
//
#pragma once
#include "pch.hpp"
#include "AstVisitor.hpp"

namespace lbc {

class Context;

class AstPrinter final : public AstVisitor<AstPrinter> {
    NO_COPY_AND_MOVE(AstPrinter)
public:
    AstPrinter(Context& context, llvm::raw_ostream& os);
    ~AstPrinter() = default;

    AST_VISITOR_DECLARE_CONTENT_FUNCS()

private:
    void writeHeader(AstRoot& ast);
    void writeLocation(AstRoot& ast);

    void writeAttributes(AstAttributeList* ast);
    void writeStmts(AstStmtList* ast);
    void writeExpr(AstExpr* ast, llvm::StringRef name = "expr");
    void writeIdent(AstIdentExpr* ast);
    void writeType(AstTypeExpr* ast);

    Context& m_context;
    llvm::json::OStream m_json;
    ControlFlowStack<> m_controlStack{};
};

} // namespace lbc
