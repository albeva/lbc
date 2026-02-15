//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "pch.hpp"
#include "AstVisitor.hpp"
namespace lbc {

class AstExprPrinter final : AstExprVisitor<> {
public:
    auto print(const AstExpr&) -> std::string;

private:
    friend AstExprVisitor;

    void accept(const auto& ast) {
        m_output += ("unhandled " + ast.getClassName()).str();
    }

    void accept(const AstVarExpr& ast);
    void accept(const AstCallExpr& ast);
    void accept(const AstLiteralExpr& ast);
    void accept(const AstUnaryExpr& ast);
    void accept(const AstBinaryExpr& ast);

    std::string m_output;
};

} // namespace lbc
