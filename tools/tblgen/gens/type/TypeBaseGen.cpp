// Custom TableGen backend for generating type base definitions.
// Reads Types.td and emits TypeBase.hpp
#include "TypeBaseGen.hpp"

TypeBaseGen::TypeBaseGen(
    raw_ostream& os,
    const RecordKeeper& records,
    StringRef generator,
    StringRef ns,
    std::vector<StringRef> includes
)
: GeneratorBase(os, records, generator, ns, std::move(includes))
, m_typeKinds(sortedByDef(m_records.getAllDerivedDefinitions("TypeKind")))
, m_types(sortedByDef(m_records.getAllDerivedDefinitions("BaseType"))) {
    m_categories.reserve(m_types.size());
    for (const auto* record : m_typeKinds) {
        m_categories.emplace_back(std::make_unique<TypeCategory>(record, *this));
    }
}

auto TypeBaseGen::run() -> bool {
    typeKind();
    typeBase();
    return false;
}

void TypeBaseGen::typeKind() {
    doc("Enumerate type kinds");
    block("enum class TypeKind : std::uint8_t", true, [&] {
        for (const auto& cat : m_categories) {
            for (const auto& type : cat->getTypes()) {
                line(type->getEnumName(), ",");
            }
        }
    });
}

void TypeBaseGen::typeBase() {
    doc("Base class for types");
    block("class TypeBase", true, [&] {
        scope(Scope::Public, true);
        line("NO_COPY_AND_MOVE(TypeBase)", "");
        newline();
        line("TypeBase() = delete");
        line("constexpr virtual ~TypeBase() noexcept = default");
        newline();

        line("explicit constexpr TypeBase(const TypeKind kind)", "");
        line(": m_kind(kind) { }", "");
        newline();

        scope(Scope::Private);
        line("TypeKind m_kind");
    });
}
