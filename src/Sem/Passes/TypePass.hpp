//
// Created by Albert Varaksin on 22/05/2021.
//
#pragma once


namespace lbc {
class SemanticAnalyzer;
struct AstTypeExpr;
struct AstIdentExpr;
struct AstFuncDecl;

namespace Sem {
    class TypePass final {
    public:
        NO_COPY_AND_MOVE(TypePass)

        explicit TypePass(SemanticAnalyzer& sem) noexcept : m_sem{ sem } {}
        ~TypePass() noexcept = default;

        void visit(AstTypeExpr& ast) const noexcept;

    private:
        void visit(AstIdentExpr& ast) const noexcept;
        void visit(AstFuncDecl& ast) const noexcept;

        SemanticAnalyzer& m_sem;
    };
} // namespace Sem
} // namespace lbc
