# IR — Intermediate Representation {#ir}

## Overview

The IR is a typed, high-level intermediate representation between the AST and
LLVM IR. It sits at a similar abstraction level to Swift's SIL — high enough to
preserve full semantic information (types, generics, ownership) but structured
enough to lower cleanly to LLVM IR.

## Goals

- **Serializable** — binary format for module interfaces (`import`). A module's
  public API (type declarations, function signatures, inlineable function
  bodies, constants) is serialized as IR and shipped alongside compiled output.
- **Interpretable** — designed for execution by the comptime VM. The IR
  instruction set maps directly to VM opcodes.
- **Template instantiation** — generic function/type bodies are stored in IR
  form. When instantiated with concrete types, the IR is cloned and specialized.
  e.g. `Function Max<T: Comparable>(a As T, b As T) As T`.
- **Public interface extraction** — the IR preserves enough information to
  extract a module's public interface, including inlineable function bodies and
  constants.
- **LBC-specific optimizations** — the IR is the place for language-level
  transformations such as reference counting (retain/release insertion), not
  low-level optimizations like constant folding or dead code elimination.
- **Easy lowering to LLVM IR** — the IR is structured so that each instruction
  maps straightforwardly to LLVM IR.

## Non-Goals

- SSA form, phi nodes, or register allocation — delegated to LLVM.
- Low-level optimizations (constant folding, DCE, inlining) — delegated to LLVM.
- Text-based input — the IR supports text printing for debugging, but there is
  no IR lexer or parser. The canonical form is binary.

## Design

### Abstraction Level

The IR is **instruction-based** with **named variables** and **numbered
temporaries**. Control flow uses **basic blocks with jumps** — all BASIC control
flow constructs (IF/ELSE, DO/LOOP, WHILE/WEND, FOR/NEXT) are lowered into
conditional and unconditional branches between labeled blocks.

- Source-level variables appear as `var name: Type` declarations.
- Expressions are decomposed into instructions that produce numbered temporaries
  (`%0`, `%1`, ...). Temp numbering resets per function.
- Control flow is lowered to basic blocks and branch instructions: conditional
  branches (`cond_br`), unconditional branches (`br`), and returns (`ret`).

### Class Hierarchy

#### Values

All IR entities that can be referenced inherit from `Value`, which carries a
`Kind` discriminator for LLVM-style RTTI and a `Type*` from the compiler's
type system.

```
Value (base — kind + type)
├── NamedValue (adds a name — satisfies the Named concept)
│   ├── Temporary (%0, %1, ...)
│   ├── Variable (named locals/globals)
│   ├── Function (name, symbol, block list)
│   └── Block (abstract — labeled unit of control flow)
│       ├── BasicBlock (flat instruction list)
│       └── ScopedBlock (child blocks + cleanup + value table)
└── Literal (unnamed compile-time constant)
```

#### Containers

The IR has a layered container structure:

```
Module
├── declarations (extern functions, global variables, type declarations)
└── functions
    └── blocks (BasicBlock | ScopedBlock)
        └── instructions (flat, uniform — future TableGen candidates)
```

- **Module** — root container, holds all top-level declarations and functions.
- **Function** — name, parameters, return type, body as a list of blocks. The
  first block is the entry point.
- **Block** — abstract base. Concrete subclasses:
  - **BasicBlock** — a labeled, straight-line sequence of instructions ending
    with a terminator (branch, conditional branch, or return).
  - **ScopedBlock** — a group of child blocks sharing a lexical scope, with
    an optional cleanup block and a ValueTable for named values. Used for
    managing object lifetimes (retain/release, destructors).
- **Instruction** — placeholder for individual IR instructions (future:
  TableGen-defined with opcodes, operands, and visitor dispatch).

### Scoped Blocks and Object Lifetimes

Unlike LLVM IR, the LBC IR preserves lexical scope information. This is
essential for:

- **Reference counting** — retain/release are explicit IR instructions that
  can be optimized (elide redundant pairs, sink releases, etc.).
- **Destructors** — called implicitly at scope exit, derived from type
  metadata. The IR does not emit explicit destructor calls; the VM and LLVM
  lowering pass infer them from the scope structure and type information.
- **Template instantiation** — scope structure is preserved through
  serialization, allowing correct cleanup generation for instantiated types.

A ScopedBlock provides three layers of cleanup:

1. **Explicit retain/release** — instructions within the body blocks that
   manipulate reference counts at specific program points.
2. **Cleanup block** — an optional block of instructions (e.g. release
   operations) that runs before any scope exit (return, branch to outer block).
3. **Implicit destructor/dealloc** — at the scope boundary, the VM/lowering
   pass walks the scope's variables and calls destructors for types that have
   them.

Example:

