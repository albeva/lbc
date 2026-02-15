//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstVisitorGen.hpp"

AstVisitorGen::AstVisitorGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: GeneratorBase(os, records, genName, "lbc", { "\"Ast/Ast.hpp\"" }) { }

auto AstVisitorGen::run() -> bool {
    // TODO: generate AstVisitor template
    return false;
}
