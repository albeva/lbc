//
// Created by Albert Varaksin on 08/07/2020.
//
#pragma once
#include "pch.hpp"

// clang-format off

// ID and STR are used in Token.deh.h for keyword tokens

//     ID        STR         Kind      c++
#define PRIMITIVE_TYPES(_) \
    _( Bool,     "BOOL",     Boolean,  bool             )  \
    _( ZString,  "ZSTRING",  ZString,  llvm::StringRef  )

//     ID        STR         Kind,     c++          bits    Signed
#define INTEGRAL_TYPES(_) \
    _( Byte,     "BYTE",     Integral, int8_t,      8,      true  ) \
    _( UByte,    "UBYTE",    Integral, uint8_t,     8,      false ) \
    _( Short,    "SHORT",    Integral, int16_t,     16,     true  ) \
    _( UShort,   "USHORT",   Integral, uint16_t,    16,     false ) \
    _( Integer,  "INTEGER",  Integral, int32_t,     32,     true  ) \
    _( UInteger, "UINTEGER", Integral, uint32_t,    32,     false ) \
    _( Long,     "LONG",     Integral, int64_t,     64,     true  ) \
    _( ULong,    "ULONG",    Integral, uint64_t,    64,     false )

//     ID        STR         Kind           Bits    c++
#define FLOATINGPOINT_TYPES(_) \
    _( Single,   "SINGLE",   FloatingPoint, float,   32 ) \
    _( Double,   "DOUBLE",   FloatingPoint, double,  64 )

#define ALL_TYPES(_)   \
    PRIMITIVE_TYPES(_) \
    INTEGRAL_TYPES(_)  \
    FLOATINGPOINT_TYPES(_)
