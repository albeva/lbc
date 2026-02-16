# LBC Design

Design notes for lbc — a BASIC compiler with full C ABI compatibility and an LLVM backend.

## Goals

- Fully functional BASIC compiler, not a toy
- Full C ABI compatibility: data types, bitfields, packed structs, calling conventions
- Interop with C libraries via simple declarations (liek header files)
- Module system (`import module`)
- Compile-time evaluation (comptime functions, type construction)
- Simplified C++-style templates
- Clean, modern C++23 codebase

## Compiler Pipeline

```
Source → Lexer → Parser → AST → Semantic Analysis → IR → LLVM IR → Machine Code
                                                     ↕
                                                  Comptime VM
```

### Frontend

Lexer, parser, AST, semantic analysis, and diagnostics. Prefer standard C++23 where it's the
natural fit, but freely use LLVM tools and libraries throughout (e.g. TableGen for code generation,
LLVM ADT utilities where they're better than the STL equivalents).

### IR

A typed, high-level intermediate representation that sits between the AST and LLVM IR.
Unlike LLVM IR, it preserves full semantic information:

- **Types** are first-class — generics, user-defined types, and type parameters are all represented
- **Serializable** — serves as the module interface format for `import`
- **Interpretable** — designed to be executed by the comptime VM

The IR carries no optimization passes. Its purpose is to preserve semantics, not to transform
code. All optimization is delegated to LLVM.

### Comptime VM

A simple virtual machine that executes IR at compile time. Operates on the IR directly where
types are still first-class, enabling:

- Compile-time function evaluation (constexpr-style)
- Compile-time type construction (Zig-style `comptime` — functions that return types)
- Template instantiation driven by comptime evaluation

Using a custom VM rather than LLVM's JIT keeps comptime within the IR level where type
information is available.

### Backend (LLVM)

The backend lowers IR to LLVM IR for optimization and machine code emission.

**Target query interface** — the backend exposes an abstract interface for target-specific
information (type sizes, alignment, struct layout, calling conventions). The frontend uses this
during semantic analysis to make layout decisions.

## TableGen

We use [LLVM TableGen](https://llvm.org/docs/TableGen/index.html) for generating repetitive C++
definitions from declarative `.td` source files. TableGen keeps the source of truth in a single
structured file, eliminating hand-maintained enums, lookup tables, and switch statements that
would otherwise drift out of sync.

### Setup

- **`.td` files** live alongside the code that uses them (e.g. `src/Lexer/Tokens.td`).
- **Custom backends** live in `tools/tblgen/`. Each backend implements an emitter function
  declared in `Generators.hpp`. The shared `main.cpp` dispatches to the selected generator
  via the `--gen=<name>` command-line flag.
- **`lbc-tblgen`** is a single build tool binary, linked against `LLVMTableGen` and
  `LLVMSupport` via the `configure_tblgen()` CMake function. Adding a new generator means
  adding an emitter function, a `Generator` enum value in `main.cpp`, and a declaration in
  `Generators.hpp`.
- **`add_tblgen()`** is a CMake function (defined in `cmake/tblgen.cmake`) that wires
  generation into the build. Each `GENERATOR`/`INPUT` group produces one `.inc` file. Usage:

  ```cmake
  add_tblgen(<target> <tblgen_tool>
      GENERATOR <gen-name> INPUT <path/to/Foo.td> [OUTPUT <path/to/Foo.inc>]
      GENERATOR <gen-name> INPUT <path/to/Bar.td>
  )
  ```

  Paths are relative to the calling `CMakeLists.txt`. If `OUTPUT` is omitted, the `.inc`
  filename is derived from the input (`.td` → `.inc`). Each entry passes `--gen=<gen-name>`
  to the tool. Generated files are formatted with clang-format and placed under the build
  directory. The generated include directory is added to the target automatically.

- **`.inc` files** are self-contained generated headers with `#pragma once`, includes,
  and namespace wrapping. A thin hand-written header (e.g. `Token.hpp`) includes the
  `.inc` for use by the rest of the codebase.
- **`Builder`** (`tools/tblgen/Builder.hpp`) is a lightweight helper for emitting
  formatted C++ from TableGen backends. Provides RAII namespace scoping, indentation
  tracking, block/line helpers, and a fluent interface.

### Current uses

- `src/Lexer/Tokens.td` — token kinds, groups, operator properties (precedence,
  associativity, category). Generates a `TokenKind` struct with query methods,
  `std::hash` and `std::formatter` specializations.
- `src/Ast/Ast.td` — AST node hierarchy (base, group, leaf nodes), member fields,
  and custom code blocks. Generates node class definitions (`lbc-ast-def`), forward
  declarations (`lbc-ast-fwd-decl`), and the visitor infrastructure (`lbc-ast-visitor`).
- `src/Diag/Diagnostics.td` — diagnostic message catalog (see Diagnostics below).
  Generates the diagnostic enum and log function declarations (`lbc-diag-def`) and
  the format string tables and log function implementations (`lbc-diag-impl`).

## Diagnostics

Compiler passes log diagnostics through a central `DiagEngine`, which stores them in a
vector and returns an opaque `DiagIndex` handle. That handle is the error type in
`Result<T>` = `std::expected<T, DiagIndex>` — trivially copyable (4 bytes), propagated
by the `TRY` / `TRY_ASSIGN` / `TRY_DECL` / `MUST` macros unchanged. The engine owns
all diagnostic details; the error return path carries only the index.

### Flow

1. Pass calls `DiagEngine::log(diagKind, location, args...)` → gets `DiagIndex`.
2. Returns `DiagIndex` via `std::unexpected` / `Result<T>`.
3. Callers propagate via `TRY` or handle at a recovery point.
4. Accumulated diagnostics are rendered after the pass completes.

### Definitions (`Diagnostics.td`)

All diagnostics live in a single TableGen file. Each entry specifies severity
(`error`/`warning`/`note`), category (`System`/`Lex`/`Parse`/`Sema`/`IR`/`LLVM`),
a stable diagnostic code, and a format string with typed placeholders (`{name}` or
`{name:type}`). Generated by `lbc-diag-def` (header) and `lbc-diag-impl` (implementation).

## C Interoperability

The language aims for seamless C interop at the ABI level:

- All C primitive types with matching sizes and alignment
- Bitfields and packed structs with C-compatible layout
- C calling conventions
- C libraries are used by importing declaration headers (type and function signatures only,
  no glue code)

Target-specific layout rules come from the backend's target query interface, ensuring correctness
across platforms.

## Implementation Language

- C++23, targeting GCC, Clang, and MSVC
- No C++20 modules or `import std;` (too buggy)
- No exceptions (`-fno-exceptions`), no RTTI (`-fno-rtti`)
- Error handling via `std::expected`
- RAII for all resource management
