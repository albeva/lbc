//
// Created by Albert on 29/05/2021.
//
#pragma once

namespace lbc {
class SemanticAnalyzer;
struct AstModule;
struct AstStmtList;
struct AstUdtDecl;

namespace Sem {
    class TypePass;

    class UdtDeclPass final {
    public:
        NO_COPY_AND_MOVE(UdtDeclPass)

        explicit UdtDeclPass(SemanticAnalyzer& sem) noexcept : m_sem{ sem } {}
        ~UdtDeclPass() noexcept = default;

        void visit(AstModule& ast) noexcept;

    private:
        void visit(AstStmtList& ast) noexcept;
        void visit(AstUdtDecl& ast) noexcept;

        SemanticAnalyzer& m_sem;
    };
} // namespace Sem
} // namespace lbc
