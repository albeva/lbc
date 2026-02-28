// Custom TableGen backend for generating type base definitions.
// Reads Types.td and emits TypeBase.hpp
#pragma once
#include "GeneratorBase.hpp"
#include "Type.hpp"
namespace type {
/**
 * TableGen backend that reads Types.td and emits TypeBase.hpp.
 *
 * Generates the TypeKind enum, the TypeBase class with per-kind and
 * per-category query predicates, and a keyword-to-type mapping.
 * Singleton types are partitioned to the front of the category list
 * so their TypeKind ordinals form a contiguous range.
 */
class TypeBaseGen : public GeneratorBase {
public:
    /// Generator name used for CLI dispatch
    static constexpr auto genName = "lbc-type-base";

    TypeBaseGen(
        raw_ostream& os,
        const RecordKeeper& records,
        StringRef generator = genName,
        StringRef ns = "lbc",
        std::vector<StringRef> includes = { "pch.hpp", "Lexer/TokenKind.hpp" }
    );

    [[nodiscard]] auto run() -> bool override;

    /** Get all TypeKind records from the TableGen input. */
    [[nodiscard]] auto getTypeKinds() const -> const std::vector<const Record*>& { return m_typeKinds; }

    /** Get all BaseType records from the TableGen input. */
    [[nodiscard]] auto getTypes() const -> const std::vector<const Record*>& { return m_types; }

    /** Get the parsed type categories (partitioned: singletons first). */
    [[nodiscard]] auto getCategories() const -> const std::vector<std::unique_ptr<TypeCategory>>& { return m_categories; }

    /** Get all singleton types (those with a single instance each). */
    [[nodiscard]] auto getSingles() const -> const std::vector<const Type*>& { return m_singles; }

    /** Get all types that have a corresponding keyword token. */
    [[nodiscard]] auto getKeywords() const -> std::vector<const Type*>;

    /**
     * Visit every type across all categories.
     *
     * @param func Callable invoked with each Type pointer.
     */
    template <std::invocable<const Type*> Func>
    void visit(Func&& func) {
        for (const auto& cat : m_categories) {
            for (const auto& type : cat->getTypes()) {
                std::invoke(std::forward<Func>(func), type.get());
            }
        }
    }

private:
    /** Emit the TypeKind enum. */
    void typeKind();

    /** Emit the TypeBase class definition. */
    void typeBaseClass();

    /** Emit per-kind and per-category query predicates inside TypeBase. */
    void typeQueryMethods();

    /** Emit the getTokenKind() keyword mapping inside TypeBase. */
    void typeToKeyword();

    /// Raw TypeKind records from TableGen
    std::vector<const Record*> m_typeKinds;
    /// Raw BaseType records from TableGen
    std::vector<const Record*> m_types;
    /// Parsed categories (owns Type objects)
    std::vector<std::unique_ptr<TypeCategory>> m_categories;
    /// Singleton types (non-owning references into categories)
    std::vector<const Type*> m_singles;
};
} // namespace type
