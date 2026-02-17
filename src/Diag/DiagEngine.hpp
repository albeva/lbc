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
 * Result type for fallible operations that may produce diagnostics.
 * On failure, carries a DiagIndex handle into DiagEngine storage.
 */
template <typename T>
using DiagResult = std::expected<T, DiagIndex>;

/**
 * Error value for returning a diagnostic from a fallible operation.
 * Wraps a DiagIndex in std::unexpected for use with DiagResult<T>.
 */
using DiagError = std::unexpected<DiagIndex>;

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

    /** Return the number of diagnostics with the given severity. */
    [[nodiscard]] auto count(llvm::SourceMgr::DiagKind kind) const -> std::size_t;

    /** Check whether any error-level diagnostics have been logged. */
    [[nodiscard]] auto hasErrors() const -> bool;

    /** Retrieve the structured DiagKind for a previously logged diagnostic. */
    [[nodiscard]] auto getKind(DiagIndex index) const -> DiagKind;

    /** Retrieve the LLVM SMDiagnostic for a previously logged diagnostic. */
    [[nodiscard]] auto getDiagnostic(DiagIndex index) const -> const llvm::SMDiagnostic&;

    /** Retrieve the C++ source location where the diagnostic was logged. */
    [[nodiscard]] auto getLocation(DiagIndex index) const -> const std::source_location&;

    /**
     * Log a diagnostic message and return an opaque handle to it.
     *
     * @param message diagnostic message (typically from Diagnostics::*)
     * @param loc source location, may be invalid for location-free diagnostics
     * @param ranges optional source ranges to highlight
     * @param location C++ call site, captured automatically
     * @return DiagIndex handle for the logged diagnostic
     */
    [[nodiscard]] auto log(
        const DiagMessage& message,
        llvm::SMLoc loc = {},
        const llvm::ArrayRef<llvm::SMRange>& ranges = {},
        const std::source_location& location = std::source_location::current()
    ) -> DiagIndex;

    /** Render all accumulated diagnostics to stdout. */
    void print() const;

private:
    /// Pairs the structured DiagMessage with the rendered LLVM diagnostic.
    struct Entry final { // NOLINT(*-member-init)
        DiagKind kind;
        llvm::SMDiagnostic diagnostic;
        std::source_location location;
    };

    [[maybe_unused]] Context& m_context;
    std::vector<Entry> m_messages;
};

} // namespace lbc
