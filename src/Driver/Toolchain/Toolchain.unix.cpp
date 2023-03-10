//
// Created by Albert Varaksin on 07/02/2021.
//

static fs::path getUnixToolPath(const fs::path& base, ToolKind tool) {
    switch (tool) {
    case ToolKind::Optimizer:
        return base / "opt";
    case ToolKind::Assembler:
        return base / "llc";
    case ToolKind::Linker:
        return base / "ld";
    default:
        llvm_unreachable("Invalid ToolKind ID");
    }
}

fs::path Toolchain::getPath(ToolKind tool) const {
    fs::path path;

    if (m_basePath.empty()) {
        path = getUnixToolPath("/usr/local/bin", tool);
        if (!fs::exists(path)) {
            path = getUnixToolPath("/usr/bin", tool);
        }
    } else {
        path = getUnixToolPath(m_basePath, tool);
    }

    if (!fs::exists(path)) {
        fatalError("ToolKind "_t + path.string() + " not found!");
    }
    return path;
}
