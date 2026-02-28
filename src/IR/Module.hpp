//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Function.hpp"
namespace lbc::ir {
class Declaration;

/**
 * The root IR container.
 *
 * A Module holds all top-level declarations (extern functions, global
 * variables, type declarations) and function definitions that make up
 * a translation unit.
 */
class Module final {
public:
    Module() = default;

    /** Get the top-level declarations (externs, globals, types). */
    [[nodiscard]] auto declarations() -> std::vector<Declaration*>& { return m_declarations; }
    /** Get the function definitions. */
    [[nodiscard]] auto functions() -> llvm::ilist<Function>& { return m_functions; }

private:
    std::vector<Declaration*> m_declarations; ///< top-level declarations
    llvm::ilist<Function> m_functions;        ///< function definitions
};

} // namespace lbc::ir
