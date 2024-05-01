//
// Created by Albert Varaksin on 07/02/2021.
//
#include "Toolchain.hpp"
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "ToolTask.hpp"

using namespace lbc;

ToolTask Toolchain::createTask(ToolKind kind) const {
    return ToolTask{ m_context, getPath(kind), kind };
}


static fs::path getToolPath(const fs::path& base, ToolKind tool) {
    std::string ext{};
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    ext = ".exe";
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

fs::path Toolchain::getPath(ToolKind tool) const {
    fs::path path{};

    const auto& basePath = getBasePath();
    if (basePath.empty()) {
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

const fs::path& Toolchain::getBasePath() const {
    return m_context.getOptions().getToolchainDir();
}
