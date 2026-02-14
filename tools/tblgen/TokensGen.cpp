// Custom TableGen backend for generating token definitions.
// Reads Tokens.td and emits TokenKinds.inc
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>
#include <ranges>
#include "Builder.hpp"
#include "Generators.hpp"
using namespace llvm;

namespace {

auto sortedByDef(const ArrayRef<const Record*> arr) -> std::vector<const Record*> {
    std::vector<const Record*> tokens { arr };
    std::ranges::sort(tokens, [](const Record* one, const Record* two) {
        return one->getID() < two->getID();
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

} // namespace

auto emitTokens(raw_ostream& os, const RecordKeeper& records, const StringRef generator) -> bool {
    const auto tokens = sortedByDef(records.getAllDerivedDefinitions("Token"));
    const auto groups = sortedByDef(records.getAllDerivedDefinitions("Group"));
    const auto categories = sortedByDef(records.getAllDerivedDefinitions("Category"));
    const auto operators = sortedByDef(records.getAllDerivedDefinitions("Operator"));

    // Code builder
    Builder build {
        os,
        records.getInputFilename(),
        generator,
        "lbc",
        { "\"pch.hpp\"" }
    };

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
                    } }, "*-use-enum-class")
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
                .doc("Operator category classification")
                .block("enum class Category : std::uint8_t", true, [&] {
                    build.line("Invalid", ",");
                    for (const auto* category : categories) {
                        build.line(category->getName(), ",");
                    }
                })
                .newline();

            // --------------------------------------------------------------------
            // number of tokens in total
            // --------------------------------------------------------------------
            build
                .doc("Total number of token kinds")
                .line("static constexpr std::size_t COUNT = " + std::to_string(tokens.size()), ";\n");

            // --------------------------------------------------------------------
            // Token kind constructors
            // --------------------------------------------------------------------
            build.line("constexpr TokenKind() = default", ";\n");
            build
                .line("constexpr TokenKind(const Value value) // NOLINT(*-explicit-conversions)", "")
                .line(": m_value(value) { }", "\n");

