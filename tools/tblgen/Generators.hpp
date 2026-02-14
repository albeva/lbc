#pragma once

namespace llvm {
class raw_ostream;
class RecordKeeper;
} // namespace llvm

auto emitTokens(llvm::raw_ostream& os, const llvm::RecordKeeper& records, llvm::StringRef generator) -> bool;
