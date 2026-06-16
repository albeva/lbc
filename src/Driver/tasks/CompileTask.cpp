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

auto CompileTask::run(std::string source) -> DiagResult<std::unique_ptr<llvm::Module>> {
    const auto& options = m_context.getOptions();

    std::string included;
    const auto id = m_context.getSourceMgr().AddIncludeFile(source, {}, included);
    if (id == 0) {
        return DiagError { m_context.getDiag().log(diagnostics::inputFileNotFound(source)) };
    }

    Parser parser { m_context, id };
    TRY_DECL(module, parser.parse())

    SemanticAnalyser sema { m_context };
    TRY(sema.analyse(*module))

    // Debug dumps go to stderr so they never pollute the artifact on stdout.
    if (options.isDumpAst()) {
        AstCodePrinter { llvm::errs() }.print(*module);
    }

    ir::gen::IrGenerator irGenerator { m_context };
    TRY_DECL(ir, irGenerator.generate(*module))

    if (options.isDumpIr()) {
        ir::printer::Printer { llvm::errs() }.print(*ir);
    }

    gen::Generator generator { m_context };
    return generator.generate(*ir);
}
