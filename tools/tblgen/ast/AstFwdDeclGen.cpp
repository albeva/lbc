//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstFwdDeclGen.hpp"
using namespace ast;

AstFwdDeclGen::AstFwdDeclGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: AstGen(os, records, genName, "lbc", {}) {}

auto AstFwdDeclGen::run() -> bool {
    getRoot()->visit([&](const AstClass* klass) {
        line("class " + klass->getClassName());
    });
    return false;
}
