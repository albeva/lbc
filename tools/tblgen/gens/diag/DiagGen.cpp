// Custom TableGen backend for generating diagnostic definitions.
// Reads Diagnostics.td and emits Diagnostics.hpp
#include "DiagGen.hpp"

DiagGen::DiagGen(
    raw_ostream& os,
    const RecordKeeper& records,
    StringRef generator,
    StringRef ns,
    std::vector<StringRef> includes
)
: GeneratorBase(os, records, generator, ns, std::move(includes))
, m_categories(sortedByDef(m_records.getAllDerivedDefinitions("Category")))
, m_severities(sortedByDef(m_records.getAllDerivedDefinitions("Severity")))
, m_diagnostics(sortedByDef(m_records.getAllDerivedDefinitions("Diag"))) {
}

auto DiagGen::run() -> bool {
    diagKind();
    newline();

    doc("Encapsulate a diagnostic kind and its formatted message");
    line("using DiagMessage = std::pair<DiagKind, std::string>");
    newline();

    diagnosticFunctions();

    // Manually close the reopened namespace (closeNamespace() already ran in diagKind)
    m_os << "} // namespace lbc\n";
    return false;
}

// -------------------------------------------------------------------------
// DiagKind smart enum
// -------------------------------------------------------------------------

void DiagGen::diagKind() {
    doc("DiagKind identifies a specific diagnostic and carries its static metadata");
    block("struct DiagKind final", true, [&] {
        diagKindEnums();
        diagKindConstructors();
        diagKindAccessors();
        diagKindCollections();

        // private field
        scope(Scope::Private, true);
        comment("Underlying enumerator");
        line("Value m_value");
    });
    closeNamespace();
    newline();

    diagKindFormatter();

    // Reopen namespace for DiagMessage alias and factory functions
    m_os << "namespace lbc {\n";
}

void DiagGen::diagKindEnums() {
    doc("Diagnostic identifier");
    block("enum Value : std::uint8_t", true, [&] {
        for (const auto* diag : m_diagnostics) {
            line(diag->getName(), ",");
        }
    }, "*-use-enum-class");
    newline();

    doc("Diagnostic subsystem");
    block("enum class Category : std::uint8_t", true, [&] {
        for (const auto* cat : m_categories) {
            line(cat->getName(), ",");
        }
    });
    newline();

    doc("Total number of diagnostic kinds");
    line("static constexpr std::size_t COUNT = " + std::to_string(m_diagnostics.size()), ";\n");
}

void DiagGen::diagKindConstructors() {
    doc("Default-construct to an uninitialized diagnostic kind");
    line("constexpr DiagKind() = default", ";\n");
    doc("Implicitly convert from a Value enumerator");
    line("constexpr DiagKind(const Value value) // NOLINT(*-explicit-conversions)", "");
    line(": m_value(value) { }", "\n");

    doc("Return the underlying Value enum");
    block("[[nodiscard]] constexpr auto value() const", [&] {
        line("return m_value");
    });
    newline();

    doc("Compare two DiagKind values for equality");
    line("[[nodiscard]] constexpr auto operator==(const DiagKind& other) const -> bool = default", ";\n");
    doc("Compare against a raw Value enumerator");
    block("[[nodiscard]] constexpr auto operator==(const Value value) const -> bool", [&] {
        line("return m_value == value");
    });
    newline();
}

void DiagGen::diagKindAccessors() {
    doc("Return the category for this diagnostic");
    block("[[nodiscard]] constexpr auto getCategory() const -> Category", [&] {
        block("switch (m_value)", [&] {
            for (const auto* cat : m_categories) {
                const auto cases = collect(m_diagnostics, "category", cat);
                if (cases.empty()) {
                    continue;
                }
                for (const auto* cse : cases) {
                    line("case " + cse->getName(), ":");
                }
                line("    return Category::" + cat->getName());
            }
        });
        line("std::unreachable()");
    });
    newline();

    doc("Return the severity for this diagnostic");
    block("[[nodiscard]] constexpr auto getSeverity() const -> llvm::SourceMgr::DiagKind", [&] {
        block("switch (m_value)", [&] {
            for (const auto* sev : m_severities) {
                const auto cases = collect(m_diagnostics, "severity", sev);
                if (cases.empty()) {
                    continue;
                }
                for (const auto* cse : cases) {
                    line("case " + cse->getName(), ":");
                }
                line("    return " + getSeverity(sev).str());
            }
        });
        line("std::unreachable()");
    });
    newline();

    doc("Return the diagnostic code string");
    block("[[nodiscard]] constexpr auto getCode() const -> llvm::StringRef", [&] {
        block("switch (m_value)", [&] {
            for (const auto* diag : m_diagnostics) {
                line("case " + diag->getName() + ": return " + quoted(diag->getValueAsString("diagCode")));
            }
        });
        line("std::unreachable()");
    });
    newline();
}

