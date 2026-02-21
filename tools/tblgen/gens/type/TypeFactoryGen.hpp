// Custom TableGen backend for generating type factory.
// Reads Types.td and emits TypeFactory.hpp
#pragma once
#include "TypeBaseGen.hpp"

/**
 * TableGen backend that reads Types.td and emits TypeFactoryBase.hpp.
 *
 * Generates the TypeFactoryBase class with typed singleton getters,
 * protected storage, and a kSingleTypeKinds constant array.
 * Extends TypeBaseGen to reuse the type/category hierarchy.
 */
class TypeFactoryGen : public TypeBaseGen {
public:
    static constexpr auto genName = "lbc-type-factory";

    TypeFactoryGen(
        raw_ostream& os,
        const RecordKeeper& records,
        StringRef generator = genName,
        StringRef ns = "lbc",
        std::vector<StringRef> includes = { "pch.hpp", "Type.hpp", "Aggregate.hpp", "Compound.hpp", "Numeric.hpp", "Lexer/TokenKind.hpp" }
    );

    [[nodiscard]] auto run() -> bool override;

private:
    /** Emit the TypeFactoryBase class definition. */
    void factoryClass();

    /** Emit typed getter methods for each singleton type. */
    void singleTypeGetters();
    void keywordToType();
};
