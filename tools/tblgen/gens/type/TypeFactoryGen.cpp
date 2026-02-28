// Custom TableGen backend for generating type factory.
// Reads Types.td and emits TypeFactoryBase.hpp
#include "TypeFactoryGen.hpp"
#include <llvm/ADT/StringSet.h>
using namespace type;

TypeFactoryGen::TypeFactoryGen(
    raw_ostream& os,
    const RecordKeeper& records,
    StringRef generator,
    StringRef ns,
    std::vector<StringRef> includes
)
: TypeBaseGen(os, records, generator, ns, std::move(includes)) {
}

auto TypeFactoryGen::run() -> bool {
    factoryClass();
    return false;
}

void TypeFactoryGen::factoryClass() {
    doc([&] {
        lines("Generated base class for the type factory.");
        lines("");
        lines("Provides typed getters for singleton types and protected");
        lines("storage for type instances indexed by TypeKind. Subclasses");
        lines("are responsible for allocating and registering types via");
        lines("setSingleton().");
    });
    block("class TypeFactoryBase", true, [&]() {
        scope(Scope::Public, true);
        line("NO_COPY_AND_MOVE(TypeFactoryBase)", "");

        line("TypeFactoryBase() = default");
        line("virtual ~TypeFactoryBase() = default");
        newline();

        singletonGetters();
        keywordToType();
        newline();

        scope(Scope::Protected);

        doc("Retrieve a singleton type by its TypeKind.");
        block("[[nodiscard]] auto getSingleton(const TypeKind kind) const -> const Type*", [&] {
            line("const auto index = static_cast<std::size_t>(kind)");
            line("return m_singletons.at(index)");
        });
        newline();

        doc("Register a singleton type, indexed by its TypeKind.");
        block("void setSingleton(const Type* type)", [&] {
            line("const auto index = static_cast<std::size_t>(type->getKind())");
            line("m_singletons.at(index) = type");
        });
        newline();

        comment("Number of singleton types");
        line("static constexpr std::size_t COUNT = " + std::to_string(getSingles().size()));
        comment("TypeKind values for all singleton types");
        block("static constexpr std::array<TypeKind, COUNT> kSingletonKinds", true, [&] {
            for (const auto* single : getSingles()) {
                line("TypeKind::" + single->getEnumName(), ",");
            }
        });
        newline();

        scope(Scope::Private);
        comment("Storage for singleton type instances, indexed by TypeKind ordinal");
        line("std::array<const Type*, COUNT> m_singletons {}");
    });
}

void TypeFactoryGen::singletonGetters() {
    line("// NOLINTBEGIN(*-static-cast-downcast)", "");
    newline();
    llvm::SmallPtrSet<const TypeCategory*, 16> set {};
    for (const auto* single : getSingles()) {
        const auto backingClass = single->getBackingClassName();
        const auto backingName = backingClass.value_or("Type");

        if (set.insert(single->getCategory()).second) {
            section((single->getCategory()->getRecord()->getName() + " types").str());
        }

        block("[[nodiscard]] auto get" + ucfirst(single->getEnumName()) + "() const -> const " + backingName + "*", [&] {
            const auto needCast = backingClass.has_value();
            space();
            add("return ");
            if (needCast) {
                add("static_cast<const " + backingName + "*>(");
            }
            add("getSingleton(TypeKind::" + single->getEnumName() + ")");
            if (needCast) {
                add(")");
            }
            add(";\n");
        });
        newline();
    }
    line("// NOLINTEND(*-static-cast-downcast)", "");
    newline();
}

void TypeFactoryGen::keywordToType() {
    doc("Get type for given TokenKind or a nullptr");
    block("[[nodiscard]] constexpr auto getType(const TokenKind kind) const -> const Type*", [&] {
        block("switch (kind.value())", [&] {
            for (const auto& type : getKeywords()) {
                line("case TokenKind::" + type->getEnumName(), ":");
                line("    return get" + type->getEnumName() + "()");
            }
            line("default", ":");
            line("    return nullptr");
        });
    });
}
