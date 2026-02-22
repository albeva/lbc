# Type System {#type_system}

## Type

@ref lbc::Type "Type" is the base representation for all types in the compiler.

## TypeFactory

@ref lbc::TypeFactory "TypeFactory" is the central registry for creating and
retrieving types. It is owned by @ref lbc::Context "Context" and ensures type
uniqueness — each distinct type exists as a single instance.

Types are constructed exclusively through the factory; direct construction is
restricted.

## Numeric Types

The language provides the following built-in numeric types:

### Signed Integers

| Type      | Size    | Range                                      |
|-----------|---------|--------------------------------------------|
| `BYTE`    | 1 byte  | -128 to 127                                |
| `SHORT`   | 2 bytes | -32,768 to 32,767                          |
| `INTEGER` | 4 bytes | -2,147,483,648 to 2,147,483,647            |
| `LONG`    | 8 bytes | -9,223,372,036,854,775,808 to ...807       |

### Unsigned Integers

| Type       | Size    | Range                          |
|------------|---------|--------------------------------|
| `UBYTE`    | 1 byte  | 0 to 255                       |
| `USHORT`   | 2 bytes | 0 to 65,535                    |
| `UINTEGER` | 4 bytes | 0 to 4,294,967,295             |
| `ULONG`    | 8 bytes | 0 to 18,446,744,073,709,551,615|

### Floating Point

| Type     | Size    | Precision        |
|----------|---------|------------------|
| `SINGLE` | 4 bytes | ~7 decimal digits  |
| `DOUBLE` | 8 bytes | ~15 decimal digits |

## Numeric Literals

Integer literals (e.g. `5`, `100`, `0`) are stored as `std::uint64_t` and are
**polymorphic** — they have no committed type until a type context constrains
them. Without any context, an integer literal defaults to `INTEGER`.

Floating-point literals (e.g. `3.14`, `0.5`) are stored as `double` and are
**committed** to floating-point type. Without any context, a floating-point
literal defaults to `DOUBLE`.

This distinction is important: integer literals can adapt to any numeric type
context (integral or floating-point), while floating-point literals are strictly
floating-point.

```basic
DIM a = 5        ' INTEGER (default for integer literals)
DIM d = 3.14     ' DOUBLE  (default for floating-point literals)
DIM b = 5 AS BYTE ' BYTE   (literal adapts to explicit cast)
```

> **Future:** C-style integer suffixes (e.g. `5UL` for unsigned long) may be
> added to allow explicit literal typing.

## Type Contexts

Every expression is resolved in one of three type contexts:

### 1. Implicit (Type Deduction)

The variable's type is deduced from the initializer expression. No target type
is imposed from outside.

```basic
DIM a = 5           ' INTEGER — default for integer literals
DIM d = 3.14        ' DOUBLE  — default for floating-point literals
DIM x = 1 + 2 * 3  ' INTEGER — all literals default to INTEGER
```

### 2. Coerced (Cast or Assignment Target)

A target type is imposed by an `AS` cast or by assignment into a typed
variable. The expression must be compatible with the target type.

```basic
DIM b AS BYTE = 5       ' 5 adapts to BYTE (literal)
DIM s AS SHORT = b      ' BYTE widens to SHORT (safe widening)
b += 1                  ' result stays BYTE (compound assignment preserves type)
```

### 3. Explicit (Typed Declaration with Initializer)

The variable has a declared type and an initializer. The initializer must be
compatible with the declared type.

```basic
DIM myint AS INTEGER = 10
```

Cases 2 and 3 are equivalent from the expression's perspective — both provide a
target type that the expression must conform to.

## AS Operator

`AS` is an infix operator that casts its left operand to the specified type.
It binds with normal operator precedence:

```basic
DIM x = 1 + 2 * 3 - 5 AS BYTE
' Parsed as: ((1 + (2 * 3)) - (5 AS BYTE))
' The AS BYTE applies only to the literal 5
' Since 5 is a polymorphic literal, it becomes BYTE
' Then: (1 + (2 * 3)) is INTEGER, minus BYTE
' Result type depends on binary operator type resolution
```

## Implicit Conversion Rules

The type system is **statically typed** with only **safe implicit conversions**.
There is **no C-style integer promotion** — types do not implicitly decay or
widen to `int`.

### Safe Widening (Variable and Literal)

A value of a smaller type can be implicitly converted to a larger type of the
same kind. This applies to both variables and literals.

**Signed → larger signed:**
```basic
DIM b AS BYTE = 5
DIM s AS SHORT = b    ' BYTE → SHORT (OK)
DIM i AS INTEGER = s  ' SHORT → INTEGER (OK)
DIM l AS LONG = i     ' INTEGER → LONG (OK)
```

