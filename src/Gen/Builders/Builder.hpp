//
// Created by Albert on 29/05/2021.
//
#pragma once
#include "pch.hpp"
#include "Ast/Ast.hpp"
#include "Gen/CodeGen.hpp"

namespace lbc::Gen {

template<typename T>
    requires std::is_base_of_v<AstRoot, T>
class Builder {
public:
    NO_COPY_AND_MOVE(Builder)

    Builder(CodeGen& gen, T& ast)
    : m_gen{ gen },
      m_builder{ gen.getBuilder() },
      m_llvmContext{ m_builder.getContext() },
      m_ast{ ast } {}

    ~Builder() = default;

    CodeGen& m_gen;
    llvm::IRBuilder<>& m_builder;
    llvm::LLVMContext& m_llvmContext;
    T& m_ast;
};

} // namespace lbc::Gen
