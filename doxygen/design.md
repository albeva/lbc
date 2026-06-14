# Design {#design}

## Goals

- Fully functional BASIC compiler, not a toy
- Full C ABI compatibility: data types, bitfields, packed structs, calling
  conventions
- Interop with C libraries via simple declarations (no glue code)
- Module system (`import module`)
- Compile-time evaluation (comptime functions, type construction)
- Simplified C++-style templates

## Memory Model

LBC follows the **C++ object model**, with cleaner syntax and one extra static
check. It does **not** aim for perfect memory safety — as in C and C++, misuse
of raw pointers and references is the programmer's responsibility.

- **Value semantics.** User types define constructors, a destructor, copy and
  move constructors, and copy/move assignment — the same special members as
  C++, invoked at the same points (initialization, assignment, scope exit).
- **The type system encodes `ref`, `ptr`, and `const`** as type constructors
  (`TypeReference`, `TypePointer`, `TypeConst`), so passing and ownership are
  expressed through the type rather than ad-hoc parameter keywords.
- **`consuming` parameters** take full ownership of the argument (≈ C++ `T&&`
  / a by-value sink).
- **`consume`** forces an ownership transfer (move) where the compiler would
  otherwise copy. In most cases the compiler infers the move at the last use.
- **Use-after-consume is a compile error** — the one guarantee beyond C++.
  Once a variable is consumed it is inaccessible, even where the consume was
  conditional:

  ```basic
  dim x as Box
  if isTrue() then
      pass(consume x)   ' force the move
  end if
  x = 10                ' error: 'x' is consumed
  ```

  The check is conservative — consumed on *any* path ⇒ inaccessible after the
  join — and there is no revival by reassignment.

Beyond these, semantics follow the C/C++ model.

### Type Expressions

Type expressions mirror C++, using keywords in place of punctuation:

| LBC           | C++        | Meaning            |
|---------------|------------|--------------------|
| `T ptr`       | `T*`       | pointer            |
| `T ref`       | `T&`       | reference          |
| `const T`     | `const T`  | const-qualified    |
| `const T ref` | `const T&` | reference to const |

Qualifier and indirection nesting follows C++ rules; the type system represents
each combination as a distinct nested type. Generics/templates are not yet
implemented.

The copy and move constructors show the conventions in use:

```basic
constructor(other as const This ref)       ' const This&  — copy constructor
constructor(consuming other as This ref)    ' This&&       — move constructor
```

## IR

A typed, high-level intermediate representation between the AST and LLVM IR.
Unlike LLVM IR, it preserves full semantic information:

- **Types** are first-class — generics, user-defined types, and type parameters
- **Serializable** — serves as the module interface format for `import`
- **Interpretable** — designed for execution by the comptime VM

No optimization passes — all optimization is delegated to LLVM.

## Comptime VM

A virtual machine that executes IR at compile time, enabling:

- Compile-time function evaluation (constexpr-style)
- Compile-time type construction (Zig-style `comptime`)
- Template instantiation driven by comptime evaluation

Uses a custom VM rather than LLVM's JIT to stay at the IR level where type
information is available.

## C Interoperability

The language targets seamless C interop at the ABI level:

- All C primitive types with matching sizes and alignment
- Bitfields and packed structs with C-compatible layout
- C calling conventions
- C libraries are imported via declaration headers (type and function signatures
  only)

Target-specific layout rules come from the backend's target query interface.

## TableGen

LLVM TableGen generates repetitive C++ definitions from declarative `.td` files.
Current uses:

- `src/Lexer/TokenKind.td` — token kinds, groups, operator properties
- `src/Ast/Ast.td` — AST node hierarchy, members, and visitor infrastructure
- `src/Diag/Diagnostics.td` — diagnostic message catalog and factory functions
