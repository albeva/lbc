#include "pch.hpp"
#include <llvm/Support/InitLLVM.h>
#include "Ast/AstCodePrinter.hpp"
#include "Diag/DiagEngine.hpp"
#include "Driver/Context.hpp"
#include "IR/gen/IrGenerator.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"
#include "IR/printer/IrPrinter.hpp"

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

    const lbc::ir::printer::IrPrinter printer{};
    printer.print(*ir);

    return {};
}

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };
    MUST(build("samples/hello.bas"))
}
