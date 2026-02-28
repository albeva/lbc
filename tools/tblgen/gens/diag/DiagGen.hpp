//
// Created by Albert Varaksin on 16/02/2026.
//
#pragma once
#include "GeneratorBase.hpp"
namespace diag {
/**
 * TableGen backend that reads Diagnostics.td and emits Diagnostics.hpp.
 * Parses format string placeholders ({name} / {name:type}) to extract
 * typed parameters for each diagnostic message.
 */
class DiagGen : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-diag-def";

    DiagGen(
        raw_ostream& os,
        const RecordKeeper& records,
        StringRef generator = genName,
        StringRef ns = "lbc",
        std::vector<StringRef> includes = { "pch.hpp" }
    );

    [[nodiscard]] auto run() -> bool override;

private:
    void diagKind();
    void diagKindEnums();
    void diagKindConstructors();
    void diagKindAccessors();
    void diagKindCollections();
    void diagKindFormatter();
    void diagnosticFunctions();
    void category(const Record* cat);
    void diagnostic(const Record* record);
    static auto messageSpec(const Record* record) -> std::pair<std::string, std::string>;
    static auto getCategory(const Record* record) -> llvm::StringRef;
    static auto getSeverity(const Record* record) -> llvm::StringRef;

    std::vector<const Record*> m_categories;
    std::vector<const Record*> m_severities;
    std::vector<const Record*> m_diagnostics;
};
} // namespace diag