void DiagGen::diagKindCollections() {
    for (const auto* sev : m_severities) {
        const auto all = collect(m_diagnostics, "severity", sev);
        if (all.empty()) {
            continue;
        }
        const auto name = sev->getName();
        const auto pascal = name.substr(0, 1).upper() + name.substr(1).str();
        doc("Return all " + pascal + " diagnostics");
        block("[[nodiscard]] static consteval auto all" + pascal + "s() -> std::array<DiagKind, " + std::to_string(all.size()) + ">", [&] {
            space();
            m_os << "return { ";
            bool first = true;
            for (const auto* diag : all) {
                if (first) {
                    first = false;
                } else {
                    m_os << ", ";
                }
                m_os << diag->getName();
            }
            m_os << " };\n"; }, "*-magic-numbers");
        newline();
    }
}

void DiagGen::diagKindFormatter() {
    doc("Support using DiagKind with std::print and std::format");
    line("template <>", "");
    block("struct std::formatter<lbc::DiagKind, char> final", true, [&] {
        block("constexpr static auto parse(std::format_parse_context& ctx)", [&] {
            line("return ctx.begin()");
        });
        newline();

        block("auto format(const lbc::DiagKind& value, auto& ctx) const", [&] {
            line("return std::format_to(ctx.out(), \"{}\", value.getCode())");
        });
    });
    newline();
}

// -------------------------------------------------------------------------
// Diagnostic factory functions
// -------------------------------------------------------------------------

void DiagGen::diagnosticFunctions() {
    block("namespace diagnostics", [&] {
        line("template<typename T>", "");
        line("concept Loggable = std::formattable<T, char>");
        newline();

        for (const auto& cat : m_categories) {
            category(cat);
        }
    });
}

void DiagGen::category(const Record* cat) {
    const auto diagnostics = collect(m_diagnostics, "category", cat);
    if (diagnostics.empty()) {
        return;
    }
    section(cat->getName());

    // Group by severity ordering: error, warning, remark, note
    for (const auto& sev : m_severities) {
        for (const auto& diag : collect(diagnostics, "severity", sev)) {
            diagnostic(diag);
            newline();
        }
    }
}

void DiagGen::diagnostic(const Record* record) {
    comment("Create " + record->getName() + " message");
    const auto [params, message] = messageSpec(record);

    block("[[nodiscard]] inline auto " + record->getName() + "(" + params + ") -> DiagMessage", [&] {
        line("return { DiagKind::" + record->getName() + ", " + message + " }");
    });
}

/**
 * Parse format string placeholders and return a (params, expression) pair.
 *
 * Untyped placeholders use the Loggable concept:
 *   "unexpected {found}, expected {expected}" -> pair(
 *       "const Loggable auto& found, const Loggable auto& expected",
 *       "std::format(\"unexpected {}, expected {}\", found, expected)"
 *   )
 *
 * Typed placeholders use the specified C++ type directly:
 *   "expected {expected:int}, got {got:int}" -> pair(
 *       "const int expected, const int got",
 *       "std::format(\"expected {}, got {}\", expected, got)"
 *   )
 *
 * Messages without placeholders return an empty params string and
 * a quoted string literal (no std::format call).
 */
auto DiagGen::messageSpec(const Record* record) -> std::pair<std::string, std::string> {
    const auto message = record->getValueAsString("message").str();

    std::string params;
    std::string formatStr;
    std::string formatArgs;

    for (std::size_t pos = 0; pos < message.size(); ++pos) {
        if (message[pos] != '{') {
            formatStr += message[pos];
            continue;
        }

        const auto end = message.find('}', pos + 1);
        assert(end != std::string::npos && "Unterminated placeholder in diagnostic message");

        const auto placeholder = std::string_view(message).substr(pos + 1, end - pos - 1);
        const auto colon = placeholder.find(':');

        const auto name = placeholder.substr(0, colon);
        const auto type = colon != std::string_view::npos ? placeholder.substr(colon + 1) : std::string_view {};

        if (!params.empty()) {
            params += ", ";
            formatArgs += ", ";
        }

        if (type.empty()) {
            params += "const Loggable auto& ";
        } else {
            params += "const ";
            params += type;
            params += ' ';
        }
        params += name;

        formatStr += "{}";
        formatArgs += name;

        pos = end;
    }

    if (formatArgs.empty()) {
        return { "", quoted(message) };
    }

    return { params, "std::format(" + quoted(formatStr) + ", " + formatArgs + ")" };
}

auto DiagGen::getCategory(const Record* record) -> llvm::StringRef {
    return record->getValueAsDef("category")->getName();
}

auto DiagGen::getSeverity(const Record* record) -> llvm::StringRef {
    const auto name = record->getName();
    if (name == "error") {
        return "llvm::SourceMgr::DK_Error";
    }
    if (name == "warning") {
        return "llvm::SourceMgr::DK_Warning";
    }
    if (name == "note") {
        return "llvm::SourceMgr::DK_Note";
    }
    if (name == "remark") {
        return "llvm::SourceMgr::DK_Remark";
    }
    std::unreachable();
}
