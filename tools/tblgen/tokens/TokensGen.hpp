//
// Created by Albert Varaksin on 14/02/2026.
//
#pragma once
#include "lib/GeneratorBase.hpp"
namespace tokens {
using namespace llvm;

/** TableGen backend that reads TokenKind.td and emits TokenKind.hpp. */
class TokensGen final : public lib::GeneratorBase {
public:
    static constexpr auto genName = "lbc-tokens-def";

    TokensGen(raw_ostream& os, const RecordKeeper& records);

    [[nodiscard]] auto run() -> bool override;
};
} // namespace tokens
