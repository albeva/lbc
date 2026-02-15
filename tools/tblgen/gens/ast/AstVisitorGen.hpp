//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "../../GeneratorBase.hpp"

/**
 * TableGen backend that reads Ast.td and emits AstVisitor.inc.
 * Generates a CRTP-free visitor base class using C++23 deducing this,
 * with a switch-based dispatch method and per-node accept handlers.
 */
class AstVisitorGen final : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-ast-visitor";

    AstVisitorGen(
        raw_ostream& os,
        const RecordKeeper& records
    );

    [[nodiscard]] auto run() -> bool final;
};