```
func foo(z: bool): integer {
entry:
    cond_br z, @true, @false

scoped true: {
    var obj: Object
    obj = call getObj()
    retain obj
    %0 = field obj, res
    ret %0
} cleanup {
    release obj
}

scoped false: {
    var obj2: Object
    obj2 = call getObj2()
    retain obj2
    %1 = field obj2, res
    ret %1
} cleanup {
    release obj2
}
}
```

### Value Table

Each ScopedBlock owns a `ValueTable` — a `SymbolTableBase<NamedValue>` that
maps names to IR values within that scope. Value tables chain via parent
pointers, mirroring the nesting of scoped blocks, so name lookups walk
outward through enclosing scopes.

### Type System

The IR reuses the compiler's existing type system (`Type`, `TypeIntegral`,
`TypeFloatingPoint`, `TypePointer`, etc.) rather than defining its own. IR nodes
carry `Type*` pointers from the same `TypeFactory`. The `Label` sentinel type
is used for block values.

### Memory Model

IR nodes use `llvm::ilist` (intrusive linked lists) for ownership of functions,
blocks, and instructions. This matches LLVM's own IR ownership model and
provides efficient insertion, removal, and iteration.

### Textual Form

C-like syntax with `{ }` for scoping. For debugging and test output only — no
parser.

```
declare func puts(msg: ZString)

func main() {
entry:
    var hello: ZString = "Hello "
    var world: ZString = "world"
    call puts(hello)
    call puts(world)
    call puts("!")
    ret
}
```

Expressions decompose into instructions with temporaries:

```
entry:
    var a: Integer = 1
    var b: Integer = 2
    var c: Integer = 3
    %0 = mul b, c
    %1 = add a, %0
    var x: Integer = %1
```

### Binary Serialization

The binary format is the canonical IR representation. It is used for:

- Module interface files (public declarations + inlineable bodies)
- Caching compiled modules
- Input to the comptime VM

Top-level container serialization is hand-written (few types, complex
structure). Instruction serialization is generated from TableGen definitions.

## TableGen Integration

Instructions are defined in a `.td` file (e.g. `src/IR/Instructions.td`). The
TableGen backend(s) generate:

- Instruction node classes with fields and accessors
- Visitor infrastructure (dispatch + accept methods)
- Binary serialization / deserialization code
- Text printer support

Top-level containers (`Module`, `Function`, `Block`, `ScopedBlock`) are
hand-written, as there are only a few of them and each has unique structure.

## Compiler Pipeline Integration

```
AST (after Sema) → IR → LLVM IR → Machine Code
                   ↕
                Comptime VM
```

### Pass 1: AST → IR (Lowering)

An `AstVisitor`-based pass walks the semantically annotated AST and emits IR.
This pass:

- Decomposes expressions into instruction sequences with temporaries
- Lowers all control flow to basic blocks and branch instructions
- Wraps lexical scopes in ScopedBlocks with cleanup blocks
- Preserves full type information on every value and instruction
- Emits explicit retain/release for reference-counted types

### Pass 2: IR → LLVM IR (Code Generation)

Walks IR containers and instructions, emitting LLVM IR:

- Maps IR types to LLVM types
- Maps IR instructions to LLVM instructions
- Materializes cleanup blocks as real basic blocks with branch rewrites
- Emits implicit destructor calls at scope boundaries
- Sets up target machine, data layout, calling conventions
- Emits object files or LLVM IR text (`.ll`) for debugging

## Prior Art

The LBC IR design draws from several compiler IRs that sit between AST and a
low-level backend. This section documents how they compare and which ideas
informed our choices.

### Swift SIL

The closest analogue in purpose. SIL (Swift Intermediate Language) preserves
Swift's full type system, reference counting semantics (ARC), generic type
parameters, and protocol witness tables. It exists in three forms: in-memory,
textual (`.sil`), and binary (`.swiftmodule` for distributing inlineable/generic
code). SIL is SSA-based with a CFG of basic blocks. SIL carries the heaviest
optimization burden in the Swift compiler: mandatory passes (definite
initialization, memory promotion, ARC optimization), devirtualization, and
generic specialization all happen at the SIL level before lowering to LLVM IR.
Swift also has a limited SIL-based interpreter for compile-time constant
evaluation. Instructions are defined via C++ classes with X-macro `.def` files,
not TableGen.

A key difference: SIL resolves scope information entirely during SILGen
(AST → SIL lowering) using an internal `CleanupStack`. By the time SIL exists,
all cleanup is explicit instructions — no scope boundaries remain. LBC IR
preserves scope structure because the VM, template instantiation, and
serialization benefit from knowing scope boundaries.

### Rust MIR

MIR deliberately chose **against pure SSA** because the borrow checker operates
on memory paths, not abstract values. Variables are indexed (`_0`, `_1`) rather
than named. Control flow uses a CFG with basic blocks. All expressions are fully
decomposed into three-address code — nesting is prohibited. MIR is serialized
into `.rmeta` for cross-crate inlining and const evaluation. Rust's **Miri**
interpreter executes MIR directly, powering `const fn` evaluation and UB
detection. Instructions are hand-written Rust enums. MIR runs optimizations
after borrow checking (inlining, const propagation, DCE) but delegates heavy
optimization to LLVM.

