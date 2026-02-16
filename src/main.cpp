#include "pch.hpp"
#include <llvm/Support/InitLLVM.h>
#include "Driver/Context.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };

    lbc::Context context;
    std::string included;
    auto id = context.getSourceMgr().AddIncludeFile("samples/hello.bas", {}, included);
    lbc::Parser parser { context, id };
    const auto res = parser.parse();
    context.getDiag().print();
    if (!res) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
