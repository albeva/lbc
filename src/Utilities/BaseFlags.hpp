//
// Created by Albert Varaksin on 20/02/2026.
//
#pragma once

/**
 * A base class for untyped flags, allowing for flexible flag management.
 * It uses a template parameter to define the underlying type for the flags.
 * The default type is `unsigned`, but it can be specialized for other types.
 */
template <typename Enum>
    requires std::is_enum_v<Enum>
struct TypedFlags {
    using FlagType = Enum;

    constexpr TypedFlags() = default;
    constexpr explicit TypedFlags(const Enum flags) noexcept
    : m_flags(flags) {}

    /**
     * Get the current flags as an enum type.
     */
    [[nodiscard]] constexpr auto getFlags() const noexcept -> Enum { return m_flags; }

    /**
     * Sets the flags to the specified value, replacing any existing flags.
     *
     * @param flag The flag value to set.
     */
    constexpr void setFlags(const Enum flag) noexcept { m_flags = flag; }

    /**
     * Checks if the given flag(s) is set.
     *
     * @param flag The flag to check.
     * @return True if the flag is set, false otherwise.
     */
    [[nodiscard]] constexpr auto hasFlag(const Enum flag) const noexcept -> bool { return (underlying() & std::to_underlying(flag)) != 0; }

    /**
     * Sets a specific flag, adding it to the existing flags.
     *
     * @param flag The flag to set.
     */
    constexpr void setFlag(const Enum flag) noexcept { m_flags = static_cast<Enum>(underlying() | std::to_underlying(flag)); }

    /**
     * Unsets a specific flag, removing it from the existing flags.
     *
     * @param flag The flag to unset.
     */
    constexpr void unsetFlag(const Enum flag) noexcept { m_flags = static_cast<Enum>(underlying() & ~std::to_underlying(flag)); }

    /**
     * Toggles a specific flag, switching its state.
     *
     * @param flag The flag to toggle.
     */
    constexpr void toggleFlag(const Enum flag) noexcept { m_flags = static_cast<Enum>(underlying() ^ std::to_underlying(flag)); }

    /**
     * Reset all flags to their default state (zero).
     */
    constexpr void resetFlags() noexcept { m_flags = {}; }

private:
    [[nodiscard]] constexpr auto underlying() const noexcept -> std::underlying_type_t<Enum> {
        return std::to_underlying(m_flags);
    }

    Enum m_flags {};
};
