//
// Created by Albert Varaksin on 17/06/2026.
//
#include "CompilerBase.hpp"
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Gen/Generator.hpp"
#include "IR/gen/IrGenerator.hpp"
#include "IR/lib/Module.hpp"
#include "JIT.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"
using namespace lbc;

namespace {
// Captured program output; the JIT redirects printf/puts here. Tests run serially.
thread_local std::string g_output; // NOLINT(*-avoid-non-const-global-variables)

auto capturePrintf(const char* format, ...) -> int { // NOLINT(cert-dcl50-cpp,*-vararg)
    va_list args;
    va_start(args, format);
    const int length = std::vsnprintf(nullptr, 0, format, args);
    va_end(args);
    if (length <= 0) {
        return length;
    }
    std::vector<char> buffer(static_cast<std::size_t>(length) + 1);
    va_start(args, format);
    std::vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);
    g_output.append(buffer.data(), static_cast<std::size_t>(length));
    return length;
}

auto capturePuts(const char* str) -> int {
    g_output += str;
    g_output += '\n';
    return 0;
}

auto render(Context& context) -> std::string {
    std::string out;
    llvm::raw_string_ostream os { out };
    context.getDiag().print(os);
    return llvm::StringRef { out }.trim().str();
}
} // namespace

auto CompilerBase::enumerate(const std::filesystem::path& dir) -> std::vector<std::filesystem::path> {
    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath() / dir)) {
        if (entry.is_regular_file()) {
            paths.push_back(entry.path());
        }
    }
    std::ranges::sort(paths);
    return paths;
}

auto CompilerBase::lower(Context& context) -> std::unique_ptr<llvm::Module> {
    auto buffer = llvm::MemoryBuffer::getFile(GetParam().string());
    if (!buffer) {
        return nullptr;
    }
    const auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(*buffer), llvm::SMLoc {});

    auto ast = Parser { context, id }.parse();
    if (!ast) {
        return nullptr;
    }
    if (SemanticAnalyser sema { context }; !sema.analyse(**ast)) {
        return nullptr;
    }
    auto ir = ir::gen::IrGenerator { context }.generate(**ast);
    if (!ir) {
        return nullptr;
    }
    return gen::Generator { context }.generate(**ir);
}

auto CompilerBase::compile() -> std::string {
    Context context;
    context.getDiag().setAutoPrint(false);
    std::ignore = lower(context);
    m_failed = context.getDiag().hasErrors();
    return render(context);
}

auto CompilerBase::run() -> std::string {
    Context context;
    context.getDiag().setAutoPrint(false);

    auto module = lower(context);
    if (module == nullptr || context.getDiag().hasErrors()) {
        m_failed = true;
        return render(context);
    }
    m_failed = false;

    // The module lives in `context`'s LLVMContext, which ORC cannot own. Move it
    // into a JIT-owned context through an in-memory bitcode round-trip.
    llvm::SmallVector<char> bitcode;
    {
        llvm::raw_svector_ostream os { bitcode };
        llvm::WriteBitcodeToFile(*module, os);
    }
    auto llvmContext = std::make_unique<llvm::LLVMContext>();
    auto reparsed = llvm::parseBitcodeFile(
        llvm::MemoryBufferRef { llvm::StringRef { bitcode.data(), bitcode.size() }, "jit" },
        *llvmContext
    );
    if (!reparsed) {
        llvm::consumeError(reparsed.takeError());
        m_failed = true;
        return "bitcode round-trip failed";
    }

    auto jit = llvm::cantFail(test::JIT::create());
    (*reparsed)->setDataLayout(jit.getDataLayout());
    llvm::cantFail(jit.define("printf", &capturePrintf));
    llvm::cantFail(jit.define("puts", &capturePuts));
    llvm::orc::ThreadSafeModule tsm { std::move(*reparsed), llvm::orc::ThreadSafeContext { std::move(llvmContext) } };
    llvm::cantFail(jit.addModule(std::move(tsm)));
    llvm::cantFail(jit.initialize());

    g_output.clear();
    const auto main = llvm::cantFail(jit.lookup("main"));
    main.toPtr<int (*)()>()();
    return llvm::StringRef { g_output }.trim().str();
}

auto CompilerBase::expected() const -> std::string {
    static constexpr llvm::StringRef prefix = "'' CHECK: ";
    static constexpr llvm::StringRef fileKey = "__FILE__";
    const auto file = GetParam().string();

    std::ifstream stream { GetParam() };
    std::string result;
    std::string line;
    bool first = true;
    while (std::getline(stream, line)) {
        const llvm::StringRef ref { line };
        if (!ref.starts_with(prefix)) {
            continue;
        }
        std::string check = ref.substr(prefix.size()).str();
        if (const auto pos = check.find(fileKey); pos != std::string::npos) {
            check.replace(pos, fileKey.size(), file);
        }
        if (not first) {
            result += '\n';
        }
        first = false;
        result += check;
    }
    return llvm::StringRef { result }.trim().str();
}
