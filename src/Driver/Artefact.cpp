//
// Created by Albert Varaksin on 16/06/2026.
//
#include "Artefact.hpp"
#include <llvm/Support/FileSystem.h>
using namespace lbc;

void Artefact::destroy() {
    if (m_temporary && !m_path.empty()) {
        std::ignore = llvm::sys::fs::remove(m_path);
    }
}
