//
// Created by Albert Varaksin on 13/02/2026.
//
#include "Context.hpp"
using namespace lbc;

Context::Context()
: m_diagEngine(*this)
, m_typeFactory(*this) {}

auto Context::retain(llvm::StringRef string) -> llvm::StringRef {
    return m_strings.insert(string).first->first();
}
