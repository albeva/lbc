// Custom TableGen backend for generating token definitions.
// Reads Tokens.td and emits TokenKinds.inc
#include "TokensGen.hpp"

TokensGen::TokensGen(raw_ostream& os, const RecordKeeper& records)
: GeneratorBase(os, records, genName) {
}

auto TokensGen::run() -> bool {
    const auto tokens = sortedByDef(m_records.getAllDerivedDefinitions("Token"));
    const auto groups = sortedByDef(m_records.getAllDerivedDefinitions("Group"));
    const auto categories = sortedByDef(m_records.getAllDerivedDefinitions("Category"));
    const auto operators = sortedByDef(m_records.getAllDerivedDefinitions("Operator"));

    // TokenKind struct
    doc("TokenKind represents the value of a scanned token");
    block("struct TokenKind final", true, [&] {
        // --------------------------------------------------------------------
        // Inner value enum
        // --------------------------------------------------------------------
        doc("Value backing TokenKind. This is intentionally implicitly defined in scope");
        block("enum Value : std::uint8_t", true, [&] {
            for (const auto* token : tokens) {
                line(token->getName(), ",");
            } }, "*-use-enum-class");
        newline();

        // --------------------------------------------------------------------
        // Token Groups enum
        // --------------------------------------------------------------------
        doc("Token group represents the generic class of token");
        block("enum class Group : std::uint8_t", true, [&] {
            for (const auto* group : groups) {
                line(group->getName(), ",");
            }
        });
        newline();

        // --------------------------------------------------------------------
        // Operator categories
        // --------------------------------------------------------------------
        doc("Operator category classification");
        block("enum class Category : std::uint8_t", true, [&] {
            line("Invalid", ",");
            for (const auto* category : categories) {
                line(category->getName(), ",");
            }
        });
        newline();

        // --------------------------------------------------------------------
        // number of tokens in total
        // --------------------------------------------------------------------
        doc("Total number of token kinds");
        line("static constexpr std::size_t COUNT = " + std::to_string(tokens.size()), ";\n");

        // --------------------------------------------------------------------
        // Token kind constructors
        // --------------------------------------------------------------------
        line("constexpr TokenKind() = default", ";\n");
        line("constexpr TokenKind(const Value value) // NOLINT(*-explicit-conversions)", "");
        line(": m_value(value) { }", "\n");

        // --------------------------------------------------------------------
        // get value
        // --------------------------------------------------------------------
        doc("Return the underlying Value enum");
        block("[[nodiscard]] constexpr auto value() const", [&] {
            line("return m_value");
        });
        newline();

        // --------------------------------------------------------------------
        // Assignment from Value
        // --------------------------------------------------------------------
        block("constexpr auto operator=(const Value value) -> TokenKind&", [&] {
            line("m_value = value");
            line("return *this");
        });
        newline();

        // --------------------------------------------------------------------
        // Comparison
        // --------------------------------------------------------------------
        line("[[nodiscard]] constexpr auto operator==(const TokenKind& value) const -> bool = default", ";\n");
        block("[[nodiscard]] constexpr auto operator==(const Value value) const -> bool", [&] {
            line("return m_value == value");
        });
        newline();

        // --------------------------------------------------------------------
        // isOneOf
        // --------------------------------------------------------------------
        doc("Check if this token matches any of the given kinds");
        line("template <typename... Tkns>", "");
        block("[[nodiscard]] constexpr auto isOneOf(Tkns... tkn) const -> bool", [&] {
            line("return ((m_value == TokenKind(tkn).m_value) || ...)");
        });
        newline();

        // --------------------------------------------------------------------
        // Query methods
        // --------------------------------------------------------------------
        for (const auto* group : groups) {
            if (const auto range = findRange(tokens, "group", group)) {
                doc(("Check if this token belongs to the " + group->getName() + " group").str());
                block("[[nodiscard]] constexpr auto is" + group->getName() + "() const -> bool", [&] {
                    line("return m_value >= " + range->first->getName() + " && m_value <= " + range->second->getName());
                });
                newline();
            }
        }

        // --------------------------------------------------------------------
        // Get operator category
        // --------------------------------------------------------------------
        doc("Return the operator category, or Invalid for non-operators");
        block("[[nodiscard]] constexpr auto getCategory() const -> Category", [&] {
            block("switch (m_value)", [&] {
                for (const auto* cat : categories) {
                    const auto cases = collect(operators, "category", cat);
                    if (cases.empty()) {
                        continue;
                    }
                    for (const auto* cse : cases) {
                        line("case " + cse->getName(), ":");
                    }
                    line("    return Category::" + cat->getName());
                }
                line("default", ":");
                line("    return Category::Invalid");
            });
        });
        newline();

        // --------------------------------------------------------------------
        // operator queries
        // --------------------------------------------------------------------
        for (const auto* category : categories) {
            if (not contains(operators, "category", category)) {
                continue;
            }
            doc(("Check if this is " + articulate(category->getName()) + category->getName() + " operator").str());
            block("[[nodiscard]] constexpr auto is" + category->getName() + "() const -> bool", [&] {
                line("return getCategory() == Category::" + category->getName());
            });
            newline();
        }

        // --------------------------------------------------------------------
        // Get operator precedence
        // --------------------------------------------------------------------
        doc("Return operator precedence (higher binds tighter), or 0 for non-operators");
        block("[[nodiscard]] constexpr auto getPrecedence() const -> int", [&] {
            line("// NOLINTBEGIN(*-magic-numbers)", "");
            block("switch (m_value)", [&] {
                std::int64_t value = 0;
                bool first = true;
                for (const auto* op : operators) {
                    if (first) {
                        first = false;
                        value = op->getValueAsInt("prec");
                    } else {
                        if (const auto newValue = op->getValueAsInt("prec"); value != newValue) {
                            line("    return " + std::to_string(value));
                            value = newValue;
                        }
                    }
                    line("case " + op->getName(), ":");
                }
                line("    return " + std::to_string(value));
                line("default", ":");
                line("    return 0");
            });
            line("// NOLINTEND(*-magic-numbers)", "");
        });
        newline();

        // --------------------------------------------------------------------
        // Check if operator isBinary
        // --------------------------------------------------------------------
        doc("Check if this is a binary operator");
        block("[[nodiscard]] constexpr auto isBinary() const -> bool", [&] {
            block("switch (m_value)", [&] {
                for (const auto* op : operators) {
                    if (!op->getValueAsBit("isBinary")) {
                        continue;
                    }
                    line("case " + op->getName(), ":");
                }
                line("    return true");
                line("default", ":");
                line("    return false");
            });
        });
        newline();

        // --------------------------------------------------------------------
        // Check if operator isUnary
        // --------------------------------------------------------------------
        doc("Check if this is a unary operator");
        block("[[nodiscard]] constexpr auto isUnary() const -> bool", [&] {
            line("return isOperator() && !isBinary()");
        });
        newline();

        // --------------------------------------------------------------------
        // Check if operator isLeftAssociative
        // --------------------------------------------------------------------
        doc("Check if this operator is left-associative");
        block("[[nodiscard]] constexpr auto isLeftAssociative() const -> bool", [&] {
            block("switch (m_value)", [&] {
                for (const auto* op : operators) {
                    if (!op->getValueAsBit("isLeftAssociative")) {
                        continue;
                    }
                    line("case " + op->getName(), ":");
                }
                line("    return true");
                line("default", ":");
                line("    return false");
            });
        });
        newline();

        // --------------------------------------------------------------------
        // Check if operator isRightAssociative
        // --------------------------------------------------------------------
        doc("Check if this operator is right-associative");
        block("[[nodiscard]] constexpr auto isRightAssociative() const -> bool", [&] {
            line("return isOperator() && !isLeftAssociative()");
        });
        newline();

        // --------------------------------------------------------------------
        // Get string
        // --------------------------------------------------------------------
        doc("Return the string representation of this token");
        block("[[nodiscard]] constexpr auto string() const -> llvm::StringRef", [&] {
            block("switch (m_value)", [&] {
                for (const auto* token : tokens) {
                    line("case " + token->getName() + ": return " + quoted(token->getValueAsString("str")));
                }
            });
            line("std::unreachable()");
        });
        newline();

        // --------------------------------------------------------------------
        // get token groups
        // --------------------------------------------------------------------
        for (const auto* group : groups) {
            const auto all = collect(tokens, "group", group);
            if (all.empty()) {
                continue;
            }
            doc(("Return all " + group->getName() + " tokens").str());
            block("[[nodiscard]] static consteval auto all" + group->getName() + "s() -> std::array<TokenKind, " + std::to_string(all.size()) + ">", [&] {
                space();
                m_os << "return {";
                bool first = true;
                for (const auto* token : all) {
                    if (first) {
                        first = false;
                    } else {
                        m_os << ", ";
                    }
                    m_os << token->getName();
                }
                m_os << "};\n"; }, "*-magic-numbers");
            newline();
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
                doc("Return all operators that look like keywords");
                block("[[nodiscard]] static consteval auto allOperatorKeywords() -> std::array<TokenKind, " + std::to_string(count) + ">", [&] {
                    space();
                    m_os << "return { ";
                    bool first = true;
                    for (const auto* token : opkws) {
                        if (first) {
                            first = false;
                        } else {
                            m_os << ", ";
                        }
                        m_os << token->getName();
                    }
                    m_os << " };\n"; }, "*-magic-numbers");
                newline();
            }
        }

        // --------------------------------------------------------------------
        // value field
        // --------------------------------------------------------------------
        line("private:", "");
        line("Value m_value");
    });
    closeNamespace();
    newline();

    // --------------------------------------------------------------------
    // std::hash
    // --------------------------------------------------------------------
    doc("Support hashing TokenKind");
    line("template <>", "");
    block("struct std::hash<lbc::TokenKind> final", true, [&] {
        block("[[nodiscard]] auto operator()(const lbc::TokenKind& value) const noexcept -> std::size_t", [&] {
            line("return std::hash<std::underlying_type_t<lbc::TokenKind::Value>> {}(value.value())");
        });
    });
    newline();

    // --------------------------------------------------------------------
    // std::formatter
    // --------------------------------------------------------------------
    doc("Support using TokenKind with std::print and std::format");
    line("template <>", "");
    block("struct std::formatter<lbc::TokenKind, char> final", true, [&] {
        // parse
        block("constexpr static auto parse(std::format_parse_context& ctx)", [&] {
            line("return ctx.begin()");
        });
        newline();

        block("auto format(const lbc::TokenKind& value, auto& ctx) const", [&] {
            line("return std::format_to(ctx.out(), \"{}\", value.string())");
        });
    });

    return false;
}
