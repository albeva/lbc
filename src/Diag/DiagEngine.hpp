//
// Created by Albert Varaksin on 16/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {
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

    /// Underlying index type.
    using Index = std::uint32_t;

    /// Construct a valid index pointing into DiagEngine storage.
    constexpr explicit DiagIndex(Index index) : m_index(index) {}

    /// Return the raw index value. Asserts validity in debug builds.
    [[nodiscard]] constexpr auto getIndex() const -> Index {
        assert(isValid() && "Getting value from invalid DiagIndex");
        return m_index;
    }

    /// Check whether this index points to an actual diagnostic.
    [[nodiscard]] constexpr auto isValid() const -> bool {
        return m_index != std::numeric_limits<Index>::max();
    }

    /// Raw index, sentinel when default-constructed.
    Index m_index = std::numeric_limits<Index>::max();
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
};

} // namespace lbc
