DynamicArray TODO
-----------------

Here are things that need to be added to the compiler to support
dynamic arrays, in no particular order.

- [x] Implement passing UDTs by value
- [x] Implement returning UDTs by value
- [ ] Move primitive types to Context, so they are not global.
- [ ] Add support for reference types.
    - Internally references are just pointers, but do not require `*` to dereference value
    - Are guaranteed to be not null.
    - [ ] Add support for reference types in the parser
    - [ ] Add support for reference types in the semantic analyzer
    - [ ] Add support for reference types in the code generator
    - [ ] Disallow casting reference away
    - [ ] Check when returning a reference to local variable
    - [ ] HAndle `IS` and `AS` operators
    - [ ] Support reference in `IF` expressions
    - [ ] Fix `FOR` statement to work with reference types
    - [ ] After `CONST` variable support been added, allow implicit conversions to `const ref` types as in c++
- [ ] Add support for more complex LHS in call expressions without parenthesis.
    - pointer dereferencing: `*funcPTr "Hello"`
    - member access: `funcPtr.member "hello"`
- [ ] Introduce `->` for pointer dereferencing?
  - Is there benefit in having this? Currently, we use `.` for member access and pointer dereferencing. When adding
    reference support, should there be way to distinguish between the three?
- [ ] Add constructors and destructors
  - Similar to C++ scoping rules
- [ ] Add move constructors
  - [ ] Implement language construct
  - [ ] Use move constructors on return statements
  - [ ] Use move constructors on assignments
  - [ ] Use move constructors on function arguments
- [ ] Add copy constructors
  - [ ] Implement language construct
  - [ ] Use copy constructors on return statements
  - [ ] Use move constructors on assignments
  - [ ] Use copy constructors on function arguments
- [ ] Add read-only computed properties
- [ ] Add methods on structs
- [ ] Add operators [] allowing indexed access
  - Need to decide on syntax. Below might work,
    but conflicts with attributes syntax
  - For getters: `Function[index As UInteger] As T`
  - For setters: `Sub Function[index As UInteger] value As T`
- [ ] Add passing
- [ ] Add return by reference
- [ ] Add pointer arithmetic 
  - `ptr + x`, will be treated as `ptr + x * sizeof(typeof(*ptr))`
  - `ptr[x]` will be treated as `*(ptr + x)`
- [ ] Add support for private, public, protected keywords
  - Expose in LLVM IR
  - Follow during semantic analyses
  - Support assymetric visibiliy on variables and properties
- Implement generics/templating support.
  - This is probably easiest to do via AST replication? 
  - [ ] Add support for generic types
  - [ ] Add support for generic functions
  - [ ] Add support for generic structs
  - [ ] Add support for generic methods
  - [ ] Add support for `Where` constraints on declarations. Similar to C++ `requires` clause, but with Swift inspired
    syntax and semantics.
- [ ] Add support for interfaces
- [ ] Implement compiler intrinsic TypeInfo templated interface
