// lbc-tblgen entry point. Dispatches to the selected generator.
#include <llvm/Support/CommandLine.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/Record.h>
#include "gens/ast/AstFwdDeclGen.hpp"
#include "gens/ast/AstGen.hpp"
#include "gens/ast/AstVisitorGen.hpp"
#include "gens/diag/DiagGen.hpp"
#include "gens/ir/IRInstGen.hpp"
#include "gens/tokens/TokensGen.hpp"
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
    IRInstDef,
};

const auto generatorOpt = cl::opt<Generator> {
    "gen",
    cl::desc("Generator to run"),
    cl::Required,
    cl::values(
        clEnumValN(Generator::TokensDef, tokens::TokensGen::genName, "Generate token definitions"),
        clEnumValN(Generator::AstDef, ast::AstGen::genName, "Generate AST node definitions"),
        clEnumValN(Generator::AstFwdDecl, ast::AstFwdDeclGen::genName, "Generate AST forward declarations"),
        clEnumValN(Generator::AstVisitor, ast::AstVisitorGen::genName, "Generate AST visitor"),
        clEnumValN(Generator::DiagDef, diag::DiagGen::genName, "Generate diagnostic definitions"),
        clEnumValN(Generator::TypeBase, type::TypeBaseGen::genName, "Generate type base definitions"),
        clEnumValN(Generator::TypeFactory, type::TypeFactoryGen::genName, "Generate type factory"),
        clEnumValN(Generator::IRInstDef, ir::IRInstGen::genName, "Generate IR instruction definitions")
    )
};

auto dispatch(raw_ostream& os, const RecordKeeper& records) -> bool {
    switch (generatorOpt) {
    case Generator::TokensDef:
        return tokens::TokensGen(os, records).run();
    case Generator::AstDef:
        return ast::AstGen(os, records).run();
    case Generator::AstFwdDecl:
        return ast::AstFwdDeclGen(os, records).run();
    case Generator::AstVisitor:
        return ast::AstVisitorGen(os, records).run();
    case Generator::DiagDef:
        return diag::DiagGen(os, records).run();
    case Generator::TypeBase:
        return type::TypeBaseGen(os, records).run();
    case Generator::TypeFactory:
        return type::TypeFactoryGen(os, records).run();
    case Generator::IRInstDef:
        return ir::IRInstGen(os, records).run();
    default:
        std::unreachable();
    }
}

} // namespace

auto main(const int argc, const char* argv[]) -> int {
    cl::ParseCommandLineOptions(argc, argv);
    return TableGenMain(argv[0], dispatch); // NOLINT(*-pro-bounds-pointer-arithmetic)
}
