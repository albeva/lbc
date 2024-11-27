//
// Created by Albert on 26/02/2022.
//
#pragma once
#include "pch.hpp"
namespace lbc {
class SemanticAnalyzer;

namespace Sem {
    class Pass {
    public:
        NO_COPY_AND_MOVE(Pass)
        explicit Pass(SemanticAnalyzer& sem)
        : m_sem { sem } {
        }
        virtual ~Pass() = default;

    protected:
        SemanticAnalyzer& m_sem;
    };
} // namespace Sem
} // namespace lbc
