//
// Created by Albert Varaksin on 01/03/2026.
//
#include "TreeGen.hpp"
using namespace lib;
using namespace std::string_literals;

void TreeGenBase::treeKindEnum() {
    doc("Enumerates all concrete tree nodes");
    block("enum class " + getKindEnumName() + " : std::uint8_t", true, [&] {
        getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const auto* node) {
            line(node->getEnumName(), ",");
        });
    });
}

void TreeGenBase::treeForwardDeclare() {
    section("Forward Declarations");
    getRoot()->visit([&](const auto* node) {
        line("class " + node->getClassName());
    });
    newline();
}

void TreeGenBase::treeGroups(const TreeNode* node) {
    if (node->isLeaf()) {
        treeNode(node);
    } else {
        section((node->getRecord()->getName() + " nodes").str());
        treeNode(node);
        for (const auto& child : node->getChildren()) {
            treeGroups(child.get());
        }
    }
}

void TreeGenBase::treeNode(const TreeNode* node) {
    if (node->isGroup()) {
        doc("Abstract base for all " + node->getRecord()->getValueAsString("desc").str() + " nodes");
    } else {
        doc(node->getRecord()->getValueAsString("desc"));
    }
    klass(node->getClassName(), { .extends = node->getBaseClassName(), .isFinal = node->isLeaf() }, [&] {
        m_scope = Scope::Private;
        if (node->isRoot()) {
            scope(Scope::Public);
            line("NO_COPY_AND_MOVE(" + node->getClassName() + ")", "");
            newline();
        }

        treeNodeConstructor(node);
        treeNodeClassOf(node);
        treeNodeMethods(node);
        treeNodeData(node);
    });
    newline();
}

void TreeGenBase::treeNodeConstructor(const TreeNode* node) {
    if (node->isLeaf()) {
        scope(Scope::Public);
    } else {
        scope(Scope::Protected);
    }

    // constructor
    if (node->isLeaf() || node->isRoot() || node->hasOwnCtorParams()) {
        const auto params = node->ctorParams();
        const bool isExplicit = params.size() <= 1;
        doc("Construct "s + articulate(node->getClassName()) + node->getClassName() + " node");
        line("constexpr "s + (isExplicit ? "explicit " : "") + node->getClassName() + "(", "");
        indent(false, [&] {
            if (node->isRoot() || node->isGroup()) {
                line("const " + getKindEnumName() + " kind", params.empty() ? "" : ",");
            }
            list(params, { .suffix = "," });
        });
        line(")", "");

        // constructor init
        if (node->isRoot()) {
            line(": m_kind(kind)", params.empty() ? " {}" : "");
        }
        list(
            node->ctorInitParams(),
            { .firstPrefix = (node->isRoot() ? ", " : ": "), .prefix = ", ", .lastSuffix = " {}" }
        );
    } else {
        line("using " + node->getParent()->getClassName() + "::" + node->getParent()->getClassName());
    }
    newline();
}

void TreeGenBase::treeNodeClassOf(const TreeNode* node) {
    scope(Scope::Public);

    const auto range = node->getLeafRange();

    if (node->isRoot()) {
        classof(getRoot()->getClassName(), "getKind", "");
    } else if (range) {
        if (range->first == range->second) {
            classof(getRoot()->getClassName(), "getKind", getKindEnumName(), range->first->getEnumName());
        } else {
            classof(getRoot()->getClassName(), "getKind", getKindEnumName(), range->first->getEnumName(), range->second->getEnumName());
        }
    }
}

void TreeGenBase::treeNodeMethods(const TreeNode* node) {
    const auto functions = node->classFunctions();
    if (functions.empty() && !node->isRoot()) {
        return;
    }
    newline();
    scope(Scope::Public);

    std::size_t count = 0;
    getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const lib::TreeNode* /* cls */) {
        count++;
    });

    if (node->isRoot()) {
        comment("Number of leaf nodes");
        line("static constexpr std::size_t NODE_COUNT = " + std::to_string(count));
        newline();

        comment("Get the kind discriminator for this node");
        block("[[nodiscard]] constexpr auto getKind() const -> " + getKindEnumName(), [&] {
            line("return m_kind");
        });
        newline();

        comment("Get node class name");
        block("[[nodiscard]] constexpr auto getClassName() const -> llvm::StringRef", [&] {
            line("const auto index = static_cast<std::size_t>(m_kind)");
            line("return kClassNames.at(index)");
        });
        newline();
    }

    for (const auto& func : functions) {
        lines(func, "\n");
        newline();
    }
}

void TreeGenBase::treeNodeData(const TreeNode* node) {
    const auto args = node->classArgs();
    if (args.empty() && !node->isRoot()) {
        return;
    }

    scope(Scope::Private);
    if (node->isRoot()) {
        line(getKindEnumName() + " m_kind");
    }
    list(args, {});

    if (node->isRoot()) {
        std::vector<std::string> classes;
        getRoot()->visit(TreeNode::Kind::Leaf, [&](const TreeNode* leaf) {
            classes.push_back(leaf->getClassName());
        });

        block("static constexpr std::array<llvm::StringRef, NODE_COUNT> kClassNames", true, [&] {
            list(classes, { .suffix = ",", .quote = true });
        });
    }
}
