// Custom TableGen backend for generating token definitions.
// Reads Tokens.td and emits TokenKinds.inc
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/Record.h>
#include <ranges>
#include <span>
#include "Builder.hpp"
using namespace llvm;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

auto sortedByDef(const ArrayRef<const Record*> arr) -> std::vector<const Record*> {
    std::vector<const Record*> tokens { arr };
    std::ranges::sort(tokens, [](const Record* a, const Record* b) {
        return a->getID() < b->getID();
    });
    return tokens;
}

auto findRange(const std::vector<const Record*>& tokens, const StringRef field, const Record* record) -> std::optional<std::pair<const Record*, const Record*>> {
    const auto pred = [&](const auto* token) {
        const auto* value = token->getValue(field);
        return value != nullptr && value->getValue() == record->getDefInit();
    };

    const auto first = std::ranges::find_if(tokens, pred);
    if (first == std::ranges::end(tokens)) {
        return std::nullopt;
    }

    const auto lastPrev = std::ranges::find_if(tokens | std::views::reverse, pred);
    const auto last = std::prev(lastPrev.base());
    return std::make_pair(*first, *last);
}

auto collect(const std::vector<const Record*>& tokens, const StringRef field, const Record* record) -> std::vector<const Record*> {
    const auto pred = [&](const auto* token) {
        const auto* value = token->getValue(field);
        return value != nullptr && value->getValue() == record->getDefInit();
    };

    std::vector<const Record*> result {};
    for (const auto* token : tokens) {
        if (pred(token)) {
            result.push_back(token);
        }
    }

    return result;
}

auto contains(const std::vector<const Record*>& tokens, const StringRef field, const Record* record) -> bool {
    const auto pred = [&](const auto* token) {
        const auto* value = token->getValue(field);
        return value != nullptr && value->getValue() == record->getDefInit();
    };

    const auto it = std::ranges::find_if(tokens, pred);
    return it != tokens.end();
}

