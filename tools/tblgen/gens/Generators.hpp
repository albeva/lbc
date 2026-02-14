#pragma once
#include "../GeneratorBase.hpp"

class TokensGenerator final : public GeneratorBase {
public:
    using GeneratorBase::GeneratorBase;
    [[nodiscard]] auto run() -> bool final;
};

class AstGenerator final : public GeneratorBase {
public:
    using GeneratorBase::GeneratorBase;
    [[nodiscard]] auto run() -> bool final;
};
