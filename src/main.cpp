#include "pch.hpp"
#include <llvm/Support/InitLLVM.h>
#include "Utilities/Sample.hpp"
using namespace lbc::utils;

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };
    std::print("{}", Sample::getMessage());
}
