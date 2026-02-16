//
// Created by Albert Varaksin on 16/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Diagnostics.hpp"
namespace lbc {
class Context;
class DiagEngine;

/**
 * Opaque handle into the diagnostic engine's storage.
 *
 * Represents an index into DiagEngine's internal diagnostic vector.
 * Intentionally opaque — outside code can hold and propagate a DiagIndex
 * (e.g. as the error type in Result<T>), but only DiagEngine can create,
 * inspect, or resolve one.
 *
 * Default-constructed instances carry a sentinel value and are considered
 * invalid. This allows use with TRY_DECL and other patterns that require
 * default-constructible error types.
 *
 * Trivially copyable so that Result<T> = std::expected<T, DiagIndex>
 * remains lightweight.
 */
class [[nodiscard]] DiagIndex final {
public:
    /** Construct an invalid (sentinel) index. */
    constexpr DiagIndex() = default;

private:
    friend class DiagEngine;

    /// Underlying value type.
    using Value = std::uint32_t;

    /// Construct a valid index pointing into DiagEngine storage.
    constexpr explicit DiagIndex(const Value value)
    : m_value(value) { }

    /// Return the raw value. Asserts validity in debug builds.
    [[nodiscard]] constexpr auto get() const -> Value {
        assert(isValid() && "Getting value from invalid DiagIndex");
        return m_value;
    }

    /// Check whether this index points to an actual diagnostic.
    [[nodiscard]] constexpr auto isValid() const -> bool {
        return m_value != DiagIndex().m_value;
    }

    /// Raw value, sentinel when default-constructed.
    Value m_value = std::numeric_limits<Value>::max();
};

/**
 * Central diagnostic engine that accumulates diagnostics during compilation.
 *
 * Compiler passes log diagnostics through DiagEngine, which stores them
 * in a vector and returns a DiagIndex handle. That handle is propagated
 * as the error type in Result<T> = std::expected<T, DiagIndex>, keeping
 * the error path lightweight while the engine owns all diagnostic details
 * (severity, category, source location, formatted message, attached notes).
 */
class DiagEngine final {
public:
    NO_COPY_AND_MOVE(DiagEngine)

    explicit DiagEngine(Context& context);
    ~DiagEngine();

    [[nodiscard]] auto count(llvm::SourceMgr::DiagKind kind) const -> std::size_t;
    [[nodiscard]] auto hasErrors() const -> bool;
    [[nodiscard]] auto get(DiagIndex index) const -> const llvm::SMDiagnostic&;

    [[nodiscard]] auto log(
        llvm::SMLoc Loc,
        llvm::SourceMgr::DiagKind Kind,
        const std::string& Msg,
        llvm::ArrayRef<llvm::SMRange> Ranges = {}
    ) -> DiagIndex;

    void print() const;

private:
    [[maybe_unused]] Context& m_context;
    std::vector<llvm::SMDiagnostic> m_messages;
};

} // namespace lbc
