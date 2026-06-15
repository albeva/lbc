//
// Created by Albert Varaksin on 15/06/2026.
//
#include "CompileTask.hpp"
#include "Ast/AstCodePrinter.hpp"
#include "Driver/Context.hpp"
#include "Gen/Generator.hpp"
#include "IR/gen/IrGenerator.hpp"
#include "IR/printer/Printer.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"
using namespace lbc;

auto CompileTask::run(Context& context, Unit& unit) -> DiagResult<void> {
    const auto& options = context.getOptions();

    std::string included;
    const auto id = context.getSourceMgr().AddIncludeFile(unit.sourcePath, {}, included);
    if (id == 0) {
        return DiagError { context.getDiag().log(diagnostics::inputFileNotFound(unit.sourcePath)) };
    }

    Parser parser { context, id };
    TRY_DECL(module, parser.parse())

    SemanticAnalyser sema { context };
    TRY(sema.analyse(*module))

    // Debug dumps go to stderr so they never pollute the artifact on stdout.
    if (options.isDumpAst()) {
        AstCodePrinter { llvm::errs() }.print(*module);
    }

    ir::gen::IrGenerator irGenerator { context };
    TRY_DECL(ir, irGenerator.generate(*module))

    if (options.isDumpIr()) {
        ir::printer::Printer { llvm::errs() }.print(*ir);
    }

    gen::Generator generator { unit.llvmContext };
    unit.module = generator.generate(*ir);
    unit.module->setTargetTriple(context.getTriple());

    return {};
}
