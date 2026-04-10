# CLAUDE.md

Guidance for Claude Code (claude.ai/code) working with this repo.

## Project Overview

lbc = BASIC compiler, LLVM backend, C++23. Targets GCC, Clang, MSVC. No C++20 modules/`import std;` — too buggy. Exceptions+RTTI disabled on GCC/Clang (`-fno-exceptions -fno-rtti`); MSVC defaults. CMake+Ninja build, Google Test.

## Your Role

Rules always followed:

- Primary purpose: help, advise, review code I write.
- I write code, you critique/feedback.
- No code generation unless explicitly asked.
- When writing code, minimum viable scope only.
- Large task/refactor → stop, ask how to proceed.
- No co-authorship mentions in commits.
- Never push to git repo, commit only.

## Build Commands

```bash
# Configure (from project root, using default compiler)
cmake -G Ninja -B build/claude -DCMAKE_BUILD_TYPE=Debug

# Build everything
ninja -C build/claude

# Build just the library
ninja -C build/claude lbc_lib

# Build just tests
ninja -C build/claude tests
```

Main executable → `bin/lbc`.

## Testing

```bash
# Run all tests
./build/claude/tests/tests

# Run a single test by name
./build/claude/tests/tests --gtest_filter="BasicTests.IsTrue"
```

Google Test v1.17.0, fetched via CMake FetchContent.

## Code Formatting and Linting

- **clang-format**: WebKit-based style (`.clang-format`)
- **clang-tidy**: Comprehensive checks, warnings-as-errors (`.clang-tidy`)

## Architecture

- `src/` — Main source. Static library (`lbc_lib`) + thin executable (`lbc`) linking against it.
- `tests/` — Google Test suite, links `lbc_lib`.
- `tools/tblgen/` — Custom LLVM TableGen backends. Single `lbc-tblgen` binary, selects generator via `--gen=<name>`. Shared infra in `lib/` (namespace `lib`): `Builder` = C++ codegen helpers; `GeneratorBase` extends with `RecordKeeper` access + utilities (`sortedByDef`, `findRange`, `collect`, `contains`). `TreeGenBase` handles Node/Group/Leaf lookups + shared codegen (kind enum, forward decls, class defs w/ constructors, RTTI classof, accessors, data members); `TreeGen<ClassT, ArgT>` = template building in-memory `TreeNode` tree from `.td` defs via virtual factory methods (`makeNode`, `makeArg`). `TreeNode`/`TreeNodeArg` model hierarchy+fields; `TreeNode::getClassName()`/`getBaseClassName()` virtual, allowing subclass overrides (e.g., `IrNodeClass`). Concrete generators in subdirs (`ast/`, `ir/`, `diag/`, `tokens/`, `type/`) extend `GeneratorBase`/`TreeGen`. Generated `.hpp` emitted alongside source `.td` files (e.g., `src/Lexer/TokenKind.td` → `src/Lexer/TokenKind.hpp`), git-tracked.
- `cmake/` — Build config: `options.cmake` (compiler flags), `warnings.cmake` (warnings-as-errors), `llvm.cmake` (LLVM integration), `tblgen.cmake` (TableGen custom command helper).
- `configured_files/` — CMake-generated headers (version/metadata via `config.hpp.in`).
- `src/pch.hpp` — Precompiled header shared across `lbc_lib`, `lbc`, `tests`. Common STL headers here.

### Compiler Pipeline

Frontend (lexer, parser, AST, sema) → IR → Backend (LLVM IR → machine code).

- **LLVM usage** — prefer std C++ where natural, freely use LLVM tools/libs throughout entire codebase. No artificial isolation.
- **IR** — typed, serializable, interpretable intermediate representation. `src/IR/` with two subdirs: `lib/` (namespace `lbc::ir::lib`) = core IR classes (values, blocks, instructions, builder, module); `gen/` (namespace `lbc::ir::gen`) = `IrGenerator` lowering analysed AST to IR. Basic blocks w/ branch instructions for control flow (not structured if/loop). Preserves semantic info (types, generics, module interfaces) + lexical scope boundaries (via `ScopedBlock`) LLVM IR discards. Retain/release = explicit instructions; destructors implicit at scope boundaries from type metadata. Value hierarchy: `Value` → `NamedValue` → `Temporary`/`Variable`/`Function`/`Block`. `Variable`/`Function` hold `Symbol*` back-reference for debug/diagnostics. Blocks: `BasicBlock` (flat instruction list) or `ScopedBlock` (child blocks + cleanup + `ValueTable`; parent-chained for scope resolution). `SymbolTableBase<T>` = generic name→value mapping used by `SymbolTable` (frontend, maps to `Symbol`) and `ValueTable` (IR, maps to `NamedValue`). Containers use `llvm::ilist` for ownership. Instruction classes + `Builder` generated from `src/IR/lib/Instructions.td` via `lbc-tblgen`. No optimization passes — all optimization delegated to LLVM.

## Code Conventions

