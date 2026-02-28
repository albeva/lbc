//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "pch.hpp"
#include <llvm/Support/raw_ostream.h>
#include "AstVisitor.hpp"
namespace lbc {

class AstCodePrinter final : AstVisitor<> {
public:
    NO_COPY_AND_MOVE(AstCodePrinter)

    explicit AstCodePrinter(llvm::raw_ostream& output = llvm::outs())
    : m_output(output) {}

    void print(const AstRoot&);

private:
    friend AstVisitor;

    void accept(const AstModule& ast);
    void accept(const AstBuiltInType& ast);
    void accept(const AstPointerType& ast);
    void accept(const AstReferenceType& ast);
    void accept(const AstStmtList& ast);
    void accept(const AstExprStmt& ast);
    void accept(const AstDeclareStmt& ast);
    void accept(const AstFuncStmt& ast);
    void accept(const AstReturnStmt& ast);
    void accept(const AstDimStmt& ast);
    void accept(const AstAssignStmt& ast);
    void accept(const AstIfStmt& ast);
    void accept(const AstVarDecl& ast);
    void accept(const AstFuncDecl& ast);
    void accept(const AstFuncParamDecl& ast);
    void accept(const AstCastExpr& ast);
    void accept(const AstVarExpr& ast);
    void accept(const AstCallExpr& ast);
    void accept(const AstLiteralExpr& ast);
    void accept(const AstUnaryExpr& ast);
    void accept(const AstBinaryExpr& ast);
    void accept(const AstMemberExpr& ast);

    void space();

    void emitType(const auto& node) {
        if (const auto* type = node.getType()) {
            m_output << type->string();
        } else if (const auto* typeExpr = node.getTypeExpr()) {
            visit(*typeExpr);
        } else {
            m_output << "/'<unknown type in " << node.getClassName() << ">'/";
        }
    }

    llvm::raw_ostream& m_output;
    std::size_t m_indent = 0;
};

} // namespace lbc
