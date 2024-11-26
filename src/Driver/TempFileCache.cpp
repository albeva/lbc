//
//  TemporaryFileCache.cpp
//  lbc
//
//  Created by Albert Varaksin on 26/04/2021.
//
#include "TempFileCache.hpp"
#include <llvm/Support/FileSystem.h>
using namespace lbc;

namespace {
std::vector<fs::path> tempFiles{}; // NOLINT
llvm::SmallVector<char, 255> filenameCache{}; // NOLINT
} // namespace

auto TempFileCache::createUniquePath(llvm::StringRef suffix) -> fs::path {
    filenameCache.clear();
    llvm::sys::fs::createUniquePath("lbc-%%%%%%%%%%%%"_t + suffix, filenameCache, true);
    return tempFiles.emplace_back(filenameCache.begin(), filenameCache.end());
}

auto TempFileCache::createUniquePath(const fs::path& file, llvm::StringRef suffix) -> fs::path {
    filenameCache.clear();
    llvm::sys::fs::createUniquePath(
        "lbc-"_t + file.stem().string() + "-%%%%%%%%%%%%" + suffix,
        filenameCache,
        true
    );
    return tempFiles.emplace_back(filenameCache.begin(), filenameCache.end());
}

void TempFileCache::removeTemporaryFiles() {
    for (const auto& temp : tempFiles) {
        if (fs::exists(temp)) {
            fs::remove(temp);
        }
    }
    tempFiles.clear();
}
