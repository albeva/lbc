//
// Created by Albert on 26/02/2022.
//
#pragma once
#include "Pass.hpp"

namespace lbc {
class Symbol;
class TypeRoot;
struct AstModule;
struct AstStmtList;
struct AstDecl;
struct AstDeclList;
struct AstFuncDecl;
struct AstUdtDecl;
struct AstTypeAlias;
struct AstFuncParamDecl;

namespace Sem {

    /**
     * Forward declare all user defined types, aliases and precedures
     */
    class DeclPass final : public Pass {
    public:
        using Pass::Pass;
        void visit(AstModule&) noexcept;
        void define(AstDecl& ast) noexcept;

    private:
        void declare(AstStmtList& ast) noexcept;
        void declare(AstDecl& ast) noexcept;

        void defineFunc(AstFuncDecl& ast) noexcept;
        void defineFuncParam(AstFuncParamDecl& ast) noexcept;
        void defineAlias(AstTypeAlias& ast) noexcept;
        void defineUdt(AstUdtDecl& ast) noexcept;

        Symbol* createParamSymbol(AstFuncParamDecl& ast) noexcept;
        void checkCircularAlias(const TypeRoot* owner, const TypeRoot* aliased) const noexcept;
        void checkCircularDependency(const TypeRoot* udt, const TypeRoot* nested) noexcept;

        using RelKey = std::pair<const TypeRoot*, const TypeRoot*>;
        llvm::DenseMap<RelKey, const TypeRoot*> m_typeRelations{};
    };

} // namespace Sem
} // namespace lbc
