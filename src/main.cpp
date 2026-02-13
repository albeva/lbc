#include "pch.hpp"
#include <llvm/Support/InitLLVM.h>
#include "Lexer/Token.hpp"
using namespace lbc::lexer;

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };

    constexpr TokenKind kind = TokenKind::StringLiteral;
    constexpr TokenKind second { kind };

    std::print("kind = {}", second);
}
