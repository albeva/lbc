//
// Created by Albert on 29/05/2021.
//
#pragma once
#include "Ast/Ast.hpp"
#include "Gen/CodeGen.hpp"

namespace lbc::Gen {

template<typename T>
requires std::is_base_of_v<AstRoot, T>
class Builder {
public:
    Builder(CodeGen& gen, T& ast) noexcept
    : m_gen{ gen },
      m_builder{ gen.getBuilder() },
      m_llvmContext{ m_builder.getContext() },
      m_ast{ ast } {}

    CodeGen& m_gen;
    llvm::IRBuilder<>& m_builder;
    llvm::LLVMContext& m_llvmContext;
    T& m_ast;
};

} // namespace lbc::Gen
