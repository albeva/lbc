//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "../../GeneratorBase.hpp"
#include "AstGen.hpp"

/**
 * TableGen backend that reads Ast.td and emits AstVisitor.hpp.
 * Generates a CRTP-free visitor base class using C++23 deducing this,
 * with a switch-based dispatch method and per-node accept handlers.
 */
class AstVisitorGen final : public AstGen {
public:
    static constexpr auto genName = "lbc-ast-visitor";

    AstVisitorGen(
        raw_ostream& os,
        const RecordKeeper& records
    );

    [[nodiscard]] auto run() -> bool override;

private:
    void visitorBaseClass();
    void visitorClasses();
    void visitorClass(const AstClass* ast);
    void visit(const AstClass* klass);
    [[nodiscard]] static auto visitorSample(const AstClass* klass) -> std::string;
    void caseAccept(const AstClass* klass);
    void defaultCase();
};
