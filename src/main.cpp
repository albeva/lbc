#include "pch.hpp"
#include <llvm/Support/InitLLVM.h>

#include "Driver/CompileOptions.hpp"
#include "Driver/Driver.hpp"

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };

    lbc::CompileOptions options;
    options.addFile("samples/hello.bas");
    options.setOutputType(lbc::CompileOptions::OutputType::LlvmIr);

    lbc::Driver driver { std::move(options) };
    return driver.execute() ? 0 : 1;
}
