//
// Created by Albert on 10/03/2023.
//
#pragma once
#include "pch.hpp"

/**
 * Mark an enum as bitmasked enum
 * \code
 *   enum class Flags: unsigned { a = 1, b = 2, c = 4 };
 *   MARK_AS_FLAGS_ENUM(Flags);
 * \endcode
 */
#define MARK_AS_FLAGS_ENUM(TYPE) \
    template <>                  \
    struct flags::FlaggedEnum<TYPE> final : flags::Tag {}

namespace lbc::flags {
struct Tag {};

template <typename T>
    requires std::is_enum_v<T> && std::is_unsigned_v<std::underlying_type_t<T>>
struct FlaggedEnum {};

template <typename T>
concept IsFlagsEnum = std::is_base_of_v<Tag, FlaggedEnum<T>>;

template <typename T>
constexpr auto underlying(T val) {
    return static_cast<std::underlying_type_t<T>>(val);
}

inline namespace operators {
    // Binary operators

    template <IsFlagsEnum E>
    constexpr auto operator&(E lhs, E rhs) -> E {
        return static_cast<E>(underlying(lhs) & underlying(rhs));
    }

    template <IsFlagsEnum E>
    constexpr auto operator|(E lhs, E rhs) -> E {
        return static_cast<E>(underlying(lhs) | underlying(rhs));
    }

    template <IsFlagsEnum E>
    constexpr auto operator^(E lhs, E rhs) -> E {
        return static_cast<E>(underlying(lhs) ^ underlying(rhs));
    }

    // unary operator

    template <IsFlagsEnum E>
    constexpr auto operator~(E val) -> E {
        return static_cast<E>(~underlying(val));
    }

    // assignment

    template <IsFlagsEnum E>
    constexpr auto operator|=(E& lhs, E rhs) -> E& {
        lhs = lhs | rhs;
        return lhs;
    }

    template <IsFlagsEnum E>
    constexpr auto operator&=(E& lhs, E rhs) -> E& {
        lhs = lhs & rhs;
        return lhs;
    }

    template <IsFlagsEnum E>
    constexpr auto operator^=(E& lhs, E rhs) -> E& {
        lhs = lhs ^ rhs;
        return lhs;
    }

    //  comparison to underlying type

    template <IsFlagsEnum E, typename U = std::underlying_type<E>>
    constexpr auto operator==(E lhs, U rhs) -> bool {
        return underlying(lhs) == rhs;
    }

    template <IsFlagsEnum E, typename U = std::underlying_type<E>>
    constexpr auto operator!=(E lhs, U rhs) -> bool {
        return underlying(lhs) != rhs;
    }
} // namespace operators

// helper functions

template <IsFlagsEnum E>
constexpr auto has(E flags, E bits) -> bool {
    return (flags & bits) == bits;
}

template <IsFlagsEnum E>
constexpr void set(E& flags, E bits) {
    flags |= bits;
}

template <IsFlagsEnum E>
constexpr void unset(E& flags, E bits) {
    flags &= ~bits;
}

template <IsFlagsEnum E>
constexpr void toggle(E& flags, E bits) {
    flags ^= bits;
}
} // namespace lbc::flags
