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

    // single types should be at the top, to ensure predictable index range
    std::ranges::stable_partition(m_categories, &TypeCategory::isSingle);

    // filter all the single types
    visit([&](const Type* type) {
        if (type->getCategory()->isSingle()) {
            m_singles.emplace_back(type);
        }
    });
}

auto TypeBaseGen::run() -> bool {
    typeKind();
    typeBaseClass();
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

void TypeBaseGen::typeBaseClass() {
    doc("Base class for types");
    block("class TypeBase", true, [&] {
        scope(Scope::Public, true);
        line("NO_COPY_AND_MOVE(TypeBase)", "");
        newline();
        line("TypeBase() = delete");
        line("virtual ~TypeBase() = default");
        newline();

        comment("Get underlying type kind");
        getter("kind", "TypeKind");
        newline();

        typeQueryMethods();

        const auto* keywordType = m_records.getClass("KeywordType");
        if (keywordType != nullptr) {
            comment("Is it a built-in (with a keyword) type");
            predicate("builtin", true, [&] {
                block("switch (m_kind)", false, [&] {
                    bool found = false;
                    for (const auto& cat : getCategories()) {
                        for (const auto& type : cat->getTypes()) {
                            if (type->getRecord()->hasDirectSuperClass(keywordType)) {
                                found = true;
                                line("case TypeKind::" + type->getEnumName(), ":");
                            }
                        }
                    }
                    if (found) {
                        line("    return true");
                    }
                    line("default", ":");
                    line("    return false");
                });
            });
            newline();
        }

        scope(Scope::Protected);
        line("explicit constexpr TypeBase(const TypeKind kind)", "");
        line(": m_kind(kind) { }", "");
        newline();

        scope(Scope::Private);
        line("TypeKind m_kind");
    });
}

void TypeBaseGen::typeQueryMethods() {
    section("Basic type queries");

    for (const auto& cat : m_categories) {
        const auto& types = cat->getTypes();
        // is category check
        comment(cat->getRecord()->getName() + " types");
        predicate(cat->getRecord()->getName(), true, [&] {
            if (types.empty()) {
                line("return false");
                return;
            }
            const auto* first = types.front().get();
            const auto* last = types.back().get();
            if (first == last) {
                line("return m_kind == TypeKind::" + first->getEnumName());
            } else {
                line("return m_kind >= TypeKind::" + first->getEnumName() + " && m_kind <= TypeKind::" + last->getEnumName());
            }
        });
        // is type check
        for (const auto& type : types) {
            predicate(type->getEnumName(), true, "m_kind == TypeKind::" + type->getEnumName());
        }
        newline();
    }
}
