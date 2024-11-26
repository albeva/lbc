//
// Created by Albert Varaksin on 30/04/2021.
//
#pragma once
#include "pch.hpp"
#include "CompileOptions.hpp"
#include <utility>

namespace lbc {

/**
 * Class that represents a source file
 */
struct Source final {
    NO_COPY_AND_MOVE(Source)

    Source(CompileOptions::FileType ty, fs::path path_, bool gen, const Source* src)
    : type{ ty }, path{ std::move(path_) }, isGenerated{ gen }, origin{ src == nullptr ? *this : *src } {}

    ~Source() = default;

    [[nodiscard]] static auto create(CompileOptions::FileType type, fs::path path, bool generated, const Source* origin = nullptr) -> std::unique_ptr<Source> {
        return std::make_unique<Source>(type, std::forward<fs::path>(path), generated, origin);
    }

    const CompileOptions::FileType type;
    const fs::path path;
    const bool isGenerated;
    const Source& origin;

    /**
     * Derive new generated Source with the same origin
     */
    [[nodiscard]] auto derive(CompileOptions::FileType ty, fs::path pth) const -> std::unique_ptr<Source> {
        return create(ty, std::move(pth), true, &origin);
    }
};

} // namespace lbc