// lbc-tblgen entry point. Dispatches to the selected generator.
#include <llvm/Support/CommandLine.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/Record.h>
#include "Generators.hpp"
using namespace llvm;

namespace {

enum class Generator : std::uint8_t {
    TokensDef,
    AstDef,
};

const auto generatorOpt = cl::opt<Generator> {
    "gen",
    cl::desc("Generator to run"),
    cl::Required,
    cl::values(
        clEnumValN(Generator::TokensDef, "lbc-tokens-def", "Generate token definitions"),
        clEnumValN(Generator::AstDef, "lbc-ast-def", "Generate AST node definitions")
    )
};

auto dispatch(raw_ostream& os, const RecordKeeper& records) -> bool {
    switch (generatorOpt) {
    case Generator::TokensDef:
        return TokensGenerator(os, records, "lbc-tokens-def").run();
    case Generator::AstDef:
        return AstGenerator(os, records, "lbc-ast-def").run();
    default:
        std::unreachable();
    }
}

} // namespace

auto main(const int argc, const char* argv[]) -> int {
    cl::ParseCommandLineOptions(argc, argv);
    return TableGenMain(argv[0], dispatch); // NOLINT(*-pro-bounds-pointer-arithmetic)
}
