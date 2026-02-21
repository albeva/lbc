// Custom TableGen backend for generating type factory.
// Reads Types.td and emits TypeFactory.hpp
#pragma once
#include "TypeBaseGen.hpp"

/**
 * TableGen backend that reads Types.td and emits TypeFactory.hpp.
 */
class TypeFactoryGen : public TypeBaseGen {
public:
    static constexpr auto genName = "lbc-type-factory";

    TypeFactoryGen(
        raw_ostream& os,
        const RecordKeeper& records,
        StringRef generator = genName,
        StringRef ns = "lbc",
        std::vector<StringRef> includes = { "pch.hpp", "Type.hpp", "Aggregate.hpp", "Compound.hpp", "Numeric.hpp" }
    );

    [[nodiscard]] auto run() -> bool override;

private:
    void factoryClass();
    void singleTypeGetters();
};
