#include "pch.hpp"
#include <llvm/Support/InitLLVM.h>
#include "Driver/Context.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };

    lbc::Context context;
    std::string included;
    auto id = context.getSourceMgr().AddIncludeFile("samples/hello.bas", {}, included);
    lbc::Parser parser { context, id };

    const auto module = parser.parse();
    if (!module) {
        return EXIT_FAILURE;
    }

    lbc::SemanticAnalyser sema(context);
    if (!sema.analyse(*module.value())) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
