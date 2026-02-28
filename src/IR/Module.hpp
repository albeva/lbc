//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Function.hpp"
namespace lbc::ir {
class Declaration;

class Module final {
public:
    Module() = default;

    [[nodiscard]] auto declarations() -> std::vector<Declaration*>& { return m_declarations; }
    [[nodiscard]] auto functions() -> llvm::ilist<Function>& { return m_functions; }

private:
    std::vector<Declaration*> m_declarations;
    llvm::ilist<Function> m_functions;
};

} // namespace lbc::ir
