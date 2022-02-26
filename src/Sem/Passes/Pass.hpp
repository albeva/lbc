//
// Created by Albert on 26/02/2022.
//
#pragma once
namespace lbc {
class SemanticAnalyzer;
struct AstModule;

namespace Sem {
    /**
     * Forward declare all user defined types, aliases and precedures
     */
    class Pass {
    public:
        NO_COPY_AND_MOVE(Pass)
        explicit Pass(SemanticAnalyzer& sem) noexcept : m_sem{ sem } {}
        ~Pass() noexcept = default;

    protected:
        SemanticAnalyzer& m_sem;
    };
} // namespace Sem
} // namespace lbc
