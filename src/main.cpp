//
// Created by Albert Varaksin on 03/07/2020.
//
#include "Driver/CmdLineParser.hpp"
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Driver/Driver.hpp"
#include <llvm/Support/InitLLVM.h>
using namespace lbc;

int main(int argc, const char* argv[]) {
    llvm::InitLLVM const init{ argc, argv };

    CompileOptions options{};
    CmdLineParser cmdLineParser{ options };
    cmdLineParser.parse({ argv, static_cast<size_t>(argc) });
    options.validate();

    llvm::LLVMContext llvmContext{};
    Context context{ options, &llvmContext };
    Driver{ context }.drive();

    return EXIT_SUCCESS;
}
