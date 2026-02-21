// Custom TableGen backend for generating type base definitions.
// Reads Types.td and emits TypeBase.hpp
#pragma once
#include "../../GeneratorBase.hpp"
#include "Type.hpp"

/**
 * TableGen backend that reads Types.td and emits TypeBase.hpp.
 */
class TypeBaseGen : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-type-base";

    TypeBaseGen(
        raw_ostream& os,
        const RecordKeeper& records,
        StringRef generator = genName,
        StringRef ns = "lbc",
        std::vector<StringRef> includes = { "pch.hpp", "Lexer/TokenKind.hpp" }
    );

    [[nodiscard]] auto run() -> bool override;

    [[nodiscard]] auto getTypeKinds() const -> const std::vector<const Record*>& { return m_typeKinds; }
    [[nodiscard]] auto getTypes() const -> const std::vector<const Record*>& { return m_types; }
    [[nodiscard]] auto getCategories() const -> const std::vector<std::unique_ptr<TypeCategory>>& { return m_categories; }
    [[nodiscard]] auto getSingles() const -> const std::vector<const Type*>& { return m_singles; }
    [[nodiscard]] auto getKeywords() const -> std::vector<const Type*>;

    template <std::invocable<const Type*> Func>
    void visit(Func&& func) {
        for (const auto& cat : m_categories) {
            for (const auto& type : cat->getTypes()) {
                std::invoke(std::forward<Func>(func), type.get());
            }
        }
    }

private:
    void typeKind();
    void typeBaseClass();
    void typeQueryMethods();
    void typeToKeyword();

    std::vector<const Record*> m_typeKinds;
    std::vector<const Record*> m_types;
    std::vector<std::unique_ptr<TypeCategory>> m_categories;
    std::vector<const Type*> m_singles;
};
