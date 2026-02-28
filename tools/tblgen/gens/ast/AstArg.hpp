//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include <string>

namespace llvm {
class Record;
} // namespace llvm

namespace ast {
/**
 * Wraps a TableGen Member record. Determines whether the member is a
 * constructor parameter (no default value) or an initialized field, and
 * whether a setter should be generated (mutable bit).
 */
class AstArg final {
public:
    explicit AstArg(const llvm::Record* record);

    /// Whether this member generates a setter (mutable flag set in .td)
    [[nodiscard]] auto hasSetter() const -> bool { return m_mutable; }
    /// Whether this member is a constructor parameter (has no default value)
    [[nodiscard]] auto hasCtorParam() const -> bool { return m_ctorParam; }
    [[nodiscard]] auto getName() const -> const std::string& { return m_name; }
    [[nodiscard]] auto getType() const -> const std::string& { return m_type; }
    [[nodiscard]] auto getDefault() const -> const std::string& { return m_default; }
    /// Non-pointer types are passed as const in constructor and setter parameters
    [[nodiscard]] auto passAsConst() const -> bool {
        return m_type.back() != '*';
    }

private:
    std::string m_name;
    std::string m_type;
    std::string m_default;
    bool m_mutable;
    bool m_ctorParam;
};
} // namespace ast
