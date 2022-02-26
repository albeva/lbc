//
// Created by Albert Varaksin on 08/05/2021.
//
#pragma once
#include "Ast/Ast.hpp"
#include "Source.hpp"

namespace lbc {

struct TranslationUnit final {
    NO_COPY_AND_MOVE(TranslationUnit)

    TranslationUnit(std::unique_ptr<llvm::Module> module, const Source* src, AstModule* tree) noexcept
    : llvmModule{ std::move(module) }, source{ src }, ast{ tree } {}

    ~TranslationUnit() noexcept = default;

    std::unique_ptr<llvm::Module> llvmModule;
    const Source* source;
    AstModule* ast;
};

} // namespace lbc
