//
// Created by Albert Varaksin on 28/05/2021.
//
#pragma once
#include "pch.hpp"


namespace lbc {
class TypeRoot;
enum class TokenKind : std::uint8_t;

namespace Gen {
    [[nodiscard]] auto getCmpPred(const TypeRoot* type, TokenKind op) -> llvm::CmpInst::Predicate;
    [[nodiscard]] auto getBinOpPred(const TypeRoot* type, TokenKind op) -> llvm::Instruction::BinaryOps;
} // namespace Gen
} // namespace lbc
