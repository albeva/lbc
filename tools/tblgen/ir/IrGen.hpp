//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>
#include "GeneratorBase.hpp"
#include "IrClass.hpp"
namespace ir {

/** TableGen backend that reads Instructions.td and emits Instructions.hpp. */
class IrGen final : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-ir-inst-def";

    IrGen(raw_ostream& os, const RecordKeeper& records);
    ~IrGen() override;

    [[nodiscard]] auto run() -> bool override;
    void forwardDecls();
    void irNodesEnum();
    void irGroup(const IrClass* cls);
    void irClass(const IrClass* cls);
    void constructor(const IrClass* cls);
    void classof(const IrClass* cls);
    void functions(const IrClass* cls);
    void classArgs(const IrClass* cls);

    [[nodiscard]] auto getNodeRecords() const -> const std::vector<const Record*>& { return nodeRecords; }
    [[nodiscard]] auto getRoot() const -> const IrClass* { return m_root.get(); }
    [[nodiscard]] auto getNodeClass() const -> const Record* { return m_nodeClass; }
    [[nodiscard]] auto getLeafClass() const -> const Record* { return m_leafClass; }
    [[nodiscard]] auto getGroupClass() const -> const Record* { return m_groupClass; }
    [[nodiscard]] auto getArgClass() const -> const Record* { return m_argClass; }
    [[nodiscard]] auto getFuncClass() const -> const Record* { return m_funcClass; }

private:
    /// Root of the AstClass tree, built in the constructor
    std::unique_ptr<IrClass> m_root;
    std::vector<const Record*> nodeRecords;

    const Record* m_nodeClass;
    const Record* m_leafClass;
    const Record* m_groupClass;
    const Record* m_argClass;
    const Record* m_funcClass;
};
} // namespace ir
