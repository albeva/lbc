//
// Created by Albert Varaksin on 13/07/2020.
//
#include "Driver.hpp"
#include "Ast/AstPrinter.hpp"
#include "Ast/CodePrinter.hpp"
#include "Context.hpp"
#include "Driver/Toolchain/Toolchain.hpp"
#include "Gen/CodeGen.hpp"
#include "JIT.hpp"
#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "TempFileCache.hpp"
#include "Toolchain/ToolTask.hpp"
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
using namespace lbc;

namespace {
llvm::ExitOnError exitOnErr;

std::string exec(const char* cmd) {
#ifdef __APPLE__
    constexpr auto bufferSize = 128;
    std::array<char, bufferSize> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> const pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        fatalError("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    auto ref = llvm::StringRef(result);
    return ref.trim().str();
#else
    (void)cmd;
    fatalError("exec unsupported");
#endif
}
} // namespace

Driver::Driver(Context& context)
: m_context{ context },
  m_options{ context.getOptions() } {}

void Driver::drive() {
    // make sure jit is loaded if targeting it
    if (m_options.getCompilationTarget() == CompileOptions::CompilationTarget::JIT) {
        (void)m_context.getJIT();
    }

    // compile sources
    compile();

    if (m_options.getDumpAst()) {
        dumpAst();
        return;
    }

    if (m_options.getDumpCode()) {
        dumpCode();
        return;
    }

    switch (m_options.getCompilationTarget()) {
    case CompileOptions::CompilationTarget::Executable:
        emitBitCode(true);
        optimize();
        emitObjects(true);
        emitExecutable();
        break;
    case CompileOptions::CompilationTarget::Object:
        switch (m_options.getOutputType()) {
        case CompileOptions::OutputType::Native:
            emitBitCode(true);
            optimize();
            emitObjects(false);
            break;
        case CompileOptions::OutputType::LLVM:
            emitBitCode(false);
            optimize();
            break;
        }
        break;
    case CompileOptions::CompilationTarget::Assembly:
        switch (m_options.getOutputType()) {
        case CompileOptions::OutputType::Native:
            emitBitCode(true);
            optimize();
            emitAssembly(false);
            break;
        case CompileOptions::OutputType::LLVM:
            emitLLVMIr(false);
            optimize();
            break;
        }
        break;
    case CompileOptions::CompilationTarget::JIT: {
        execute();
        break;
    }
    }

    TempFileCache::removeTemporaryFiles();
}

/**
 * Compile sources
 */
void Driver::compile() {
    processInputs();
    compileSources();
}

/**
 * Execute modules in JIT
 */
void Driver::execute() {
    if (m_modules.empty()) {
        return;
    }

    auto& jit = m_context.getJIT();

    // Add modules
    for (auto& module : m_modules) {
        exitOnErr(jit.addModule({ std::move(module->llvmModule),
            std::make_unique<llvm::LLVMContext>() }));
    }

    // init
    exitOnErr(jit.initialize());

    // run main
    auto main = exitOnErr(jit.lookup("main"));
    main.toPtr<int()>()();

    // clean up
    exitOnErr(jit.deinitialize());
}

/**
 * Process provided input files from the context, resolve their path,
 * ansure they exost and store in driver paths structure
 */
void Driver::processInputs() {
    for (const auto& paths : m_options.getInputFiles()) {
        auto type = paths.first;
        auto& dst = getSources(type);
        for (const auto& path : paths.second) {
            // cppcheck-suppress useStlAlgorithm
            dst.emplace_back(Source::create(type, m_options.resolveFilePath(path), false));
        }
    }
}

std::unique_ptr<Source> Driver::deriveSource(const Source& source, CompileOptions::FileType type, bool temporary) const {
    const auto& original = source.origin.path;
    const auto ext = CompileOptions::getFileExt(type);
    const auto path = temporary
        ? TempFileCache::createUniquePath(original, ext)
        : m_options.resolveOutputPath(original, ext);
    return source.derive(type, path);
}

void Driver::emitLLVMIr(bool temporary) {
    emitLlvm(CompileOptions::FileType::LLVMIr, temporary, [](auto& stream, auto& module) {
        auto* printer = llvm::createPrintModulePass(stream);
        printer->runOnModule(module);
    });
}

void Driver::emitBitCode(bool temporary) {
    emitLlvm(CompileOptions::FileType::BitCode, temporary, [](auto& stream, auto& module) {
        llvm::WriteBitcodeToFile(module, stream);
    });
}

void Driver::emitLlvm(CompileOptions::FileType type, bool temporary, void (*generator)(llvm::raw_fd_ostream&, llvm::Module&)) {
    auto& dstFiles = getSources(type);
    dstFiles.reserve(dstFiles.size() + m_modules.size());

    for (auto& module : m_modules) {
        const auto& source = module->source;
        auto output = deriveSource(*source, type, temporary);

        std::error_code errors{};
        llvm::raw_fd_ostream stream{
            output->path.string(),
            errors,
            llvm::sys::fs::OpenFlags::OF_None
        };

        generator(stream, *module->llvmModule);

        stream.flush();
        stream.close();

        dstFiles.emplace_back(std::move(output));
    }
}

void Driver::emitAssembly(bool temporary) {
    emitNative(CompileOptions::FileType::Assembly, temporary);
}

void Driver::emitObjects(bool temporary) {
    emitNative(CompileOptions::FileType::Object, temporary);
}

void Driver::emitNative(CompileOptions::FileType type, bool temporary) {
    const auto& bcFiles = getSources(CompileOptions::FileType::BitCode);
    auto& dstFiles = getSources(type);
    std::string filetype;
    if (type == CompileOptions::FileType::Object) {
        filetype = "obj";
    } else {
        filetype = "asm";
    }
    dstFiles.reserve(dstFiles.size() + bcFiles.size());

    auto assembler = m_context.getToolchain().createTask(ToolKind::Assembler);
    for (const auto& source : bcFiles) {
        auto output = deriveSource(*source, type, temporary);

        assembler.reset();
        assembler.addArg("-filetype="s + filetype);
        if (type == CompileOptions::FileType::Assembly && m_context.getTriple().isX86()) {
            assembler.addArg("--x86-asm-syntax=intel");
        }
        assembler.addPath("-o", output->path);
        assembler.addPath(source->path);

        if (assembler.execute() != EXIT_SUCCESS) {
            fatalError("Failed emit '"_t + output->path.string() + "'");
        }

        dstFiles.emplace_back(std::move(output));
    }
}

void Driver::optimize() {
    auto level = m_options.getOptimizationLevel();
    if (level == CompileOptions::OptimizationLevel::O0) {
        return;
    }
    bool llvmIr = m_options.isOutputLLVMIr();
    const auto& files = getSources(llvmIr ? CompileOptions::FileType::LLVMIr : CompileOptions::FileType::BitCode);

    auto optimizer = m_context.getToolchain().createTask(ToolKind::Optimizer);
    for (const auto& file : files) {
        optimizer.reset();
        if (llvmIr) {
            optimizer.addArg("-S");
        }

        switch (level) {
        case CompileOptions::OptimizationLevel::OS:
            optimizer.addArg("-OS");
            break;
        case CompileOptions::OptimizationLevel::O1:
            optimizer.addArg("-O1");
            break;
        case CompileOptions::OptimizationLevel::O2:
            optimizer.addArg("-O2");
            break;
        case CompileOptions::OptimizationLevel::O3:
            optimizer.addArg("-O3");
            break;
        default:
            llvm_unreachable("Unexpected optimization level");
        }
        optimizer.addPath("-o", file->path);

        optimizer.addPath(file->path);

        if (optimizer.execute() != EXIT_SUCCESS) {
            fatalError("Failed to optimize "_t + file->path.string());
        }
    }
}

void Driver::emitExecutable() {
    auto linker = m_context.getToolchain().createTask(ToolKind::Linker);
    const auto& objFiles = getSources(CompileOptions::FileType::Object);
    const auto& triple = m_context.getTriple();

    if (objFiles.empty()) {
        fatalError("No objects to link");
    }

    if (triple.isArch32Bit()) {
        fatalError("32bit is not implemented yet");
    }

    auto output = m_options.getOutputPath();
    if (output.empty()) {
        output = m_options.getWorkingDir() / objFiles[0]->origin.path.stem();
        if (triple.isOSWindows()) {
            output += ".exe";
        }
    } else if (output.is_relative()) {
        output = fs::absolute(m_options.getWorkingDir() / output);
    }

    if (m_options.getOptimizationLevel() != CompileOptions::OptimizationLevel::O0) {
        if (!triple.isMacOSX()) {
            linker.addArg("-O");
            linker.addArg("-s");
        }
    }

    if (triple.isOSWindows()) {
        auto sysLibPath = m_context.getToolchain().getBasePath() / "lib";
        linker
            .addArg("-m", "i386pep")
            .addPath("-o", output)
            .addArg("-subsystem", "console")
            .addArg("--stack", "1048576,1048576")
            .addPath("-L", sysLibPath)
            .addPath(sysLibPath / "crt2.o")
            .addPath(sysLibPath / "crtbegin.o");

        for (const auto& obj : objFiles) {
            linker.addPath(obj->path);
        }

        linker
            .addArgs({ "-(",
                "-lgcc",
                "-lmsvcrt",
                "-lkernel32",
                "-luser32",
                "-lmingw32",
                "-lmingwex",
                "-)" })
            .addPath(sysLibPath / "crtend.o");
    } else if (triple.isMacOSX()) {
        auto macosSdk = exec("xcrun --show-sdk-path");
        linker
            .addPath("-L", "/usr/local/lib")
            .addPath("-syslibroot", macosSdk)
            .addArg("-lSystem")
            .addPath("-o", output);

        for (const auto& obj : objFiles) {
            linker.addPath(obj->path);
        }
    } else if (triple.isOSLinux()) {
        std::string const linuxSysPath = "/usr/lib/x86_64-linux-gnu";
        linker
            .addArg("-m", "elf_x86_64")
            .addArg("-dynamic-linker", "/lib64/ld-linux-x86-64.so.2")
            .addArg("-L", "/usr/lib")
            .addArg(linuxSysPath + "/crt1.o")
            .addArg(linuxSysPath + "/crti.o")
            .addPath("-o", output);

        for (const auto& obj : objFiles) {
            linker.addPath(obj->path);
        }

        linker.addArg("-lc");
        linker.addArg(linuxSysPath + "/crtn.o");
    } else {
        fatalError("Compilation not this platform not supported");
    }

    if (linker.execute() != EXIT_SUCCESS) {
        fatalError("Failed generate '"_t + output.string() + "'");
    }
}

// Compile

void Driver::compileSources() {
    if (m_options.logVerbose()) {
        llvm::outs() << "Compile:\n";
    }

    const auto& sources = getSources(CompileOptions::FileType::Source);
    m_modules.reserve(m_modules.size() + sources.size());
    for (const auto& source : sources) {
        std::string included;
        auto ID = m_context.getSourceMrg().AddIncludeFile(source->path.string(), {}, included);
        if (ID == ~0U) {
            fatalError("Failed to load '"_t + source->path.string() + "'");
        }
        compileSource(source.get(), ID);
    }

    if (m_options.logVerbose()) {
        llvm::outs() << '\n';
    }
}

void Driver::compileSource(const Source* source, unsigned int ID) {
    const auto& path = source->path;
    if (m_options.logVerbose()) {
        llvm::outs() << path.string() << '\n';
    }

    bool isMain = m_options.isMainFile(path);
    Lexer lexer{ m_context, ID };
    Parser parser{ m_context, lexer, isMain };

    auto astOrErr = parser.parse();
    if (astOrErr.hasError()) {
        std::exit(EXIT_FAILURE);
    }
    AstModule* ast = astOrErr.getValue();

    // Analyze
    SemanticAnalyzer sem{ m_context };
    MUST(sem.visit(*ast))

    if (m_options.getDumpAst() || m_options.getDumpCode()) {
        m_modules.emplace_back(std::make_unique<TranslationUnit>(
            nullptr,
            source,
            ast));
        return;
    }

    // generate IR
    CodeGen gen{ m_context };
    gen.visit(*ast);

    // done
    if (!gen.validate()) {
        fatalError("Failed to compile '"_t + path.string() + "'");
    }

    // Happy Days
    m_modules.emplace_back(std::make_unique<TranslationUnit>(
        gen.getModule(),
        source,
        ast));
}

void Driver::dumpAst() {
    auto print = [&](llvm::raw_ostream& stream) {
        AstPrinter printer{ m_context, stream };
        for (const auto& module : m_modules) {
            printer.visit(*module->ast);
        }
    };

    auto output = m_options.getOutputPath();
    if (output.empty()) {
        print(llvm::outs());
    } else {
        if (output.is_relative()) {
            output = fs::absolute(m_options.getWorkingDir() / output);
        }

        std::error_code errors{};
        llvm::raw_fd_ostream stream{
            output.string(),
            errors,
            llvm::sys::fs::OpenFlags::OF_None
        };

        print(stream);

        stream.flush();
        stream.close();
    }
}

void Driver::dumpCode() {
    auto print = [&](llvm::raw_ostream& stream) {
        CodePrinter printer{ stream };
        for (const auto& module : m_modules) {
            printer.visit(*module->ast);
        }
    };

    auto output = m_options.getOutputPath();
    if (output.empty()) {
        print(llvm::outs());
    } else {
        if (output.is_relative()) {
            output = fs::absolute(m_options.getWorkingDir() / output);
        }

        std::error_code errors{};
        llvm::raw_fd_ostream stream{
            output.string(),
            errors,
            llvm::sys::fs::OpenFlags::OF_None
        };

        print(stream);

        stream.flush();
        stream.close();
    }
}
