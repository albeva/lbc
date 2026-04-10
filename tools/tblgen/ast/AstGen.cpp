// Custom TableGen backend for generating AST node definitions.
// Reads Ast.td and emits Ast.hpp
#include "AstGen.hpp"
using namespace std::string_literals;
using namespace ast;

// -----------------------------------------------------------------------------
// The generator
// -----------------------------------------------------------------------------

AstGen::AstGen(
    raw_ostream& os,
    const RecordKeeper& records,
    StringRef generator,
    StringRef ns,
    std::vector<StringRef> includes
)
: TreeGen(os, records, generator, "Ast", ns, std::move(includes)) {}

auto AstGen::run() -> bool {
    header();
    forwardDecls();
    treeKindEnum();
    newline();
    treeForwardDeclare();
    treeGroups(getRoot());
    footer();
    return false;
}

/**
 * Generate forward declarations of types required by ast
 */
void AstGen::forwardDecls() {
    line("class Type");
    line("class SymbolTable");
    line("class Symbol");
    block("namespace ir::lib", [&] {
        line("class Value");
    });
    newline();
}
