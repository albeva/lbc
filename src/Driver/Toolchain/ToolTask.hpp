//
// Created by Albert Varaksin on 07/02/2021.
//
#pragma once
#include "pch.hpp"
namespace lbc {

class Toolchain;
class Context;
enum class ToolKind : std::uint8_t;

class ToolTask final {
public:
    NO_COPY_AND_MOVE(ToolTask)

    ToolTask(Context& context, fs::path path, const ToolKind kind)
    : m_context { context }
    , m_path { std::move(path) }
    , m_kind { kind } {
    }

    ~ToolTask() = default;

    auto reset() -> ToolTask&;

    auto addArg(const std::string& arg) -> ToolTask&;
    auto addArg(const std::string& name, const std::string& value) -> ToolTask&;
    auto addPath(const fs::path& path) -> ToolTask&;
    auto addPath(const std::string& name, const fs::path& value) -> ToolTask&;
    auto addArgs(std::initializer_list<std::string> args) -> ToolTask&;

    [[nodiscard]] auto execute() const -> int;

private:
    std::vector<std::string> m_args;
    Context& m_context;
    const fs::path m_path;
    const ToolKind m_kind;
};

} // namespace lbc
