# Type System {#type_system}

## Type

@ref lbc::Type "Type" is the base representation for all types in the compiler.

## TypeFactory

@ref lbc::TypeFactory "TypeFactory" is the central registry for creating and
retrieving types. It is owned by @ref lbc::Context "Context" and ensures type
uniqueness — each distinct type exists as a single instance.

Types are constructed exclusively through the factory; direct construction is
restricted.
