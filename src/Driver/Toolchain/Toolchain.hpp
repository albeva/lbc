#include <utility>

//
// Created by Albert Varaksin on 07/02/2021.
//
#pragma once
#include "pch.hpp"


namespace lbc {

class ToolTask;
class Context;

enum class ToolKind {
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

    explicit Toolchain(Context& context) : m_context{ context } {}
    ~Toolchain() = default;

    void setBasePath(fs::path path) { m_basePath = std::move(path); }
    [[nodiscard]] const fs::path& getBasePath() const { return m_basePath; }

    [[nodiscard]] fs::path getPath(ToolKind tool) const;

    [[nodiscard]] ToolTask createTask(ToolKind kind) const;

private:
    fs::path m_basePath{};
    Context& m_context;
};

} // namespace lbc
