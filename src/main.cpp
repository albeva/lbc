#include "pch.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/InitLLVM.h>

#include <print>
#include "Ast/AstCodePrinter.hpp"
#include "Diag/DiagEngine.hpp"
#include "Driver/Context.hpp"
#include "Gen/Generator.hpp"
#include "IR/gen/IrGenerator.hpp"
#include "IR/printer/Printer.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"

namespace {
auto build(const std::string& source) -> lbc::DiagResult<void> {
    lbc::Context context;
    std::string included;
    const auto id = context.getSourceMgr().AddIncludeFile(source, {}, included);

    lbc::Parser parser { context, id };
    TRY_DECL(module, parser.parse())

    lbc::SemanticAnalyser sema { context };
    TRY(sema.analyse(*module))

    lbc::ir::gen::IrGenerator irGenerator { context };
    TRY_DECL(ir, irGenerator.generate(*module));

    const lbc::ir::printer::Printer printer {};
    printer.print(*ir);

    lbc::gen::Generator generator {};
    auto& llvmModule = generator.generate(*ir);
    if (llvm::verifyModule(llvmModule, &llvm::errs())) {
        std::println(stderr, "error: generated LLVM module failed verification");
    }
    llvmModule.print(llvm::outs(), nullptr);
    return {};
}
} // namespace

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };
    MUST(build("samples/hello.bas"))
}