            // --------------------------------------------------------------------
            // get value
            // --------------------------------------------------------------------
            build
                .doc("Return the underlying Value enum")
                .block("[[nodiscard]] constexpr auto value() const", [&] {
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
                .line("[[nodiscard]] constexpr auto operator==(const TokenKind& value) const -> bool = default", ";\n")
                .block("[[nodiscard]] constexpr auto operator==(const Value value) const -> bool", [&] {
                    build.line("return m_value == value");
                })
                .newline();

            // --------------------------------------------------------------------
            // isOneOf
            // --------------------------------------------------------------------
            build
                .doc("Check if this token matches any of the given kinds")
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
                        .doc(("Check if this token belongs to the " + group->getName() + " group").str())
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
                .doc("Return the operator category, or Invalid for non-operators")
                .block("[[nodiscard]] constexpr auto getCategory() const -> Category", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* cat : categories) {
                            const auto cases = collect(operators, "category", cat);
                            if (cases.empty()) {
                                continue;
                            }
                            for (const auto* cse : cases) {
                                build.line("case " + cse->getName(), ":");
                            }
                            build.line("    return Category::" + cat->getName());
                        }
                        build
                            .line("default", ":")
                            .line("    return Category::Invalid");
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
                    .doc(("Check if this is " + Builder::articulate(category->getName()) + category->getName() + " operator").str())
                    .block("[[nodiscard]] constexpr auto is" + category->getName() + "() const -> bool", [&] {
                        build.line("return getCategory() == Category::" + category->getName());
                    })
                    .newline();
            }

            // --------------------------------------------------------------------
            // Get operator precedence
            // --------------------------------------------------------------------
            build
                .doc("Return operator precedence (higher binds tighter), or 0 for non-operators")
                .block("[[nodiscard]] constexpr auto getPrecedence() const -> int", [&] {
                    build
                        .line("// NOLINTBEGIN(*-magic-numbers)", "")
                        .block("switch (m_value)", [&] {
                            std::int64_t value = 0;
                            bool first = true;
                            for (const auto* op : operators) {
                                if (first) {
                                    first = false;
                                    value = op->getValueAsInt("prec");
                                } else {
                                    if (const auto newValue = op->getValueAsInt("prec"); value != newValue) {
                                        build.line("    return " + std::to_string(value));
                                        value = newValue;
                                    }
                                }
                                build
                                    .line("case " + op->getName(), ":");
                            }
                            build.line("    return " + std::to_string(value));
                            build.line("default", ":")
                                .line("    return 0");
                        })
                        .line("// NOLINTEND(*-magic-numbers)", "");
                })
                .newline();

            // --------------------------------------------------------------------
            // Check if operator isBinary
            // --------------------------------------------------------------------
            build
                .doc("Check if this is a binary operator")
                .block("[[nodiscard]] constexpr auto isBinary() const -> bool", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* op : operators) {
                            if (!op->getValueAsBit("isBinary")) {
                                continue;
                            }
                            build.line("case " + op->getName(), ":");
                        }
                        build.line("    return true");
                        build.line("default", ":")
                            .line("    return false");
                    });
                })
                .newline();

            // --------------------------------------------------------------------
            // Check if operator isUnary
            // --------------------------------------------------------------------
            build
                .doc("Check if this is a unary operator")
                .block("[[nodiscard]] constexpr auto isUnary() const -> bool", [&] {
                    build.line("return isOperator() && !isBinary()");
                })
                .newline();

            // --------------------------------------------------------------------
            // Check if operator isLeftAssociative
            // --------------------------------------------------------------------
            build
                .doc("Check if this operator is left-associative")
                .block("[[nodiscard]] constexpr auto isLeftAssociative() const -> bool", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* op : operators) {
                            if (!op->getValueAsBit("isLeftAssociative")) {
                                continue;
                            }
                            build.line("case " + op->getName(), ":");
                        }
                        build.line("    return true");
                        build.line("default", ":")
                            .line("    return false");
                    });
                })
                .newline();

            // --------------------------------------------------------------------
            // Check if operator isRightAssociative
            // --------------------------------------------------------------------
            build
                .doc("Check if this operator is right-associative")
                .block("[[nodiscard]] constexpr auto isRightAssociative() const -> bool", [&] {
                    build.line("return isOperator() && !isLeftAssociative()");
                })
                .newline();

            // --------------------------------------------------------------------
            // Get string
            // --------------------------------------------------------------------
            build
                .doc("Return the string representation of this token")
                .block("[[nodiscard]] constexpr auto string() const -> llvm::StringRef", [&] {
                    build.block("switch (m_value)", [&] {
                        for (const auto* token : tokens) {
                            build.line("case " + token->getName() + ": return " + Builder::quoted(token->getValueAsString("str")));
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
                    .doc(("Return all " + group->getName() + " tokens").str())
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
                    }, "*-magic-numbers")
                    .newline();
            }

            // --------------------------------------------------------------------
            // get operators that are alpha-numeric
            // --------------------------------------------------------------------
            {
                static constexpr auto pred = [](const Record* record) {
                    return std::isalpha(*(record->getValueAsString("str").begin()));
                };
                auto opkws = operators | std::views::filter(pred);
                if (const auto count = std::ranges::distance(opkws); count != 0) {
                    build
                        .doc("Return all operators that look like keywords")
                        .block("[[nodiscard]] static consteval auto allOperatorKeywords() -> std::array<TokenKind, " + std::to_string(count) + ">", [&] {
                            build.space();
                            os << "return { ";
                            bool first = true;
                            for (const auto* token : opkws) {
                                if (first) {
                                    first = false;
                                } else {
                                    os << ", ";
                                }
                                os << token->getName();
                            }
                            os << " };\n";
                        }, "*-magic-numbers")
                        .newline();
                }
            }

            // --------------------------------------------------------------------
            // value field
            // --------------------------------------------------------------------
            build
                .line("private:", "")
                .line("Value m_value");
        })
        .closeNamespace()
        .newline();

    // --------------------------------------------------------------------
    // std::hash
    // --------------------------------------------------------------------
    build
        .doc("Support hashing TokenKind")
        .line("template <>", "")
        .block("struct std::hash<lbc::TokenKind> final", true, [&] {
            build.block("[[nodiscard]] auto operator()(const lbc::TokenKind& value) const noexcept -> std::size_t", [&] {
                build.line("return std::hash<std::underlying_type_t<lbc::TokenKind::Value>> {}(value.value())");
            });
        })
        .newline();

    // --------------------------------------------------------------------
    // std::formatter
    // --------------------------------------------------------------------
    build
        .doc("Support using TokenKind with std::print and std::format")
        .line("template <>", "")
        .block("struct std::formatter<lbc::TokenKind, char> final", true, [&] {
            // parse
            build
                .block("constexpr static auto parse(std::format_parse_context& ctx)", [&] {
                    build.line("return ctx.begin()");
                })
                .newline();

            build.block("auto format(const lbc::TokenKind& value, auto& ctx) const", [&] {
                build.line("return std::format_to(ctx.out(), \"{}\", value.string())");
            });
        });

    return false;
}
