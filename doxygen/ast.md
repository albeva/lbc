# AST {#ast}

The AST is generated from `src/Ast/Ast.td` via `lbc-tblgen --gen=lbc-ast-def`.

## Node Hierarchy

Three class types in the TableGen schema:

- **Node** — base class (`AstRoot`), carries an `llvm::SMRange`
  and an intrusive `next` pointer
- **Group** — abstract intermediate nodes (types, statements, declarations,
  expressions)
- **Leaf** — concrete instantiable nodes (e.g. `AstModule`,
  `AstLiteralExpr`)

`AstKind` enum values are grouped by parent so range checks can determine
group membership.

## Visitor

Generated via `lbc-tblgen --gen=lbc-ast-visitor`. Uses C++23 deducing this
(`this auto&`) for static dispatch — no CRTP needed. Dispatch is via `visit()`
(switch on `AstKind`), handlers are `accept()` methods implemented by the
derived class.

## Memory

Nodes are arena-allocated via @ref lbc::Context::create "Context::create\<T\>()".
During parsing, @ref lbc::Sequencer "Sequencer\<T\>" collects nodes into an
intrusive linked list, then flattens them into a contiguous `std::span` via
@ref lbc::Sequencer::sequence "Sequencer::sequence()".
