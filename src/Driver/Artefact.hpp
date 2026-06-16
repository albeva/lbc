//
// Created by Albert Varaksin on 16/06/2026.
//
#pragma once
#include "pch.hpp"

namespace lbc {

/**
 * A file produced or supplied during compilation, paired with ownership of it.
 *
 * A temporary artefact is deleted when the owning Artefact is destroyed; a
 * non-temporary one (a requested output, or a user-supplied input) is left in
 * place. Move-only: ownership of the file travels with the value, so threading
 * artefacts through the pipeline cleans up intermediates automatically while
 * leaving final outputs and pre-built inputs untouched.
 */
class Artefact final {
public:
    Artefact() = default;

    /** @param path file path. @param temporary whether to delete the file on destruction. */
    Artefact(std::string path, const bool temporary)
    : m_path(std::move(path))
    , m_temporary(temporary) {}

    Artefact(const Artefact&) = delete;
    auto operator=(const Artefact&) -> Artefact& = delete;

    Artefact(Artefact&& other)
    : m_path(std::move(other.m_path))
    , m_temporary(other.m_temporary) {
        other.m_temporary = false;
    }

    auto operator=(Artefact&& other) -> Artefact& {
        if (this != &other) {
            destroy();
            m_path = std::move(other.m_path);
            m_temporary = other.m_temporary;
            other.m_temporary = false;
        }
        return *this;
    }

    ~Artefact() { destroy(); }

    /** Path to the file. */
    [[nodiscard]] auto path() const -> llvm::StringRef { return m_path; }

    /** Whether the file is a temporary owned (and deleted) by this artefact. */
    [[nodiscard]] auto isTemporary() const -> bool { return m_temporary; }

private:
    /** Delete the file when it is a temporary we still own. */
    void destroy();

    std::string m_path;
    bool m_temporary = false;
};

} // namespace lbc
