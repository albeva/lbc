//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>
namespace ir {
using namespace llvm;
class IRInstGen;
class Category;

/// represent an instruction
class Instruction final {
public:
    Instruction(const Record* record, const Category* category, const IRInstGen& gen);

    [[nodiscard]] auto getName() const -> StringRef { return m_record->getName(); }
    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }
    [[nodiscard]] auto getCategory() const -> const Category* { return m_category; }

private:
    const Record* m_record;
    const Category* m_category;
};

/// instruction arguments
class Arg final {
public:
    explicit Arg(const Record* record);

    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }
    [[nodiscard]] auto getCpp() const -> StringRef { return m_cpp; }
    [[nodiscard]] auto getName() const -> StringRef { return m_name; }
    [[nodiscard]] auto isVarArg() const -> bool { return m_vararg; }

private:
    const Record* m_record;
    StringRef m_cpp;
    StringRef m_name;
    bool m_vararg;
};

/// represent instructions category
class Category final {
public:
    enum class Kind : std::uint8_t {
        Group,
        Instruction
    };

    Category(const Record* record, const Category* parent, const IRInstGen& gen);

    [[nodiscard]] auto getName() const -> std::string;
    [[nodiscard]] auto getClassName() const -> std::string;
    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }
    [[nodiscard]] auto getInstructions() const -> const auto& { return m_instructions; }
    [[nodiscard]] auto getParent() const -> const Category* { return m_parent; }
    [[nodiscard]] auto getChildren() const -> const auto& { return m_children; }
    [[nodiscard]] auto getArgs() const -> const auto& { return m_args; }

    template<std::invocable<const Category*> Func>
    void visit(Func&& func) {
        std::invoke(std::forward<Func>(func), this);
        for (const auto& child : m_children) {
            child->visit(std::forward<Func>(func));
        }
    }

    template<std::invocable<const Instruction*> Func>
    void visit(Func&& func) {
        for (const auto& instr : m_instructions) {
            std::invoke(std::forward<Func>(func), instr.get());
        }
        for (const auto& child : m_children) {
            child->visit(std::forward<Func>(func));
        }
    }

private:
    const Record* m_record;
    const Category* m_parent;
    std::vector<std::unique_ptr<Instruction>> m_instructions;
    std::vector<std::unique_ptr<Category>> m_children;
    std::vector<std::unique_ptr<Arg>> m_args;
};
} // namespace ir
