//
// Created by Albert Varaksin on 14/02/2026.
//
#pragma once
#include "../GeneratorBase.hpp"

/** TableGen backend that reads TokenKind.td and emits TokenKind.inc. */
class TokensGen final : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-tokens-def";

    TokensGen(raw_ostream& os, const RecordKeeper& records);

    [[nodiscard]] auto run() -> bool final;
};