- C++23, no exceptions, no RTTI, no modules/`import std;`
- No `noexcept` — redundant, exceptions globally disabled
- Target GCC, Clang, MSVC
- Classes `final` by default
- `[[nodiscard]]` on function decls where appropriate
- Prefer `llvm::StringRef` for string refs; `std::string_view`/`"text"sv` when LLVM types unavailable
- PascalCase classes, camelCase functions, lowercase namespaces
- Root namespace: `lbc`
- 4-space indent, LF endings
- Max 120 chars/line in docs+source

## Documentation Style

- `/** */` for functions, classes, enum types, other declarations
- `///` only for data members + enum cases
- `@`-style commands (`@param`, `@return`, etc.), never backslash style (`\p`, `\c`, `\a`, etc.)

## Important Patterns

- **Error handling**: `DiagResult<T>` = `std::expected<T, DiagIndex>` for fallible ops. `DiagIndex` = opaque handle into `DiagEngine` storage — lightweight propagation, engine owns diagnostic details. `TRY`/`TRY_ASSIGN`/`TRY_DECL`/`MUST` macros (SerenityOS/Ladybird-inspired) for ergonomic error propagation. `LogProvider` = CRTP-free mixin using C++23 deducing this — any class satisfying `ContextAware` concept (exposes `getContext() -> Context&`) inherits `diag()` helper that logs+returns `DiagError`.
- **Diagnostics**: generated from `src/Diag/Diagnostics.td` via `lbc-tblgen --gen=lbc-diag-def` → `src/Diag/Diagnostics.hpp`. `DiagKind` = smart enum struct: `Value` enum lists every diagnostic, constexpr member functions (`getCategory()`, `getSeverity()`, `getCode()`) encode static metadata via switch tables. `getSeverity()` returns `llvm::SourceMgr::DiagKind` directly (`DK_Error`, `DK_Warning`, `DK_Note`, `DK_Remark`). `DiagMessage` = `std::pair<DiagKind, std::string>`. Factory functions in `namespace diagnostics` parse `{name}`/`{name:type}` placeholders in `.td` message strings → typed parameters. Consteval helpers (`allErrors()`, `allWarnings()`, `allNotes()`) group diagnostics by severity at compile time.
- **Memory**: RAII everywhere, no manual new/delete. `Context` owns `llvm::BumpPtrAllocator` arena — AST nodes+spans allocated via `Context::create<T>()`/`Context::span<T>()`. `Sequencer<T>` collects AST nodes into intrusive singly-linked list during parsing, flattens to contiguous arena-allocated `std::span` via `Sequencer::sequence(Context&)`.
- **Parser**: single `Parser` class, impl split across `.cpp` files by concern (`ParseDecl.cpp`, `ParseExpr.cpp`, `ParseStmt.cpp`, `ParseType.cpp`, `Parser.cpp` for common utils).
- **Semantic Analyser**: single `SemanticAnalyser` class, same split-file pattern (`Sema.cpp`, `SemaDecl.cpp`, `SemaExpr.cpp`, `SemaStmt.cpp`, `SemaType.cpp`). Inherits `LogProvider` for diagnostics + `AstVisitor<DiagResult<void>>` for dispatch. Each AST node type has `accept()` handler. Entry: `analyse(AstModule&)`.
- **IR Generator**: `IrGenerator` in `lbc::ir::gen`, same split-file pattern (`Gen.cpp`, `GenDecl.cpp`, `GenExpr.cpp`, `GenStmt.cpp`, `GenType.cpp`). Inherits `AstVisitor<DiagResult<void>>` for AST traversal, `ir::lib::Builder` for instruction creation, `LogProvider` for diagnostics. Entry: `generate(const AstModule&) -> DiagResult<lib::Module*>`.
- **AST**: nodes generated from `src/Ast/Ast.td` via `lbc-tblgen --gen=lbc-ast-def`. TableGen schema: `Node` (base), `Group` (abstract intermediate — types, stmts, decls, exprs), `Leaf` (concrete instantiable). Each node has `list<Member>` w/ two subtypes: `Arg` (data fields — type, name, optional default+mutable flag) and `Func` (custom code blocks emitted verbatim). `Arg` fields w/o default → constructor params; w/ default → initialized fields. `mutable` bit controls setter generation. `Func` blocks auto-dedented via `unindent()`. `AstKind` enum values grouped by parent for range-check group membership. All nodes inherit `llvm::SMRange` + `next` pointer from `AstRoot`; `next` = intrusive linked list during parsing for collecting nodes before bulk-allocating into `std::span`.
- **AST Visitor**: generated from `Ast.td` via `lbc-tblgen --gen=lbc-ast-visitor` → `src/Ast/AstVisitor.hpp`. C++23 deducing this (`this auto&`) for static dispatch — no CRTP. Generates top-level `AstVisitor` + per-group visitors (`AstExprVisitor`, `AstStmtVisitor`, `AstDeclVisitor`, `AstTypeVisitor`). Dispatch = `visit()` (switch on `AstKind`), handlers = `accept()` (implemented by derived class). Const propagation automatic through `auto&` params + `llvm::cast`. Generic `accept(const auto&)` catch-all for unimplemented nodes. `AstVisitorGen` inherits `AstGen` to reuse AST class hierarchy.