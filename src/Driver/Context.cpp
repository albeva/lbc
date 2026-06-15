//
// Created by Albert Varaksin on 13/02/2026.
//
#include "Context.hpp"
#include <llvm/TargetParser/Host.h>
using namespace lbc;

namespace {
/** Representative arch type for a family; the width is fixed by the bitness. */
auto archType(const CompileOptions::Arch arch) -> llvm::Triple::ArchType {
    using Arch = CompileOptions::Arch;
    switch (arch) {
    case Arch::X86:
        return llvm::Triple::x86_64;
    case Arch::Arm:
        return llvm::Triple::aarch64;
    case Arch::RiscV:
        return llvm::Triple::riscv64;
    case Arch::PowerPc:
        return llvm::Triple::ppc64;
    case Arch::Mips:
        return llvm::Triple::mips64;
    case Arch::Wasm:
        return llvm::Triple::wasm32;
    case Arch::C:
    case Arch::Default:
        return llvm::Triple::UnknownArch;
    }
    return llvm::Triple::UnknownArch;
}

/** Resolve the target triple from the options, defaulting each field to the host. */
auto buildTriple(const CompileOptions& options) -> llvm::Triple {
    llvm::Triple triple { llvm::sys::getDefaultTargetTriple() };

    if (options.getArch() != CompileOptions::Arch::Default) {
        triple.setArch(archType(options.getArch()));
    }
    if (options.getBitness() == CompileOptions::Bitness::Bits32) {
        triple = triple.get32BitArchVariant();
    } else if (options.getBitness() == CompileOptions::Bitness::Bits64) {
        triple = triple.get64BitArchVariant();
    }

    switch (options.getPlatform()) {
    case CompileOptions::Platform::Default:
        break;
    case CompileOptions::Platform::Linux:
        triple.setVendor(llvm::Triple::UnknownVendor);
        triple.setOS(llvm::Triple::Linux);
        triple.setEnvironment(llvm::Triple::GNU);
        break;
    case CompileOptions::Platform::Windows:
        triple.setVendor(llvm::Triple::PC);
        triple.setOS(llvm::Triple::Win32);
        triple.setEnvironment(llvm::Triple::MSVC);
        break;
    case CompileOptions::Platform::MacOS:
        triple.setVendor(llvm::Triple::Apple);
        triple.setOS(llvm::Triple::MacOSX);
        triple.setEnvironment(llvm::Triple::UnknownEnvironment);
        break;
    }

    return triple;
}
} // namespace

Context::Context(CompileOptions options)
: m_options(std::move(options))
, m_triple(buildTriple(m_options))
, m_diagEngine(*this)
, m_typeFactory(*this) {}

auto Context::retain(const llvm::StringRef string) -> llvm::StringRef {
    return m_strings.insert(string).first->first();
}
