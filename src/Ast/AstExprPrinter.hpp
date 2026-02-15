//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "pch.hpp"
#include "AstVisitor.hpp"
namespace lbc {

#define CONST_PARAM
#define CONST_FUNC

class AstExprPrinter : private AstExprVisitor<> {
public:
    auto print(CONST_PARAM AstExpr&) CONST_FUNC -> std::string;

private:
    friend AstExprVisitor;

    void accept(const auto& ast) const {
        m_output += ("unhandled " + ast.getClassName()).str();
    }

    void accept(const AstVariableExpr& ast) const; // explicit const
    void accept(CONST_PARAM AstCallExpr& ast) CONST_FUNC;
    void accept(CONST_PARAM AstLiteralExpr& ast) CONST_FUNC;
    void accept(CONST_PARAM AstUnaryExpr& ast) CONST_FUNC;
    void accept(CONST_PARAM AstBinaryExpr& ast) CONST_FUNC;
    // void accept(CONST_PARAM AstCastExpr& ast) CONST_FUNC;
    void accept(CONST_PARAM AstDereferenceExpr& ast) CONST_FUNC;
    void accept(CONST_PARAM AstAddressOfExpr& ast) CONST_FUNC;
    void accept(CONST_PARAM AstMemberExpr& ast) CONST_FUNC;
    void accept(CONST_PARAM AstExrSubLeaf& ast) CONST_FUNC;

    mutable std::string m_output;
};

} // namespace lbc
