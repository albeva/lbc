//
// Created by Albert Varaksin on 05/05/2021.
//
#pragma once
#include "pch.hpp"
#include "Ast/Ast.hpp"
#include "Pass.hpp"

namespace lbc {
class TypeRoot;

namespace Sem {
    class ConstantFoldingPass final : public Pass {
    public:
        using Pass::Pass;
        void fold(AstExpr*& ast);

    private:
        auto visitUnaryExpr(const AstUnaryExpr& ast) -> AstExpr*;
        auto unary(TokenKind op, const AstLiteralExpr& ast) -> AstLiteralExpr::Value;
        auto visitIfExpr(AstIfExpr& ast) -> AstExpr*;
        auto optimizeIifToCast(AstIfExpr& ast) -> AstExpr*;
        auto visitBinaryExpr(AstBinaryExpr& ast) -> AstExpr*;
        auto visitCastExpr(const AstCastExpr& ast) -> AstExpr*;
        auto cast(const TypeRoot* type, const AstLiteralExpr& ast) -> AstLiteralExpr::Value;
    };
} // namespace Sem
} // namespace lbc