### Zig ZIR / AIR

Zig uses **two** IRs. ZIR is an untyped syntactic lowering from the AST —
immutable and reusable across generic instantiations and comptime calls.
Serialized as flat `u32` arrays for incremental compilation. AIR is the typed,
per-function IR produced after semantic analysis, where comptime evaluation
happens: expressions with compile-time-known values are folded, others become
runtime instructions. Both are SSA and instruction-indexed. Instructions are
hand-written Zig enums with a tag+data+extra encoding. Zig's comptime is
fundamental to the language and the primary driver of ZIR/AIR design.

### Kotlin IR

A **tree-based** (not instruction-based) representation that closely mirrors
source structure. Preserves full Kotlin types, generics, annotations, and
coroutine markers. Uses structured control flow (tree nodes, not a CFG).
Serialized into `.klib` archives for Kotlin Multiplatform distribution.
Optimization is done through progressive tree-to-tree **lowering passes** that
desugar high-level constructs — more like AST transformations than traditional
optimization. No comptime interpreter. Instructions are hand-written Kotlin
classes.

### Design Choices in Context

| | SIL | MIR | Zig | Kotlin | **LBC** |
|---|---|---|---|---|---|
| SSA | Yes | No | Yes | No | **No** |
| Control flow | CFG | CFG | Structured | Structured | **CFG** |
| Scope info | No (resolved at gen) | No | No | Yes (tree) | **Yes (ScopedBlock)** |
| Variables | Registers | Indexed | Indexed | Named (tree) | **Named + %temps** |
| Serializable | Yes | Partial | Yes (ZIR) | Yes | **Yes** |
| Interpretable | Limited | Yes (Miri) | Yes | No | **Yes (planned)** |
| Generics | Yes | Yes | Yes | Yes | **Yes (planned)** |
| Instruction defs | C++ .def | Enums | Enums | Classes | **TableGen** |
| Lang-level opts | ARC, devirt | Borrow ck | Comptime | Desugaring | **Refcount** |

**Key takeaways that informed our design:**

- **Non-SSA is well-precedented.** Rust and Kotlin both chose against SSA for
  good reasons. Our motivations (VM interpretability, serialization simplicity,
  readability) are equally valid — LLVM constructs SSA anyway.
- **CFG with scope preservation** is our unique combination. Unlike SIL/MIR
  (pure CFG, no scopes) and Kotlin/Zig (structured, no CFG), we use basic
  blocks for control flow but preserve scope boundaries via ScopedBlock for
  object lifetime management.
- **TableGen for instruction definitions** is unique among these compilers —
  everyone else hand-writes their IR nodes. Our existing TableGen infrastructure
  makes this a natural fit and yields generated visitors, serialization, and
  printing that others maintain by hand.
- **Every compiler doing comptime** (Rust/Miri, Zig, Swift) found it to be a
  major complexity driver. The IR should be VM-friendly from day one, but the
  actual VM can wait.
- **Serialization correlates with module system needs.** Languages needing
  cross-module optimization (Swift, Kotlin, Zig) serialize their IR. This
  validates our binary serialization plan for module interfaces.

## Implementation Plan

### Phase 1: Foundation (in progress)

1. ~~Design and implement hand-written containers (`Module`, `Function`,
   `Block`, `BasicBlock`, `ScopedBlock`).~~ Done.
2. ~~Value hierarchy (`Value`, `NamedValue`, `Temporary`, `Literal`).~~ Done.
3. ~~ValueTable for scoped name resolution.~~ Done.
4. ~~Label sentinel type for block values.~~ Done.
5. Design the instruction TableGen schema (`src/IR/Instructions.td`).
6. Write tblgen backend(s) for instruction codegen (`--gen=lbc-ir-def`,
   `--gen=lbc-ir-visitor`).
7. Implement generated instruction nodes.

### Phase 2: AST → IR Lowering

8. Implement the lowering pass (AstVisitor-based).
9. Starting instruction set: function declaration, variable declaration,
   function call — enough for hello world.
10. IR text printer for debugging.

### Phase 3: IR → LLVM IR

11. Implement LLVM IR code generation pass.
12. Type mapping (IR types → LLVM types).
13. Target machine setup and object file emission.
14. Wire into the compiler driver.

### Phase 4: Expansion (Post Hello World)

15. Arithmetic / comparison instructions.
16. Control flow instructions (branch, conditional branch).
17. Reference counting (retain/release).
18. Binary serialization (tblgen-generated for instructions, hand-written for
    containers).
19. Module interface extraction.
20. Template/generic support in IR form.
