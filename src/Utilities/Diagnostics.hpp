//
// Created by Albert Varaksin on 12/02/2026.
//
// This is inspired by SerenityOS / Ladybird browser
#pragma once

// Needed to turn the 'name' token and the preceding 'GCC diagnostic ignored'
// into a single string literal, it won't accept "foo"#bar concatenation.
#define LBC_PRAGMA_IMPL(x) _Pragma(#x)
#define LBC_PRAGMA(x) LBC_PRAGMA_IMPL(x)

// Helper macro to temporarily disable a diagnostic for the given statement.
// Using _Pragma() makes it possible to use this in other macros as well (and
// allows us to define it as a macro in the first place).
// NOTE: 'GCC' is also recognized by clang.
#define LBC_IGNORE_DIAGNOSTIC(name, statement) \
    LBC_PRAGMA(GCC diagnostic push);           \
    LBC_PRAGMA(GCC diagnostic ignored name);   \
    statement;                                 \
    LBC_PRAGMA(GCC diagnostic pop);
