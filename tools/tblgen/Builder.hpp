//
// Created by Albert Varaksin on 12/02/2026.
//
#pragma once
#include <concepts>
#include <format>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include "../../src/Utilities/NoCopy.hpp"

using namespace llvm;

template <typename T>
concept Streamable = requires(raw_ostream& os, T val) {
    os << val;
};

/// Simple abstraction to generate C++ code
class Builder { // NOLINT(*-special-member-functions)
public:
    NO_COPY_AND_MOVE(Builder)

    enum class Scope : std::uint8_t {
        Private,
        Protected,
        Public
    };

    static constexpr std::size_t COLUMNS = 80;

    explicit Builder(
        raw_ostream& os,
        const StringRef file, // NOLINT(*-easily-swappable-parameters)
        const StringRef generator,
        const StringRef ns,
        std::vector<StringRef> includes = {}
    )
    : m_os(os)
    , m_file(file)
    , m_generator(generator)
    , m_ns(ns)
    , m_includes(std::move(includes)) {
        header();
    }

    virtual ~Builder() {
        footer();
    }

    void closeNamespace() {
        footer();
    }

    template <std::invocable Func>
    void block(const Streamable auto& line, Func&& func, const StringRef nolint = {}) {
        space();
        m_os << line << " ";
        indent(true, std::forward<Func>(func), nolint);
        m_os << "\n";
    }

    template <std::invocable Func>
    void block(const Streamable auto& line, const bool terminate, Func&& func, const StringRef nolint = {}) {
        space();
        m_os << line << " ";
        indent(true, std::forward<Func>(func), nolint);
        if (terminate) {
            m_os << ";";
        }
        m_os << "\n";
    }

    void line(const Streamable auto& line, const StringRef terminator = ";") {
        space();
        m_os << line << terminator << "\n"; // NOLINT(*-pro-bounds-array-to-pointer-decay, *-no-array-decay)
    }

    void lines(const std::string& lines, const StringRef sep = "\n") {
        if (not lines.empty()) {
            space();
            for (const auto& ch : lines) {
                switch (ch) {
                case '\n':
                    m_os << sep << m_space;
                    continue;
                case '\r':
                case '\v':
                case '\f':
                    continue;
                default:
                    m_os << ch;
                }
            }
            m_os << sep;
        }
    }

    struct ListOptions final {
        StringRef firstPrefix = "";
        StringRef prefix = "";
        StringRef suffix = "";
        StringRef lastSuffix = "";
        bool quote = false;
    };

    void list(const std::vector<std::string>& items, const ListOptions options) {
        for (std::size_t idx = 0; idx < items.size(); ++idx) {
            // indent
            space();
            // prefix
            if (idx > 0) {
                m_os << options.prefix;
            } else if (!options.firstPrefix.empty()) {
                m_os << options.firstPrefix;
            }
            // item
            const auto& item = items[idx]; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
            if (options.quote) {
                m_os << quoted(item);
            } else {
                m_os << item;
            }
            // suffix
            if (idx < (items.size() - 1)) {
                m_os << options.suffix;
            } else if (not options.lastSuffix.empty()) {
                m_os << options.lastSuffix;
            }
            // line break
            m_os << '\n';
        }
    }

    static auto quoted(const StringRef line) -> std::string {
        std::string result { "\"" };
        result.reserve(line.size() + 2);
        for (const auto& ch : line) {
            switch (ch) {
            case '"':
                result += "\\\"";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\v':
                result += "\\v";
                break;
            case '\f':
                result += "\\f";
                break;
            case '\0':
                result += "\\0";
                break;
            default:
                result += ch;
            }
        }
        result += '"';
        return result;
    }

    static auto articulate(const StringRef word) -> std::string {
        return StringRef("aeiouAEIOU").contains(word.front()) ? "an " : "a ";
    }

    static auto pluralize(const StringRef word) -> std::string {
        return (word.str() + "s");
    }

    void newline() {
        if (m_isDoc) {
            space();
        }
        m_os << "\n";
    }

    void add(const Streamable auto& content) {
        m_os << content;
    }

