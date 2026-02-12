// Custom TableGen backend for generating token definitions.
// Reads Tokens.td and emits TokenKinds.inc
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/Record.h>
#include <span>
#include "Builder.hpp"
using namespace llvm;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

auto emitTokens(raw_ostream& os, const RecordKeeper& records) -> bool {
    const auto tokens = records.getAllDerivedDefinitions("Token");
    const auto categories = records.getAllDerivedDefinitions("TokenCategory");

    Builder build { os, records.getInputFilename(), "lbc::lexer" };

    // TokenCategory enum
    build.enumClass("TokenCategory", "std::uint8_t", [&] {
        for (const auto* cat : categories) {
            build.enumCase(cat->getName());
        }
    });

    // TokenKind enum
    build.enumClass("TokenKind", "std::uint8_t", [&] {
        for (const auto* tok : tokens) {
            build.enumCase(tok->getName());
        }
    });

    build.block("constexpr auto tokenSpelling(TokenKind kind) -> std::string_view", [&] {
        build.block("switch (kind)", [&] {
            for (const auto* tok : tokens) {
                build.line(
                    "case TokenKind::"s + tok->getName() + ": return \""s + tok->getValueAsString("Spelling") + "\""
                );
            }
        });
    });
    os << "\n";

    // Category lookup
    build.block("constexpr auto tokenCategory(TokenKind kind) -> TokenCategory", [&] {
        build.block("switch (kind)", [&] {
            for (const auto* tok : tokens) {
                const auto* cat = tok->getValueAsDef("Category");
                build.line("case TokenKind::"s + tok->getName() + ": return TokenCategory::"s + cat->getName());
            }
        });
    });

    return false;
}

} // namespace

auto main(const int argc, const char* argv[]) -> int {
    const auto span = std::span(argv, static_cast<std::size_t>(argc));
    cl::ParseCommandLineOptions(argc, argv);
    return TableGenMain(span.front(), emitTokens);
}
