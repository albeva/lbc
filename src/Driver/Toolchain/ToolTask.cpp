//
// Created by Albert Varaksin on 07/02/2021.
//
#include "ToolTask.hpp"
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Toolchain.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Program.h>
using namespace lbc;

auto ToolTask::reset() -> ToolTask& {
    m_args.clear();
    return *this;
}

auto ToolTask::addArg(const std::string& arg) -> ToolTask& {
    m_args.push_back(arg);
    return *this;
}

auto ToolTask::addArg(const std::string& name, const std::string& value) -> ToolTask& {
    m_args.push_back(name);
    m_args.push_back(value);
    return *this;
}

auto ToolTask::addPath(const fs::path& path) -> ToolTask& {
    addArg(path.string());
    return *this;
}

auto ToolTask::addPath(const std::string& name, const fs::path& value) -> ToolTask& {
    m_args.push_back(name);
    addPath(value);
    return *this;
}

auto ToolTask::addArgs(std::initializer_list<std::string> args) -> ToolTask& {
    if (m_args.capacity() < m_args.size() + args.size()) {
        m_args.reserve(m_args.size() + args.size());
    }
    std::ranges::move(args, std::back_inserter(m_args));
    return *this;
}

auto ToolTask::execute() const -> int {
    std::vector<llvm::StringRef> args;
    args.reserve(m_args.size() + 1);

    auto program = m_path.string();
    args.emplace_back(program);

    std::ranges::copy(m_args, std::back_inserter(args));

    if (m_context.getOptions().logVerbose()) {
        switch (m_kind) {
        case ToolKind::Optimizer:
            llvm::outs() << "Optimize:\n";
            break;
        case ToolKind::Assembler:
            llvm::outs() << "Assemble:\n";
            break;
        case ToolKind::Linker:
            llvm::outs() << "Link:\n";
            break;
        }
        llvm::outs() << program;
        for (const auto& arg : m_args) {
            llvm::outs() << " " << arg;
        }
        llvm::outs() << '\n'
                     << '\n';
    }

    return llvm::sys::ExecuteAndWait(program, args);
}
