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
 * TableGen backend that reads Ast.td and emits Ast.hpp. Builds an in-memory
 * NodeClass tree mirroring the Node/Group/Leaf hierarchy, then walks it to
 * generate: AstKind enum, forward declarations, and complete C++ class
 * definitions with constructors, accessors, and data members.
 */
class AstGen : public lib::TreeGen<lib::TreeNode, lib::TreeNodeArg> {
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
    void astNodesEnum();
    void astForwardDecls();
    void astGroup(const lib::TreeNode* cls);
    void astClass(const lib::TreeNode* cls);
    void constructor(const lib::TreeNode* cls);
    void classof(const lib::TreeNode* cls);
    void functions(const lib::TreeNode* cls);
    void classArgs(const lib::TreeNode* cls);
};
} // namespace ast
