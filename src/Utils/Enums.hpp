//
// Created by Albert on 10/03/2023.
//
#include "pch.hpp"
#pragma once

/**
 * Mark an enum as bitmasked enum
 * \code
 *   enum Flags { a = 1, b = 2, c = 4 };
 *   MARK_AS_FLAGS_ENUM(Flags);
 * \endcode
 */
#define MARK_AS_FLAGS_ENUM(TYPE)  \
    template<>                    \
    struct enums::EnumTag<TYPE> { \
        std::true_type marked;    \
    }

namespace lbc::enums {
template<typename T>
    requires std::is_enum_v<T>
struct EnumTag {
};

template<typename T>
concept IsFlagsEnum =
    requires(T) {
        { EnumTag<T>::marked };
    };

template<typename T>
constexpr auto underlying(T val) {
    return static_cast<std::underlying_type_t<T>>(val);
}

inline namespace operators {
    // Binary expr

    template<IsFlagsEnum E>
    constexpr E operator&(E lhs, E rhs) {
        return static_cast<E>(underlying(lhs) & underlying(rhs));
    }

    template<IsFlagsEnum E>
    constexpr E operator|(E lhs, E rhs) {
        return static_cast<E>(underlying(lhs) | underlying(rhs));
    }

    template<IsFlagsEnum E>
    constexpr E operator^(E lhs, E rhs) {
        return static_cast<E>(underlying(lhs) ^ underlying(rhs));
    }

    // unary expr

    template<IsFlagsEnum E>
    constexpr E operator~(E val) {
        return static_cast<E>(~underlying(val));
    }

    // assign expressions

    template<IsFlagsEnum E>
    constexpr E& operator|=(E& lhs, E rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    template<IsFlagsEnum E>
    constexpr E& operator&=(E& lhs, E rhs) {
        lhs = lhs & rhs;
        return lhs;
    }

    template<IsFlagsEnum E>
    constexpr E& operator^=(E& lhs, E rhs) {
        lhs = lhs ^ rhs;
        return lhs;
    }

    //  comparison to underlying type

    template<IsFlagsEnum E, typename U = std::underlying_type<E>>
    constexpr bool operator==(E lhs, U rhs) {
        return lhs == static_cast<E>(rhs);
    }

    template<IsFlagsEnum E, typename U = std::underlying_type<E>>
    constexpr bool operator!=(E lhs, U rhs) {
        return lhs != static_cast<E>(rhs);
    }
} // namespace operators

// helper functions
template<IsFlagsEnum E>
constexpr bool has(E flags, E flag) {
    return (flags & flag) != 0;
}

template<IsFlagsEnum E>
constexpr void set(E& flags, E flag) {
    flags |= flag;
}

template<IsFlagsEnum E>
constexpr void unset(E& flags, E flag) {
    flags &= ~flag;
}

template<IsFlagsEnum E>
constexpr void toggle(E& flags, E flag) {
    flags ^= flag;
}
} // namespace lbc::enums
