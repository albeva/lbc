// Custom TableGen backend for generating type factory.
// Reads Types.td and emits TypeFactory.hpp
#include "TypeFactoryGen.hpp"
#include <llvm/ADT/StringSet.h>

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
    newline();
    factoryClass();
    return false;
}

void TypeFactoryGen::factoryClass() {
    doc("Factory for retrieving and creating types");
    block("class TypeFactoryBase", true, [&]() {
        scope(Scope::Public, true);
        line("NO_COPY_AND_MOVE(TypeFactoryBase)", "");

        line("TypeFactoryBase() = default");
        line("virtual ~TypeFactoryBase() = default");
        newline();

        singleTypeGetters();

        scope(Scope::Protected);
        newline();

        comment("Get type from m_singleTypes based on TypeKind");
        block("[[nodiscard]] auto getSingleton(const TypeKind kind) const -> const Type*", [&] {
            line("const auto index = static_cast<std::size_t>(kind)");
            line("return m_singleTypes.at(index)");
        });
        newline();

        comment("Set type to m_singleTypes based on TypeKind");
        block("void setSingleton(const Type* type)", [&] {
            line("const auto index = static_cast<std::size_t>(type->getKind())");
            line("m_singleTypes.at(index) = type");
        });
        newline();

        line("static constexpr std::size_t COUNT = " + std::to_string(getSingles().size()));
        block("static constexpr std::array<TypeKind, COUNT> kSingleTypeKinds", true, [&] {
            for (const auto* single : getSingles()) {
                line("TypeKind::" + single->getEnumName(), ",");
            }
        });
        newline();

        scope(Scope::Private);
        line("std::array<const Type*, COUNT> m_singleTypes {}");
    });
}

void TypeFactoryGen::singleTypeGetters() {
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