    template <std::invocable Func>
    void indent(const bool scoped, Func&& func, const StringRef nolint = {}) {
        if (scoped) {
            if (nolint.empty()) {
                m_os << "{\n";
            } else {
                m_os << "{ // NOLINT(" << nolint << ")\n";
            }
        }

        m_indent++;
        updateSpace();
        std::invoke(std::forward<Func>(func));
        m_indent--;
        updateSpace();

        if (scoped) {
            space();
            m_os << "}";
        }
    }

    void comment(const Streamable auto& comment) {
        space();
        m_os << "/// " << comment << "\n";
    }

    void doc(const StringRef comment) {
        space();
        m_os << "/**\n";
        space();
        m_os << " * ";
        for (const auto& ch : comment) {
            switch (ch) {
            case '\n':
                m_os << '\n';
                space();
                m_os << " * ";
                continue;
            case '\r':
            case '\v':
            case '\f':
                continue;
            default:
                m_os << ch;
            }
        }
        m_os << '\n';
        space();
        m_os << " */\n";
    }

    template <std::invocable Func>
    void doc(Func&& func) {
        m_os << m_space << "/**\n";
        m_isDoc = true;
        std::invoke(std::forward<Func>(func));
        m_isDoc = false;
        m_os << m_space << " */\n";
    }

    void section(const StringRef comment) {
        const std::size_t dashes = COLUMNS - 3 - (m_indent * 4);

        m_os << m_space << "// " << std::string(dashes, '-') << "\n";
        m_os << m_space << "// ";
        for (const auto& ch : comment) {
            switch (ch) {
            case '\n':
                m_os << '\n'
                     << m_space << "// ";
                continue;
            case '\r':
            case '\v':
            case '\f':
                continue;
            default:
                m_os << ch;
            }
        }
        m_os << '\n'
             << m_space << "// " << std::string(dashes, '-') << "\n\n";
    }

    void space() {
        if (m_isDoc) {
            m_os << " * ";
        }
        m_os << m_space;
    }

    void scope(Scope sc, const bool force = false) {
        if (force || m_scope != sc) {
            m_scope = sc;
            const auto tmp = m_indent;
            if (m_indent > 0) {
                m_indent--;
                updateSpace();
            }
            switch (sc) {
            case Scope::Private:
                line("private", ":");
                break;
            case Scope::Protected:
                line("protected", ":");
                break;
            case Scope::Public:
                line("public", ":");
                break;
            }
            m_indent = tmp;
            updateSpace();
        }
    }

protected:
    raw_ostream& m_os;              // NOLINT(*-avoid-const-or-ref-data-members, *-non-private-member-variables-in-classes)
    Scope m_scope = Scope::Private; // NOLINT(*-non-private-member-variables-in-classes)

private:
    void updateSpace() {
        m_space.clear();
        m_space.assign(m_indent * 4, ' ');
    }

    void header() const {
        m_os << "//\n";
        m_os << "// This file is part of lbc project\n";
        m_os << "// https://github.com/albeva/lbc\n";
        m_os << "//\n";
        m_os << "// DO NOT MODIFY. This file is AUTO GENERATED.\n";
        m_os << "//\n";
        m_os << "// This file is generated by " << quoted(m_generator) << " from " << quoted(llvm::sys::path::filename(m_file).str()) << "\n";
        m_os << "// clang-format off\n";
        m_os << "#pragma once\n";

        for (const auto& include : m_includes) {
            if (include.front() == '<' || include.front() == '"') {
                m_os << "#include " << include << "\n";
            } else {
                m_os << "#include \"" << include << "\"\n";
            }
        }

        m_os << "\n";
        m_os << "namespace " << m_ns << " {\n\n";
    }

    void footer() {
        if (m_closed) {
            return;
        }
        m_closed = true;
        m_os << "} // namespace " << m_ns << "\n";
    }

    StringRef m_file;
    StringRef m_generator;
    StringRef m_ns;
    std::size_t m_indent = 0;
    std::string m_space;
    std::vector<StringRef> m_includes;
    bool m_closed = false;
    bool m_isDoc = false;
};
