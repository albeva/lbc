//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "lib/NodeGen.hpp"
namespace ir {
using namespace llvm;

class IrNodeClass final : public lib::NodeClass {
public:
    using NodeClass::NodeClass;
};

/** TableGen backend that reads Instructions.td and emits Instructions.hpp. */
class IrGen final : public lib::NodeGen<IrNodeClass, lib::NodeArg> {
public:
    static constexpr auto genName = "lbc-ir-inst-def";

    IrGen(raw_ostream& os, const RecordKeeper& records);

    [[nodiscard]] auto run() -> bool override;

    void forwardDecls();
    void irNodesEnum();
    void irGroup(const lib::NodeClass* cls);
    void irClass(const lib::NodeClass* cls);
    void constructor(const lib::NodeClass* cls);
    void classof(const lib::NodeClass* cls);
    void functions(const lib::NodeClass* cls);
    void classArgs(const lib::NodeClass* cls);
};
} // namespace ir
