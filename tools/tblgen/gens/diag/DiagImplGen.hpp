//
// Created by Albert Varaksin on 16/02/2026.
//
#pragma once
#include "DiagGen.hpp"

/**
 * TableGen backend that reads Diagnostics.td and emits Diagnostics.cpp.
 * Extends DiagGen to generate the implementation file with format string
 * tables and diagnostic emission functions.
 */
class DiagImplGen final : public DiagGen {
public:
    static constexpr auto genName = "lbc-diag-impl";

    DiagImplGen(
        raw_ostream& os,
        const RecordKeeper& records
    );

    [[nodiscard]] auto run() -> bool override;
};
