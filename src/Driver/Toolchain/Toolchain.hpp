//
// Created by Albert Varaksin on 07/02/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {

class ToolTask;
class Context;

enum class ToolKind : std::uint8_t {
    Optimizer,
    Assembler,
    Linker
};

/**
 * Abstract access and execution of tools
 * used during compilation
 *
 * e.g. a linker
 */
class Toolchain final {
public:
    NO_COPY_AND_MOVE(Toolchain)

    explicit Toolchain(Context& context)
    : m_context { context } {
    }
    ~Toolchain() = default;

    [[nodiscard]] auto getBasePath() const -> const fs::path&;

    [[nodiscard]] auto getPath(ToolKind tool) const -> fs::path;

    [[nodiscard]] auto createTask(ToolKind kind) const -> ToolTask;

private:
    Context& m_context;
};

} // namespace lbc
