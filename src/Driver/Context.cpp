//
// Created by Albert Varaksin on 13/02/2026.
//
#include "Context.hpp"
using namespace lbc;

auto Context::retain(llvm::StringRef string) -> llvm::StringRef {
    return m_strings.insert(string).first->first();
}
