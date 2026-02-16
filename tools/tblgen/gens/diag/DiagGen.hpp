//
// Created by Albert Varaksin on 16/02/2026.
//
#pragma once
#include "../../GeneratorBase.hpp"

/**
 * TableGen backend that reads Diagnostics.td and emits Diagnostics.hpp.
 * Parses format string placeholders ({name} / {name:type}) to extract
 * typed parameters for each diagnostic message.
 */
class DiagGen : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-diag-def";

    DiagGen(
        raw_ostream& os,
        const RecordKeeper& records,
        StringRef generator = genName,
        StringRef ns = "lbc",
        std::vector<StringRef> includes = { "pch.hpp" }
    );

    [[nodiscard]] auto run() -> bool override;
};
