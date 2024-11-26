//
//  TempFileCache.hpp
//  lbc
//
//  Created by Albert Varaksin on 26/04/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {

struct TempFileCache final {
    [[nodiscard]] static auto createUniquePath(llvm::StringRef suffix) -> fs::path;
    [[nodiscard]] static auto createUniquePath(const fs::path& file, llvm::StringRef suffix) -> fs::path;
    static void removeTemporaryFiles();
};

} // namespace lbc
