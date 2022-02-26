//
// Created by Albert Varaksin on 05/05/2021.
//
#pragma once
#include "Ast/Ast.hpp"
#include "Pass.hpp"

namespace lbc {
class TypeRoot;

namespace Sem {
    /**
     * Fold constant expressions
     */
    class ConstantFoldingPass final : public Pass {
    public:
        using Pass::Pass;
        void fold(AstExpr*& ast);

    private:
        AstExpr* visitUnaryExpr(const AstUnaryExpr& ast);
        AstLiteralExpr::Value unary(TokenKind op, const AstLiteralExpr& ast);
        AstExpr* visitIfExpr(AstIfExpr& ast);
        AstExpr* optimizeIifToCast(AstIfExpr& ast);
        AstExpr* visitBinaryExpr(AstBinaryExpr& ast);
        AstExpr* visitCastExpr(const AstCastExpr& ast);
        AstLiteralExpr::Value cast(const TypeRoot* type, const AstLiteralExpr& ast);
    };
} // namespace Sem
} // namespace lbc
