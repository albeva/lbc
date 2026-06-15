//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"

namespace llvm {
class Value;
}
namespace lbc {
class Type;
}

namespace lbc::ir::lib {

/**
 * Base class for all IR values.
 *
 * Every entity in the IR that can be referenced — named variables, temporaries,
 * literals, functions, and blocks — inherits from Value. Each carries a Kind
 * discriminator for LLVM-style RTTI and a Type pointer from the compiler's
 * type system.
 */
class Value {
public:
    NO_COPY_AND_MOVE(Value)

    /** Discriminator for LLVM-style RTTI across all Value subclasses. */
    enum class Kind : std::uint8_t {
        /// Named values
        Temporary,
        Variable,
        Function,
        BasicBlock,
        /// Unnamed value
        Literal
    };

    Value(const Kind kind, const Type* type)
    : m_kind(kind)
    , m_type(type) {}

    /** Get the RTTI kind discriminator. */
    [[nodiscard]] auto getKind() const -> Kind { return m_kind; }
    /** Get the type associated with this value. */
    [[nodiscard]] auto getType() const -> const Type* { return m_type; }
    /** LLVM RTTI support. */
    [[nodiscard]] static auto classof(const Value* /*unused*/) -> bool { return true; }

    /** Get the cached lowered LLVM value (set by codegen), or nullptr. */
    [[nodiscard]] auto getLlvm() const -> llvm::Value* { return m_llvm; }

    /**
     * Cache the lowered LLVM value. Marked const (and the field mutable) because
     * codegen walks the IR as `const` yet memoises the lowering on each value.
     */
    void setLlvm(llvm::Value* value) const { m_llvm = value; }

private:
    Kind m_kind;                           ///< RTTI discriminator
    const Type* m_type;                    ///< type of this value
    mutable llvm::Value* m_llvm = nullptr; ///< memoised LLVM lowering
};

} // namespace lbc::ir::lib
