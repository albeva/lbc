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
, m_diagnostics(sortedByDef(m_records.getAllDerivedDefinitions("Diag")))
, m_error(records.getDef("error"))
, m_warning(records.getDef("warning"))
, m_remark(records.getDef("remark"))
, m_note(records.getDef("note")) {
}

auto DiagGen::run() -> bool {
    doc("Error subsystem");
    block("enum class DiagCategory : std::uint8_t", true, [&] {
        for (const auto* cat : m_categories) {
            line(cat->getName(), ",");
        }
    });
    newline();

    doc("Encapsulate diagnostic message details");
    block("struct [[nodiscard]] DiagMessage final", true, [&] {
        line("llvm::SourceMgr::DiagKind kind");
        line("DiagCategory category");
        line("std::string code");
        line("std::string message");
    });
    newline();

    block("namespace Diagnostics", [&] {
        line("template<typename T>", "");
        line("concept Loggable = std::formattable<T, char>");
        newline();

        for (const auto& cat : m_categories) {
            category(cat);
        }
    });
    return false;
}

void DiagGen::category(const Record* cat) {
    const auto diagnostics = collect(m_diagnostics, "category", cat);
    if (diagnostics.empty()) {
        return;
    }
    section(cat->getName());

    for (const auto& sev : { m_error, m_warning, m_remark, m_note }) {
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
        block("return", true, [&] {
            line(".kind = " + getKind(record), ",");
            line(".category = DiagCategory::" + getCategory(record), ",");
            line(".code = " + quoted(record->getValueAsString("diagCode")), ",");
            line(".message = " + message, ""); // NOLINT(*-type-traits)
        });
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

auto DiagGen::format(const Record* record) const -> std::string {
    (void)this;
    return record->getValueAsString("message").str();
}

auto DiagGen::getCategory(const Record* record) -> llvm::StringRef {
    return record->getValueAsDef("category")->getName();
}

auto DiagGen::getKind(const Record* record) const -> llvm::StringRef {
    const auto* severity = record->getValueAsDef("severity");
    if (severity == m_error) {
        return "llvm::SourceMgr::DiagKind::DK_Error";
    }
    if (severity == m_warning) {
        return "llvm::SourceMgr::DiagKind::DK_Warning";
    }
    if (severity == m_remark) {
        return "llvm::SourceMgr::DiagKind::DK_Remark";
    }
    if (severity == m_note) {
        return "llvm::SourceMgr::DiagKind::DK_Note";
    }
    std::unreachable();
}
