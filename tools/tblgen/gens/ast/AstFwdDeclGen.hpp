//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "AstGen.hpp"
namespace ast {
/**
 * TableGen backend that reads Ast.td and emits AstFwdDecl.hpp.
 * Generates forward declarations for all AST node classes.
 */
class AstFwdDeclGen final : public AstGen {
public:
    static constexpr auto genName = "lbc-ast-fwd-decl";

    AstFwdDeclGen(
        raw_ostream& os,
        const RecordKeeper& records
    );

    [[nodiscard]] auto run() -> bool override;
};
} // namespace ast
