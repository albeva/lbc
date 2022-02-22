//
// Created by Albert on 29/05/2021.
//
#pragma once

namespace lbc {
class SemanticAnalyzer;
struct AstModule;
struct AstStmtList;
struct AstTypeDecl;

namespace Sem {
    class TypePass;

    class TypeDeclPass final {
    public:
        NO_COPY_AND_MOVE(TypeDeclPass)

        explicit TypeDeclPass(SemanticAnalyzer& sem) noexcept : m_sem{ sem } {}
        ~TypeDeclPass() noexcept = default;

        void visit(AstModule& ast) noexcept;

    private:
        void visit(AstStmtList& ast) noexcept;
        void visit(AstTypeDecl& ast) noexcept;

        SemanticAnalyzer& m_sem;
    };
} // namespace Sem
} // namespace lbc