auto emitTokens(raw_ostream& os, const RecordKeeper& records) -> bool {
    const auto tokens = sortedByDef(records.getAllDerivedDefinitions("Token"));
    const auto groups = sortedByDef(records.getAllDerivedDefinitions("Group"));
    const auto categories = sortedByDef(records.getAllDerivedDefinitions("Category"));
    const auto operators = sortedByDef(records.getAllDerivedDefinitions("Operator"));

    // Code builder
    Builder build { os, records.getInputFilename(), "lbc::lexer" };

    // TokenKind struct
    build
        .doc(
            "TokenKind represents the value of a scanned token"
        )
        .block("struct TokenKind final", true, [&] {
            // --------------------------------------------------------------------
            // Inner value enum
            // --------------------------------------------------------------------
            build
                .doc("Value backing TokenKind. This is intentionally implicitly defined in scope")
                .block("enum Value : std::uint8_t", true, [&] {
                    for (const auto* token : tokens) {
                        build.line(token->getName(), ",");
                    }
                })
                .newline();

            // --------------------------------------------------------------------
            // Token Groups enum
            // --------------------------------------------------------------------
            build
                .doc("Token group represents the generic class of token")
                .block("enum class Group : std::uint8_t", true, [&] {
                    for (const auto* group : groups) {
                        build.line(group->getName(), ",");
                    }
                })
                .newline();

            // --------------------------------------------------------------------
            // Operator categories
            // --------------------------------------------------------------------
            build
                .block("enum class Category : std::uint8_t", true, [&] {
                    for (const auto* category : categories) {
                        build.line(category->getName(), ",");
                    }
                })
                .newline();

            // --------------------------------------------------------------------
            // number of tokens in total
            // --------------------------------------------------------------------
            build
                .comment("Number of total tokens")
                .line("static constexpr std::size_t COUNT = " + std::to_string(tokens.size()), ";\n");

            // --------------------------------------------------------------------
            // Token kind constructors
            // --------------------------------------------------------------------
            build.line("constexpr TokenKind() = default", ";\n");
            build.line("constexpr TokenKind(const Value value) : m_value(value) { }", "\n");

            // --------------------------------------------------------------------
            // get value
            // --------------------------------------------------------------------
            build
                .block("constexpr auto value() const", [&] {
                    build.line("return m_value");
                })
                .newline();

            // --------------------------------------------------------------------
            // Assignment from Value
            // --------------------------------------------------------------------
            build
                .block("constexpr auto operator=(const Value value) -> TokenKind&", [&] {
                    build.line("m_value = value").line("return *this");
                })
                .newline();

            // --------------------------------------------------------------------
            // Comparison
            // --------------------------------------------------------------------
            build
                .line("constexpr auto operator==(const TokenKind& value) const -> bool = default", ";\n")
                .block("constexpr auto operator==(const Value value) const -> bool", [&] {
                    build.line("return m_value == value");
                })
                .newline();

            // --------------------------------------------------------------------
            // isOneOf
            // --------------------------------------------------------------------
            build
                .line("template <typename... Tkns>", "")
                .block("[[nodiscard]] constexpr auto isOneOf(Tkns... tkn) const -> bool", [&] {
                    build.line("return ((m_value == TokenKind(tkn).m_value) || ...)");
                })
                .newline();

            // --------------------------------------------------------------------
            // Query methods
            // --------------------------------------------------------------------
            for (const auto* group : groups) {
                if (const auto range = findRange(tokens, "group", group)) {
                    build
                        .block("[[nodiscard]] constexpr auto is" + group->getName() + "() const -> bool", [&] {
                            build.line("return m_value >= " + range->first->getName() + " && m_value <= " + range->second->getName());
                        })
                        .newline();
                }
            }

            // --------------------------------------------------------------------
            // Get operator category
            // --------------------------------------------------------------------
            build
                .block("constexpr auto getCategory() const -> std::optional<Category>", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* op : operators) {
                            build
                                .line("case " + op->getName(), ":")
                                .line("    return Category::" + op->getValue("category")->getValue()->getAsString());
                        }
                        build
                            .line("default", ":")
                            .line("return std::nullopt");
                    });
                })
                .newline();

            // --------------------------------------------------------------------
            // operator queries
            // --------------------------------------------------------------------
            for (const auto* category : categories) {
                if (not contains(operators, "category", category)) {
                    continue;
                }
                build
                    .block("[[nodiscard]] constexpr auto is" + category->getName() + "() const -> bool", [&] {
                        build.line("return getCategory() == Category::" + category->getName());
                    })
                    .newline();
            }

            // --------------------------------------------------------------------
            // Get operator precedence
            // --------------------------------------------------------------------
            build
                .block("constexpr auto getPrecedence() const -> int", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* op : operators) {
                            build
                                .line("case " + op->getName(), ":")
                                .line("    return " + std::to_string(op->getValueAsInt("prec")));
                        }
                        build.line("default", ":")
                            .line("    return 0");
                    });
                })
                .newline();

            // --------------------------------------------------------------------
            // Check if operator isBinary
            // --------------------------------------------------------------------
            build
                .block("constexpr auto isBinary() const -> bool", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* op : operators) {
                            build
                                .line("case " + op->getName(), ":")
                                .line("    return "s + (op->getValueAsBit("isBinary") ? "true" : "false"));
                        }
                        build.line("default", ":")
                            .line("    return false");
                    });
                })
                .newline();

            // --------------------------------------------------------------------
            // Check if operator isUnary
            // --------------------------------------------------------------------
            build
                .block("constexpr auto isUnary() const -> bool", [&] {
                    build.line("return isOperator() && !isBinary()");
                })
                .newline();

            // --------------------------------------------------------------------
            // Check if operator isLeftAssociative
            // --------------------------------------------------------------------
            build
                .block("constexpr auto isLeftAssociative() const -> bool", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* op : operators) {
                            build
                                .line("case " + op->getName(), ":")
                                .line("    return "s + (op->getValueAsBit("isLeftAssociative") ? "true" : "false"));
                        }
                        build.line("default", ":")
                            .line("    return false");
                    });
                })
                .newline();

            // --------------------------------------------------------------------
            // Check if operator isRightAssociative
            // --------------------------------------------------------------------
            build
                .block("constexpr auto isRightAssociative() const -> bool", [&] {
                    build.line("return isOperator() && !isLeftAssociative()");
                })
                .newline();

            // --------------------------------------------------------------------
            // Get string
            // --------------------------------------------------------------------
            build
                .block("[[nodiscard]] constexpr auto string() const -> std::string_view", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* token : tokens) {
                            build.line("case " + token->getName() + ": return " + build.quoted(token->getValueAsString("str")) + "sv");
                        }
                    });
                    build.line("std::unreachable()");
                })
                .newline();

            // --------------------------------------------------------------------
            // get token groups
            // --------------------------------------------------------------------
            for (const auto* group : groups) {
                const auto all = collect(tokens, "group", group);
                if (all.empty()) {
                    continue;
                }
                build
                    .block("[[nodiscard]] static consteval auto all" + group->getName() + "s() -> std::array<TokenKind, " + std::to_string(all.size()) + ">", [&] {
                        build.space();
                        os << "return {";
                        bool first = true;
                        for (const auto* token : all) {
                            if (first) {
                                first = false;
                            } else {
                                os << ", ";
                            }
                            os << token->getName();
                        }
                        os << "};\n";
                    })
                    .newline();
            }

            // --------------------------------------------------------------------
            // value field
            // --------------------------------------------------------------------
            build
                .line("private:", "")
                .line("Value m_value");
        });
    return false;
}

} // namespace

auto main(const int argc, const char* argv[]) -> int {
    const auto span = std::span(argv, static_cast<std::size_t>(argc));
    cl::ParseCommandLineOptions(argc, argv);
    return TableGenMain(span.front(), emitTokens);
}
