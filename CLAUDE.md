# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

lbc is a BASIC compiler with an LLVM backend, written in clean, modern C++23. Targets GCC and Clang only (MSVC is intentionally unsupported; GNU extensions are used). No C++20 modules or `import std;` — these features are too buggy. Exceptions and RTTI are disabled (`-fno-exceptions -fno-rtti`). Uses CMake with Ninja as the build generator and Google Test for testing.

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

The main executable outputs to `bin/lbc`.

## Testing

```bash
# Run all tests
./build/claude/tests/tests

# Run a single test by name
./build/claude/tests/tests --gtest_filter="BasicTests.IsTrue"
```

Tests use Google Test (v1.17.0), fetched automatically via CMake FetchContent.

## Code Formatting and Linting

- **clang-format**: WebKit-based style (see `.clang-format`)
- **clang-tidy**: Comprehensive checks with warnings as errors (see `.clang-tidy`)

## Architecture

- `src/` — Main source code. Compiled into a static library (`lbc_lib`) and a thin executable (`lbc`) that links against it.
- `tests/` — Google Test suite, links against `lbc_lib`.
- `configured_files/` — CMake-generated headers (project version/metadata via `config.hpp.in`).
- `src/pch.hpp` — Precompiled header shared across `lbc_lib`, `lbc`, and `tests`. Common STL headers go here.

## Code Conventions

- C++23 standard, no exceptions, no RTTI, no modules/`import std;`
- GCC and Clang only; GNU extensions are acceptable
- Classes use `final` by default
- Use `[[nodiscard]]` and `noexcept` on function declarations
- Prefer `std::string_view` and `"text"sv` literals over `std::string`
- PascalCase for classes, camelCase for functions, lowercase for namespaces
- Root namespace: `lbc`
- 4-space indentation, LF line endings

## Important Patterns

- Error handling: prefer `std::expected` for returning errors (no exceptions)
- Memory: RAII everywhere, no manual new/delete
