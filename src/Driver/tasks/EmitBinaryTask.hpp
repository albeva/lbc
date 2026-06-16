//
// Created by Albert Varaksin on 16/06/2026.
//
#pragma once
#include "Driver/Task.hpp"

namespace lbc {

/**
 * Link stage: links the generated object files into the final executable by
 * shelling out to the host C compiler driver (used as the linker). Unlike the
 * per-source stages it consumes every object at once, so the driver runs it
 * directly rather than through @ref pipeline.
 *
 * Takes the object paths and returns the path to the linked executable.
 */
class EmitBinaryTask final : public Task<std::vector<std::string>, std::string> {
public:
    [[nodiscard]] auto run(Context& context, std::vector<std::string> objects) -> DiagResult<std::string> override;
};

} // namespace lbc
