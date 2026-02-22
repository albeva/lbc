//
// Created by Albert Varaksin on 14/02/2026.
//
#pragma once
#include <utility>
#include "../../GeneratorBase.hpp"
#include "AstClass.hpp"

// -----------------------------------------------------------------------------
// The generator
// -----------------------------------------------------------------------------

/**
 * TableGen backend that reads Ast.td and emits Ast.hpp. Builds an in-memory
 * AstClass tree mirroring the Node/Group/Leaf hierarchy, then walks it to
 * generate: AstKind enum, forward declarations, and complete C++ class
 * definitions with constructors, accessors, and data members.
 */
class AstGen : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-ast-def";

    AstGen(
        raw_ostream& os,
        const RecordKeeper& records,
        StringRef generator = genName,
        StringRef ns = "lbc",
        std::vector<StringRef> includes = {
            "pch.hpp", "Symbol/LiteralValue.hpp", "Lexer/TokenKind.hpp"
        }
    );

    [[nodiscard]] auto run() -> bool override;

    [[nodiscard]] auto getNodeRecords() const -> const std::vector<const Record*>& { return nodeRecords; }
    [[nodiscard]] auto getRoot() const -> const AstClass* { return m_root.get(); }
    [[nodiscard]] auto getNodeClass() const -> const Record* { return m_nodeClass; }
    [[nodiscard]] auto getLeafClass() const -> const Record* { return m_leafClass; }
    [[nodiscard]] auto getGroupClass() const -> const Record* { return m_groupClass; }
    [[nodiscard]] auto getArgClass() const -> const Record* { return m_argClass; }
    [[nodiscard]] auto getFuncClass() const -> const Record* { return m_funcClass; }

private:
    void forwardDecls();
    void astNodesEnum();
    void astForwardDecls();
    void astGroup(AstClass* cls);
    void astClass(AstClass* cls);
    void constructor(AstClass* cls);
    void classof(AstClass* cls);
    void functions(AstClass* cls);
    void classArgs(AstClass* cls);

    /// Root of the AstClass tree, built in the constructor
    std::unique_ptr<AstClass> m_root;
    std::vector<const Record*> nodeRecords;

    const Record* m_nodeClass;
    const Record* m_leafClass;
    const Record* m_groupClass;
    const Record* m_argClass;
    const Record* m_funcClass;
};
