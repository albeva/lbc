//
// Created by Albert Varaksin on 14/02/2026.
//
#pragma once
#include <utility>
#include "lib/TreeGen.hpp"
namespace ast {
using namespace llvm;
// -----------------------------------------------------------------------------
// The generator
// -----------------------------------------------------------------------------

/**
 * TableGen backend that reads Ast.td and emits Ast.hpp. Uses default
 * TreeGen tree-loading and code generation for: AstKind enum, forward
 * declarations, and complete C++ class definitions with constructors,
 * accessors, and data members. Only adds AST-specific forward declarations.
 */
class AstGen : public lib::TreeGen<> {
public:
    static constexpr auto genName = "lbc-ast-def";

    AstGen(
        raw_ostream& os,
        const RecordKeeper& records,
        StringRef generator = genName,
        StringRef ns = "lbc",
        std::vector<StringRef> includes = {
            "pch.hpp", "Symbol/LiteralValue.hpp", "Lexer/TokenKind.hpp" }
    );

    [[nodiscard]] auto run() -> bool override;

private:
    void forwardDecls();
};
} // namespace ast
