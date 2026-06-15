//
// Created by Albert Varaksin on 13/02/2026.
//
#include "Context.hpp"
using namespace lbc;

Context::Context(CompileOptions options)
: m_options(std::move(options))
, m_diagEngine(*this)
, m_typeFactory(*this) {}

auto Context::retain(const llvm::StringRef string) -> llvm::StringRef {
    return m_strings.insert(string).first->first();
}
