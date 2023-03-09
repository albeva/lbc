//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"

// STL
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <filesystem>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace lbc {
namespace fs = std::filesystem;
using namespace std::literals::string_literals;
} // namespace lbc

// LLVM
#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4100 4127 4242 4244 4245 4267 4310 4324 4456 4458 4624)
#endif
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#if defined(_MSC_VER)
#    pragma warning(pop)
#endif

// APP
#if defined(_MSC_VER)
#    pragma warning(disable : 4291)
#endif
#include "Utils/Utils.hpp"
#include "Utils/Version.hpp"
