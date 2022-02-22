//
// Created by Albert Varaksin on 01/05/2021.
//
#pragma once
#include <Ast/Ast.hpp>

namespace lbc {
class Symbol;
class SemanticAnalyzer;
struct AstModule;
struct AstFuncDecl;
struct AstFuncParamDecl;

namespace Sem {
    class TypePass;

    /**
     * Semantic pass that declares all the functions
     * and declarations in the ast
     */
    class FuncDeclarerPass final {
    public:
        NO_COPY_AND_MOVE(FuncDeclarerPass)

        explicit FuncDeclarerPass(
            SemanticAnalyzer& sem,
            TypePass& typePass) noexcept
        : m_sem{ sem },
          m_typePass{ typePass } {}

        ~FuncDeclarerPass() noexcept = default;

        void visit(AstModule& ast);

    private:
        void visit(lbc::AstStmtList& ast);
        void visitFuncDecl(AstFuncDecl& ast, bool external);
        void visitFuncParamDecl(AstFuncParamDecl& ast);
        [[nodiscard]] Symbol* createParamSymbol(AstFuncParamDecl& ast);

        SemanticAnalyzer& m_sem;
        TypePass& m_typePass;
    };

} // namespace Sem
} // namespace lbc
