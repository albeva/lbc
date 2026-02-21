# Diagnostics {#diagnostics}

Compiler passes log diagnostics through @ref lbc::DiagEngine "DiagEngine",
which stores them and returns an opaque @ref lbc::DiagIndex "DiagIndex" handle.

## Key Types

| Type | Description |
|------|-------------|
| @ref lbc::DiagEngine "DiagEngine" | Accumulates diagnostics, pairs each with an `llvm::SMDiagnostic` for rendering |
| @ref lbc::DiagIndex "DiagIndex" | Opaque 4-byte handle into engine storage |
| `DiagResult<T>` | Alias for `std::expected<T, DiagIndex>` |
| `DiagError` | Alias for `std::unexpected<DiagIndex>` |
| `DiagKind` | Generated smart enum with severity, category, and code metadata |

## Error Propagation

`TRY` / `TRY_ASSIGN` / `TRY_DECL` / `MUST` macros provide ergonomic
propagation of `DiagResult` errors, inspired by SerenityOS/Ladybird.

## LogProvider

@ref lbc::LogProvider "LogProvider" is a mixin using C++23 deducing this.
Any class satisfying the `ContextAware` concept (exposes `getContext()`)
inherits a `diag()` helper that logs a message and returns `DiagError` in
one step.

## Diagnostic Definitions

All diagnostics are defined in `src/Diag/Diagnostics.td` with severity,
category, a stable code, and format strings with typed placeholders. The
`lbc-diag-def` generator emits type-safe factory functions.
