//
// Created by Albert Varaksin on 21/02/2026.
//
#include "Type.hpp"
#include "TypeBaseGen.hpp"

TypeCategory::TypeCategory(const Record* record, const TypeBaseGen& gen)
: m_record(record) {
    const auto types = TypeBaseGen::collect(gen.getTypes(), "kind", record);
    for (const auto* type : types) {
        m_types.emplace_back(std::make_unique<Type>(type, this));
    }
}

Type::Type(const Record* record, const TypeCategory* category)
: m_record(record)
, m_category(category)
, m_enumName(record->getName()) {
    m_enumName.consume_back("Type");
}
