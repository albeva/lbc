# LBC Design

Design notes for lbc — a BASIC compiler with full C ABI compatibility and an LLVM backend.

## Goals

- Fully functional BASIC compiler, not a toy
- Full C ABI compatibility: data types, bitfields, packed structs, calling conventions
- Direct interop with C libraries without wrapper code
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

### Frontend (pure C++23, no LLVM)

Lexer, parser, AST, semantic analysis, and diagnostics are entirely self-contained with no LLVM dependencies. Source management and error reporting are implemented from scratch using standard C++23.

### IR

A typed, high-level intermediate representation that sits between the AST and LLVM IR. Unlike LLVM IR, it preserves full semantic information:

- **Types** are first-class — generics, user-defined types, and type parameters are all represented
- **Serializable** — serves as the module interface format for `import`
- **Interpretable** — designed to be executed by the comptime VM

The IR carries no optimization passes. Its purpose is to preserve semantics, not to transform code. All optimization is delegated to LLVM.

### Comptime VM

A simple virtual machine that executes IR at compile time. Operates on the IR directly where types are still first-class, enabling:

- Compile-time function evaluation (constexpr-style)
- Compile-time type construction (Zig-style `comptime` — functions that return types)
- Template instantiation driven by comptime evaluation

Using a custom VM rather than LLVM's JIT keeps comptime within the IR level where type information is available, and avoids pulling LLVM into the frontend.

### Backend (LLVM, isolated)

The backend lowers IR to LLVM IR for optimization and machine code emission. LLVM headers and types do not leak into the frontend or IR.

**Target query interface** — the backend exposes an abstract interface for target-specific information (type sizes, alignment, struct layout, calling conventions). The frontend uses this during semantic analysis to make layout decisions. This is the only way the frontend communicates with LLVM, keeping the dependency narrow and swappable.

### Future Backends

The frontend and IR are LLVM-free by design. Alternative backends (WASM, C codegen) can be added without touching anything upstream of the IR.

## C Interoperability

The language aims for seamless C interop at the ABI level:

- All C primitive types with matching sizes and alignment
- Bitfields and packed structs with C-compatible layout
- C calling conventions
- C libraries are used by importing declaration headers (type and function signatures only, no glue code)

Target-specific layout rules come from the backend's target query interface, ensuring correctness across platforms.

## Implementation Language

- C++23, compiled with GCC and Clang (MSVC intentionally unsupported)
- GNU extensions are acceptable
- No C++20 modules or `import std;` (too buggy)
- No exceptions (`-fno-exceptions`), no RTTI (`-fno-rtti`)
- Error handling via `std::expected`
- RAII for all resource management
