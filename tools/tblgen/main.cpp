// lbc-tblgen entry point. Dispatches to the selected generator.
#include <llvm/Support/CommandLine.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/Record.h>
#include "gens/TokensGen.hpp"
#include "gens/ast/AstFwdDeclGen.hpp"
#include "gens/ast/AstGen.hpp"
#include "gens/ast/AstVisitorGen.hpp"
#include "gens/diag/DiagGen.hpp"
#include "gens/type/TypeBaseGen.hpp"
#include "gens/type/TypeFactoryGen.hpp"
using namespace llvm;

namespace {

enum class Generator : std::uint8_t {
    TokensDef,
    AstDef,
    AstFwdDecl,
    AstVisitor,
    DiagDef,
    TypeBase,
    TypeFactory,
};

const auto generatorOpt = cl::opt<Generator> {
    "gen",
    cl::desc("Generator to run"),
    cl::Required,
    cl::values(
        clEnumValN(Generator::TokensDef, TokensGen::genName, "Generate token definitions"),
        clEnumValN(Generator::AstDef, AstGen::genName, "Generate AST node definitions"),
        clEnumValN(Generator::AstFwdDecl, AstFwdDeclGen::genName, "Generate AST forward declarations"),
        clEnumValN(Generator::AstVisitor, AstVisitorGen::genName, "Generate AST visitor"),
        clEnumValN(Generator::DiagDef, DiagGen::genName, "Generate diagnostic definitions"),
        clEnumValN(Generator::TypeBase, TypeBaseGen::genName, "Generate type base definitions"),
        clEnumValN(Generator::TypeFactory, TypeFactoryGen::genName, "Generate type factory")
    )
};

auto dispatch(raw_ostream& os, const RecordKeeper& records) -> bool {
    switch (generatorOpt) {
    case Generator::TokensDef:
        return TokensGen(os, records).run();
    case Generator::AstDef:
        return AstGen(os, records).run();
    case Generator::AstFwdDecl:
        return AstFwdDeclGen(os, records).run();
    case Generator::AstVisitor:
        return AstVisitorGen(os, records).run();
    case Generator::DiagDef:
        return DiagGen(os, records).run();
    case Generator::TypeBase:
        return TypeBaseGen(os, records).run();
    case Generator::TypeFactory:
        return TypeFactoryGen(os, records).run();
    default:
        std::unreachable();
    }
}

} // namespace

auto main(const int argc, const char* argv[]) -> int {
    cl::ParseCommandLineOptions(argc, argv);
    return TableGenMain(argv[0], dispatch); // NOLINT(*-pro-bounds-pointer-arithmetic)
}
