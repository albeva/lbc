# lbc {#mainpage}

A BASIC compiler with an LLVM backend, written in modern C++23.

The goal is to build a modern language with BASIC-inspired syntax that is fully
C ABI compatible — allowing direct interop with C libraries without wrappers or
bindings.

## Compiler Pipeline

```
Source → Lexer → Parser → AST → Semantic Analysis → IR → LLVM IR → Machine Code
                                                     ↕
                                                  Comptime VM
```

- @ref lbc::Lexer "Lexer" tokenizes source into @ref lbc::Token "Tokens"
- @ref lbc::Parser "Parser" builds the @ref ast "AST"
- @ref lbc::SemanticAnalyser "SemanticAnalyser" validates and resolves types
- IR preserves semantic information that LLVM IR discards
- LLVM backend handles optimization and code generation

## Key Subsystems

- @subpage ast — Node hierarchy, kinds, and visitor infrastructure
- @subpage diagnostics — Error reporting and propagation
- @subpage type_system — Type representation and construction
- @subpage design — Goals, IR, comptime VM, and C interop
