//
// Created by Albert Varaksin on 13/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Character.hpp"
namespace lbc {

/**
 * Lightweight wrapper around const char* providing an OO API for
 * traversing source text. Tracks a single position in a null-terminated
 * buffer and supports lookahead, advancement, and lexeme extraction.
 *
 * In debug builds, all operations assert that the cursor does not
 * advance or peek past the null terminator.
 */
class [[nodiscard]] Cursor final {
public:
    constexpr Cursor() = default;

    /**
     * Construct a cursor pointing to the given position.
     */
    constexpr explicit Cursor(const char* ptr)
    : m_ptr(ptr) { }

    /**
     * Return the character at the current position.
     */
    [[nodiscard]] constexpr auto current() const -> Character {
        return { *m_ptr };
    }

    /**
     * Return the character at the given offset from the current position.
     * In debug builds, asserts that no intermediate character is the null
     * terminator, ensuring the peek target is within the buffer.
     */
    [[nodiscard]] constexpr auto peek(std::size_t lookAhead = 1) const -> Character {
#if LBC_DEBUG_BUILD
        for (std::size_t idx = 0; idx < lookAhead; ++idx) {
            assert(m_ptr[idx] != '\0' && "Trying to peek past \0 terminator"); // NOLINT(*-pro-bounds-pointer-arithmetic)
        }
#endif
        return Character { m_ptr[lookAhead] }; // NOLINT(*-pro-bounds-pointer-arithmetic)
    }

    /**
     * Return the raw pointer at the current position.
     */
    [[nodiscard]] constexpr auto data() const -> const char* { return m_ptr; }

    /**
     * Move the cursor forward by the given number of characters.
     * In debug builds, asserts that none of the skipped characters
     * are the null terminator.
     */
    constexpr void advance(std::size_t amount = 1) {
#if LBC_DEBUG_BUILD
        for (std::size_t idx = 0; idx < amount; ++idx) {
            assert(m_ptr[idx] != '\0' && "Advancing past \0 terminator"); // NOLINT(*-pro-bounds-pointer-arithmetic)
        }
#endif
        std::advance(m_ptr, amount);
    }

    /**
     * Advance the cursor while the predicate holds for the current character.
     */
    template <std::invocable<Character> F>
    constexpr void advanceWhile(F&& predicate) {
        while (std::invoke(std::forward<F>(predicate), current())) {
            advance();
        }
    }

    /**
     * Advance the cursor while the predicate does not hold for the current character.
     */
    template <std::invocable<Character> F>
    constexpr void advanceWhileNot(F&& predicate) {
        while (!std::invoke(std::forward<F>(predicate), current())) {
            advance();
        }
    }

    /**
     * Return the number of characters between this cursor and other.
     * This cursor must point at or before other. In debug builds,
     * asserts ordering and that no null terminator lies between them.
     */
    [[nodiscard]] constexpr auto distanceTo(const Cursor& other) const -> std::size_t {
#if LBC_DEBUG_BUILD
        assert(m_ptr <= other.m_ptr && "Current cursor should be before other");
        for (const auto* ptr = m_ptr; ptr != other.m_ptr; ++ptr) { // NOLINT(*-pro-bounds-pointer-arithmetic)
            assert(*ptr != '\0' && "distance should not cover \0 terminator");
        }
#endif
        const std::ptrdiff_t distance = other.m_ptr - m_ptr;
        return static_cast<std::size_t>(distance);
    }

    /**
     * Check if the cursor has been initialised with a valid pointer.
     */
    [[nodiscard]] constexpr auto isValid() const -> bool {
        return m_ptr != nullptr;
    }

    /**
     * Extract the text between this cursor and other as a StringRef.
     */
    [[nodiscard]] constexpr auto stringTo(const Cursor& other) const -> llvm::StringRef {
        return llvm::StringRef { m_ptr, distanceTo(other) };
    }

private:
    const char* m_ptr = nullptr;
};

} // namespace lbc
