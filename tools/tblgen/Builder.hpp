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

    auto closeNamespace() -> Builder& {
        footer();
        return *this;
    }

    template <std::invocable Func>
    auto block(const Streamable auto& line, Func&& func, const StringRef nolint = {}) -> Builder& {
        m_os << m_space << line << " ";
        indent(true, std::forward<Func>(func), nolint);
        m_os << "\n";
        return *this;
    }

    template <std::invocable Func>
    auto block(const Streamable auto& line, const bool terminate, Func&& func, const StringRef nolint = {}) -> Builder& {
        m_os << m_space << line << " ";
        indent(true, std::forward<Func>(func), nolint);
        if (terminate) {
            m_os << ";";
        }
        m_os << "\n";
        return *this;
    }

    auto line(const Streamable auto& line, const StringRef terminator = ";") -> Builder& {
        m_os << m_space << line << terminator << "\n";
        return *this;
    }

    auto lines(const std::string& lines, const StringRef sep = "\n") -> Builder& {
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
        return *this;
    }

    struct ListOptions final {
        StringRef firstPrefix = "";
        StringRef prefix = "";
        StringRef suffix = "";
        StringRef lastSuffix = "";
    };

    auto list(const std::vector<std::string>& items, const ListOptions options) {
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
            m_os << items[idx]; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
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

    auto newline() -> Builder& {
        m_os << "\n";
        return *this;
    }

    auto add(const Streamable auto& content) -> Builder& {
        m_os << content;
        return *this;
    }

    template <std::invocable Func>
    auto indent(const bool scoped, Func&& func, const StringRef nolint = {}) -> Builder& {
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
            m_os << m_space << "}";
        }

        return *this;
    }

    auto comment(const Streamable auto& comment) -> Builder& {
        m_os << m_space << "/// " << comment << "\n";
        return *this;
    }

    auto doc(const StringRef comment) -> Builder& {
        m_os << m_space << "/**\n";
        m_os << m_space << " * ";
        for (const auto& ch : comment) {
            switch (ch) {
            case '\n':
                m_os << '\n'
                     << m_space << " * ";
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
             << m_space << " */\n";
        return *this;
    }

    auto section(const StringRef comment) -> Builder& {
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
        return *this;
    }

    auto space() -> Builder& {
        m_os << m_space;
        return *this;
    }

    auto scope(Scope sc) -> Builder& {
        if (m_scope != sc) {
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
        return *this;
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
        m_os << "// This file is generated by " << quoted(m_generator) << " from " << quoted(llvm::sys::path::filename(m_file).str()) << "\n";
        m_os << "// clang-format off\n";
        m_os << "#pragma once\n";

        for (const auto& include : m_includes) {
            m_os << "#include " << include << "\n";
        }

        m_os << "\n";
        m_os << "namespace " << m_ns << " {\n\n";
    }

    void footer() {
        if (closed) {
            return;
        }
        closed = true;
        m_os << "} // namespace " << m_ns << "\n";
    }

    StringRef m_file;
    StringRef m_generator;
    StringRef m_ns;
    std::size_t m_indent = 0;
    std::string m_space;
    std::vector<StringRef> m_includes;
    bool closed = false;
};
