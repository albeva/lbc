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
temporaries**. It preserves structured control flow (no arbitrary gotos).

- Source-level variables appear as `var name: Type` declarations.
- Expressions are decomposed into instructions that produce numbered temporaries
  (`%0`, `%1`, ...). Temp numbering resets per function.
- Control flow is generalized: all BASIC loop forms (`DO/LOOP`, `WHILE/WEND`,
  `FOR/NEXT`) lower to a single uniform `loop` construct. Similarly, all
  conditional forms lower to a single `if` construct.

### Two-Level Structure

The IR has two levels of structure:

1. **Top-level containers** — hand-written C++ classes. Few in number,
   structurally unique, each with distinct semantics:
   - **Module** — root container, holds all top-level declarations.
   - **Function** — name, parameters, return type, body (instruction list),
     linkage/visibility. May be generic (uninstantiated).
   - **Type declaration** — user-defined types/classes with full field names,
     types, and methods.
   - **External declaration** — imported C functions or other external symbols.

2. **Instructions** — defined via TableGen (`.td`). Uniform shape (opcode,
   operands, result), many of them, high payoff from generated code:
   - Visitor dispatch
   - Binary serialization / deserialization
   - Text printer
   - VM opcode mapping

This mirrors LLVM's own architecture: `Module`, `Function`, `BasicBlock` are
hand-written, while the instruction set is TableGen-defined.

### Type System

The IR reuses the compiler's existing type system (`Type`, `TypeIntegral`,
`TypeFloatingPoint`, `TypePointer`, etc.) rather than defining its own. IR nodes
carry `Type*` pointers from the same `TypeFactory`.

### Memory Model

IR nodes are arena-allocated via `Context::create<T>()`, same as AST nodes.
Top-level containers own their instruction sequences.

### Textual Form

C-like syntax with `{ }` for scoping. For debugging and test output only — no
parser.

```
declare func puts(msg: ZString)

func main() {
    var hello: ZString = "Hello "
    var world: ZString = "world"
    call puts(hello)
    call puts(world)
    call puts("!")
}
```

Expressions decompose into instructions with temporaries:

```
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

Top-level containers (`Module`, `Function`, `TypeDecl`, `ExternDecl`) are
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
- Generalizes control flow (all loops → uniform `loop`, all conditionals → `if`)
- Preserves full type information on every value and instruction
- Emits explicit retain/release for reference-counted types (future)

### Pass 2: IR → LLVM IR (Code Generation)

Walks IR containers and instructions, emitting LLVM IR:

- Maps IR types to LLVM types
- Maps IR instructions to LLVM instructions
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
code). SIL is SSA-based with a CFG of basic blocks — unlike our IR, which uses
named variables and structured control flow. SIL carries the heaviest
optimization burden in the Swift compiler: mandatory passes (definite
initialization, memory promotion, ARC optimization), devirtualization, and
generic specialization all happen at the SIL level before lowering to LLVM IR.
Swift also has a limited SIL-based interpreter for compile-time constant
evaluation. Instructions are defined via C++ classes with X-macro `.def` files,
not TableGen.

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
| Control flow | CFG | CFG | Structured | Structured | **Structured** |
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
- **Structured control flow** is the less common choice (only Zig and Kotlin),
  but it is simpler to serialize, simpler to interpret in a VM, and maps
  naturally to BASIC's control flow.
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

### Phase 1: Foundation

1. Design and implement hand-written top-level containers (`IRModule`,
   `IRFunction`, `IRExternDecl`, `IRTypeDecl`).
2. Design the instruction TableGen schema (`src/IR/Instructions.td`).
3. Write tblgen backend(s) for instruction codegen (`--gen=lbc-ir-def`,
   `--gen=lbc-ir-visitor`).
4. Implement generated instruction nodes.

### Phase 2: AST → IR Lowering

5. Implement the lowering pass (AstVisitor-based).
6. Starting instruction set: function declaration, variable declaration,
   function call — enough for hello world.
7. IR text printer for debugging.

### Phase 3: IR → LLVM IR

8. Implement LLVM IR code generation pass.
9. Type mapping (IR types → LLVM types).
10. Target machine setup and object file emission.
11. Wire into the compiler driver.

### Phase 4: Expansion (Post Hello World)

12. Arithmetic / comparison instructions.
13. Control flow instructions (`if`, `loop`).
14. Reference counting (retain/release).
15. Binary serialization (tblgen-generated for instructions, hand-written for
    containers).
16. Module interface extraction.
17. Template/generic support in IR form.
