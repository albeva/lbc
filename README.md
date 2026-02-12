# lbc

A BASIC compiler with an LLVM backend, written in C++.

The goal is to build a modern language with BASIC-inspired syntax that is fully C ABI compatible — allowing direct
interop with C libraries without wrappers or bindings.

## Requirements

- C++23 compiler (Clang, GCC or MSVC)
- CMake 4.2+
- Ninja
- LLVM 22+

## Building

```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
ninja -C build
```

The executable is output to `bin/lbc`.

## Testing

```bash
./build/tests/tests
```
