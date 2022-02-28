//
// Created by Albert on 26/02/2022.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
class Symbol;
class TypeProxy;
class TypeRoot;
struct AstModule;
struct AstStmtList;
struct AstDecl;
struct AstDeclList;
struct AstFuncDecl;
struct AstUdtDecl;
struct AstTypeAlias;

namespace Sem {

    /**
     * Forward declare all user defined types, aliases and precedures
     */
    class ForwardDeclPass final : public Pass {
    public:
        using Pass::Pass;
        void visit(AstModule&) noexcept;

    private:
        void declare(AstStmtList& ast) noexcept;
        void declare(AstDeclList& ast) noexcept;
        void declare(AstDecl& ast) noexcept;

        void define(AstDecl& ast) noexcept;
        void define(AstTypeAlias& ast) noexcept;
        void define(AstUdtDecl& ast) noexcept;

        void implement(AstUdtDecl& ast) noexcept;

        [[nodiscard]] bool isCircularAlias(TypeProxy* proxy, TypeProxy* aliased) const noexcept;
        void checkCircularDependency(const TypeRoot* udt, const TypeRoot* nested) noexcept;

        std::vector<AstDecl*> m_nodes{};
        std::vector<AstUdtDecl*> m_udts{};
        using RelKey = std::pair<const TypeRoot*, const TypeRoot*>;
        llvm::DenseMap<RelKey, const TypeRoot*> m_typeRelations{};
    };

} // namespace Sem
} // namespace lbc
