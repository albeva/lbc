//
// Created by Albert Varaksin on 07/02/2021.
//
#pragma once
namespace lbc {

class Toolchain;
class Context;
enum class ToolKind;

class ToolTask final {
public:
    NO_COPY_AND_MOVE(ToolTask)

    ToolTask(Context& context, fs::path path, ToolKind kind) noexcept
    : m_context{ context }, m_path{ std::move(path) }, m_kind{ kind } {}

    ~ToolTask() noexcept = default;

    ToolTask& reset();

    ToolTask& addArg(const std::string& arg);
    ToolTask& addArg(const std::string& name, const std::string& value);
    ToolTask& addPath(const fs::path& path);
    ToolTask& addPath(const std::string& name, const fs::path& value);
    ToolTask& addArgs(std::initializer_list<std::string> arghs);

    [[nodiscard]] int execute() const noexcept;

private:
    std::vector<std::string> m_args;
    Context& m_context;
    const fs::path m_path;
    const ToolKind m_kind;
};

} // namespace lbc
