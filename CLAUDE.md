# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

lbc is a toy BASIC compiler written in modern C++23. It uses CMake with Ninja as the build generator and Google Test for testing. Exceptions and RTTI are disabled (`-fno-exceptions -fno-rtti`).

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
- `src/pch.hpp` — Precompiled header shared across `lbc_lib`, `lbc`, and `tests`.

## Code Conventions

- C++23 standard, no exceptions, no RTTI
- Classes use `final` by default
- Use `[[nodiscard]]` and `noexcept` on function declarations
- Prefer `std::string_view` and `"text"sv` literals over `std::string`
- PascalCase for classes, camelCase for functions, lowercase for namespaces
- Root namespace: `lbc`
- 4-space indentation, LF line endings

## Important Patterns

- Error handling: prefer std::expected for returning errors
- Memory: RAII everywhere, no manual new/delete
