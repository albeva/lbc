#include "pch.hpp"
#include <llvm/Support/InitLLVM.h>

#include "../build/claude/generated/Lexer/Tokens.inc"
#include "Driver/Context.hpp"
#include "Lexer/Lexer.hpp"

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };

    lbc::Context context;
    std::string included;
    auto id = context.getSourceMgr().AddIncludeFile("samples/hello.bas", {}, included);

    lbc::Lexer lexer { context, id };
    while (true) {
        auto token = lexer.next();
        std::println("token = {}", token.kind());
        if (token.kind() == lbc::TokenKind::EndOfFile) {
            break;
        }
    }
}
