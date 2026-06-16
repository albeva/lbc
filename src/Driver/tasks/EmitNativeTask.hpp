//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "Driver/Task.hpp"

namespace lbc {

/**
 * Native emission stage: lowers a bitcode file to a native object or assembly
 * file by shelling out to the LLVM code generator (`llc`). The output kind
 * (object vs assembly) and target (arch / pointer width) are read from the
 * context. When building an executable the object is an intermediate to be
 * linked, so it is written to a temporary; otherwise it goes to the configured
 * output path.
 *
 * Takes the input bitcode path and returns the path to the artifact it wrote
 * (for the linker). Assembly is emitted in Intel syntax.
 */
class EmitNativeTask final : public Task<std::string, std::string> {
public:
    [[nodiscard]] auto run(Context& context, std::string input) -> DiagResult<std::string> override;
};

} // namespace lbc
