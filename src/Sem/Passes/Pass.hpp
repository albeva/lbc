//
// Created by Albert on 26/02/2022.
//
#pragma once
namespace lbc {
class SemanticAnalyzer;

namespace Sem {
    class Pass {
    public:
        NO_COPY_AND_MOVE(Pass)
        explicit Pass(SemanticAnalyzer& sem) noexcept : m_sem{ sem } {}
        virtual ~Pass() noexcept = default;

    protected:
        SemanticAnalyzer& m_sem;
    };
} // namespace Sem
} // namespace lbc
