//
// Created by Albert Varaksin on 18/04/2021.
//
#include "Utils.hpp"
#include "Driver/TempFileCache.hpp"

void lbc::fatalError(const llvm::Twine& message, bool prefix, bool showLoc, std::source_location loc) {
    if (prefix) {
        llvm::errs() << "lbc: error: ";
    }

    if (showLoc) {
        llvm::errs() << loc.file_name() << ':' << loc.line() << ':' << loc.column() << ": ";
    }

    llvm::errs() << message << '\n';

    TempFileCache::removeTemporaryFiles();

    std::exit(EXIT_FAILURE);
}

void lbc::warning(const llvm::Twine& message, bool prefix) {
    if (prefix) {
        llvm::outs() << "lbc: warning: ";
    }
    llvm::outs() << message << '\n';
}
