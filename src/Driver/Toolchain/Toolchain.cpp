//
// Created by Albert Varaksin on 07/02/2021.
//
#include "Toolchain.hpp"
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "ToolTask.hpp"
using namespace lbc;

namespace {
auto getToolPath(const fs::path& base, const ToolKind tool) -> fs::path {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    const std::string ext = "exe";
#else
    const std::string ext {};
#endif

    switch (tool) {
    case ToolKind::Optimizer:
        return base / ("bin/opt" + ext);
    case ToolKind::Assembler:
        return base / ("bin/llc" + ext);
    case ToolKind::Linker:
        return base / ("bin/ld" + ext);
    default:
        llvm_unreachable("Invalid ToolKind ID");
    }
}
} // namespace

auto Toolchain::createTask(const ToolKind kind) const -> ToolTask {
    return ToolTask { m_context, getPath(kind), kind };
}

auto Toolchain::getPath(const ToolKind tool) const -> fs::path {
    fs::path path {};

    if (const auto& basePath = getBasePath(); basePath.empty()) {
        path = getToolPath("/usr/local", tool);
        if (!fs::exists(path)) {
            path = getToolPath("/usr", tool);
        }
    } else {
        path = getToolPath(basePath, tool);
    }

    if (!fs::exists(path)) {
        fatalError("ToolKind "_t + path.string() + " not found!");
    }

    return path;
}

auto Toolchain::getBasePath() const -> const fs::path& {
    return m_context.getOptions().getToolchainDir();
}