**Unsigned → larger unsigned:**
```basic
DIM ub AS UBYTE = 5
DIM us AS USHORT = ub    ' UBYTE → USHORT (OK)
DIM ui AS UINTEGER = us  ' USHORT → UINTEGER (OK)
DIM ul AS ULONG = ui     ' UINTEGER → ULONG (OK)
```

**Smaller floating-point → larger floating-point:**
```basic
DIM s AS SINGLE = 1.0
DIM d AS DOUBLE = s  ' SINGLE → DOUBLE (OK)
```

### Safe Sign Change: Unsigned → Larger Signed (Variable and Literal)

An unsigned type can be implicitly converted to a **larger** signed type, since
the larger signed type can represent all values of the smaller unsigned type.
The reverse — signed → unsigned — is **never** implicit, regardless of size,
because signedness information would be lost.

```basic
DIM ub AS UBYTE = 200
DIM s AS SHORT = ub      ' UBYTE → SHORT (OK, SHORT can hold 0–255)
DIM us AS USHORT = 1000
DIM i AS INTEGER = us    ' USHORT → INTEGER (OK)

DIM b AS BYTE = 5
DIM us2 AS USHORT = b           ' ERROR: signed → unsigned is never implicit
DIM us3 AS USHORT = b AS USHORT ' OK: explicit cast
```

### Literal Adaptation

Integer literals are polymorphic and can adapt to any numeric type context
without restriction. Because a literal's value is known at compile time, it
can conform to any type that can represent it. (Value-range checking may be
added in a future phase.)

```basic
DIM b AS BYTE = 5     ' literal 5 adapts to BYTE
DIM d AS DOUBLE = 5   ' literal 5 adapts to DOUBLE
DIM x = 3.14 + 5      ' literal 5 adapts to DOUBLE (context from 3.14)
DIM y = 5 + 3.14      ' literal 5 adapts to DOUBLE (order does not matter)
```

### Disallowed Implicit Conversions

The following conversions are **not** implicit and require an explicit `AS` cast:

- **Narrowing:** larger type → smaller type (e.g. `INTEGER` → `BYTE`)
- **Signed → unsigned** of any size (e.g. `BYTE` → `UBYTE`, `INTEGER` → `ULONG`)
- **Unsigned → signed** of same or smaller size (e.g. `UBYTE` → `BYTE`)
- **Integer → floating-point** for variables (lossy above 2^24 / 2^53)
- **Floating-point → integer** (truncation)
- **Floating-point → unsigned** (truncation and sign)

```basic
DIM a AS INTEGER = 5
DIM d AS DOUBLE = 3.14
DIM bad = d + a   ' ERROR: cannot implicitly convert INTEGER to DOUBLE
DIM ok = d + a AS DOUBLE  ' OK: explicit cast
```

## Binary Operator Type Resolution

When a binary operator has two operands of different types, the result type is
determined by the following rules (applied in order):

1. **Both literals:** both adapt; result uses the default type (`INTEGER` for
   two integer literals, `DOUBLE` if either is floating-point).
2. **One literal, one typed:** the literal adapts to the typed operand's type.
   Result type matches the typed operand.
3. **Both typed, same type:** result type is that type.
4. **Both typed, safe widening exists:** the narrower operand is implicitly
   widened. Result type is the wider type.
5. **Both typed, no safe conversion:** error.

## Compound Assignment

Compound assignment operators (`+=`, `-=`, `*=`, `/=`) preserve the type of
the left-hand side. The right-hand side must be compatible with the left-hand
side's type (via safe widening or literal adaptation).

```basic
DIM b AS BYTE = 5
b += 1        ' b stays BYTE, literal 1 adapts to BYTE
b += 300      ' future: may be a compile-time error (300 > BYTE range)
```

## Summary of Design Principles

- **Swift-inspired**, simplified for BASIC: strict static typing with limited
  implicit conversions.
- **Literals are polymorphic**, variables are not. Integer literals adapt to any
  numeric context; floating-point literals are committed to floating-point.
- **No C-style integer promotion.** Types do not silently widen to `int`. A
  `BYTE + BYTE` stays `BYTE`.
- **Safe widening only.** Implicit conversions never lose information. Narrowing,
  sign changes between same-size types, and integer-to-float conversions for
  variables all require explicit `AS` casts.
- **Type deduction is one-way.** Type information flows from context to
  expression (top-down for literals) or from expression to variable (bottom-up
  for deduction). There is no bidirectional type inference.
- **Overflow is out of scope** for the current phase. Future work may add
  compile-time range checking for literals and constant expressions.
