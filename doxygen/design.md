# Design {#design}

## Goals

- Fully functional BASIC compiler, not a toy
- Full C ABI compatibility: data types, bitfields, packed structs, calling
  conventions
- Interop with C libraries via simple declarations (no glue code)
- Module system (`import module`)
- Compile-time evaluation (comptime functions, type construction)
- Simplified C++-style templates

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
