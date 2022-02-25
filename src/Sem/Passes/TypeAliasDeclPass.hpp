//
// Created by Albert on 25/02/2022.
//
#pragma once
namespace lbc {
class SemanticAnalyzer;
struct AstModule;
struct AstTypeAlias;

namespace Sem {
    class TypeAliasDeclPass final {
    public:
        NO_COPY_AND_MOVE(TypeAliasDeclPass)
        explicit TypeAliasDeclPass(SemanticAnalyzer& sem) noexcept : m_sem{ sem } {}
        ~TypeAliasDeclPass() noexcept = default;

        void visit(AstModule& ast) const noexcept;

    private:
        void visit(AstTypeAlias& ast) const noexcept;

        SemanticAnalyzer& m_sem;
    };
} // namespace Sem

} // namespace lbc