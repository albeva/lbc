// Base class for lbc-tblgen generators.
// Extends Builder with RecordKeeper access and common utility methods.
#pragma once
#include <llvm/TableGen/Record.h>
#include <optional>
#include <ranges>
#include <vector>
#include "Builder.hpp"
using namespace llvm;

class GeneratorBase : public Builder {
public:
    GeneratorBase(
        raw_ostream& os,
        const RecordKeeper& records,
        const StringRef generator,
        const StringRef ns = "lbc",
        std::vector<StringRef> includes = { "\"pch.hpp\"" }
    )
    : Builder(os, records.getInputFilename(), generator, ns, std::move(includes))
    , m_records(records) {}

    [[nodiscard]] virtual auto run() -> bool = 0;

    /**
     * Sort records by their definition order.
     */
    [[nodiscard]] static auto sortedByDef(ArrayRef<const Record*> arr) -> std::vector<const Record*> {
        std::vector<const Record*> result { arr };
        std::ranges::sort(result, [](const Record* one, const Record* two) {
            return one->getID() < two->getID();
        });
        return result;
    }

    /**
     * Find the first and last record whose field matches the given record.
     */
    [[nodiscard]] static auto findRange(
        const std::vector<const Record*>& records,
        StringRef field,
        const Record* record
    ) -> std::optional<std::pair<const Record*, const Record*>> {
        const auto pred = [&](const auto* token) {
            const auto* value = token->getValue(field);
            return value != nullptr && value->getValue() == record->getDefInit();
        };

        const auto first = std::ranges::find_if(records, pred);
        if (first == std::ranges::end(records)) {
            return std::nullopt;
        }

        const auto lastPrev = std::ranges::find_if(records | std::views::reverse, pred);
        const auto last = std::prev(lastPrev.base());
        return std::make_pair(*first, *last);
    }

    /**
     * Collect all records whose field matches the given record.
     */
    [[nodiscard]] static auto collect(
        const std::vector<const Record*>& records,
        const StringRef field,
        const Record* record
    ) -> std::vector<const Record*> {
        const auto pred = [&](const Record* rec) {
            if (rec->isValueUnset(field)) {
                return false;
            }
            const auto* value = rec->getValue(field);
            return value != nullptr && value->getValue() == record->getDefInit();
        };

        std::vector<const Record*> result {};
        for (const auto* token : records) {
            if (pred(token)) {
                result.push_back(token);
            }
        }
        return result;
    }

    /**
     * Check if any record's field matches the given record.
     */
    [[nodiscard]] static auto contains(
        const std::vector<const Record*>& records,
        StringRef field,
        const Record* record
    ) -> bool {
        const auto pred = [&](const auto* token) {
            const auto* value = token->getValue(field);
            return value != nullptr && value->getValue() == record->getDefInit();
        };
        return std::ranges::find_if(records, pred) != records.end();
    }

protected:
    const RecordKeeper& m_records; // NOLINT(*-avoid-const-or-ref-data-members, *-non-private-member-variables-in-classes)
};
