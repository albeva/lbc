//
// Created by Albert Varaksin on 21/02/2026.
//
#include "Type.hpp"
#include "TypeBaseGen.hpp"
using namespace type;

TypeCategory::TypeCategory(const Record* record, const TypeBaseGen& gen)
: m_record(record) {
    const auto types = TypeBaseGen::collect(gen.getTypes(), "kind", record);
    for (const auto* type : types) {
        m_types.emplace_back(std::make_unique<Type>(type, this));
    }
}

auto TypeCategory::isSingle() const -> bool {
    return m_record->getValueAsBit("single");
}

Type::Type(const Record* record, const TypeCategory* category)
: m_record(record)
, m_category(category)
, m_enumName(record->getName()) {
    m_enumName.consume_back("Type");
}

auto Type::getBackingClassName() const -> std::optional<llvm::StringRef> {
    const auto value = m_record->getValueAsString("cls");
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
}
