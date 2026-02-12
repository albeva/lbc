#include "pch.hpp"
#include <llvm/Support/InitLLVM.h>
#include "Utilities/Sample.hpp"
#include "Utilities/Try.hpp"
using namespace lbc::utils;

namespace {
auto marshal() -> std::expected<void, bool> {
    TRY_DECL(msg, Sample::getMessage());
    std::print("{}", msg);
    return {};
}
} // namespace

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };
    MUST(marshal());
}
