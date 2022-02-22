//
// Created by Albert Varaksin on 08/07/2020.
//
#pragma once
#include "Ast/AstVisitor.h"
#include "Ast/ControlFlowStack.hpp"
#include "Ast/ValueFlags.hpp"
#include "Passes/ConstantFoldingPass.hpp"
#include "Passes/TypePass.hpp"

namespace lbc {
class Token;
class Symbol;
class SymbolTable;
class TypeRoot;
class Context;

class SemanticAnalyzer final : public AstVisitor<SemanticAnalyzer> {
public:
    explicit SemanticAnalyzer(Context& context);

    /// declareMembers the expression, optionally coerce result to given type
    void expression(AstExpr*& ast, const TypeRoot* type = nullptr);

    /// Checks types and if they are convertible, create CAST expression
    void coerce(AstExpr*& expr, const TypeRoot* type);

    /// Cast expression and fold the value
    void convert(AstExpr*& ast, const TypeRoot* type);

    /// Creates a CAST expression, without folding
    void cast(AstExpr*& ast, const TypeRoot* type);

    [[nodiscard]] Symbol* createNewSymbol(AstDecl& ast);
    [[nodiscard]] Context& getContext() noexcept { return m_context; }
    [[nodiscard]] SymbolTable* getSymbolTable() { return m_table; }
    [[nodiscard]] auto& getControlStack() { return m_controlStack; }
    [[nodiscard]] Sem::TypePass& getTypePass() { return m_typePass; }
    [[nodiscard]] Sem::ConstantFoldingPass& getConstantFoldingPass() { return m_constantFolder; }

    template<typename T>
    inline void with(SymbolTable* table, T handler) {
        RESTORE_ON_EXIT(m_table);
        m_table = table;
        handler();
    }

    AST_VISITOR_DECLARE_CONTENT_FUNCS()
private:
    void arithmetic(AstBinaryExpr& ast);
    void logical(AstBinaryExpr& ast);
    void comparison(AstBinaryExpr& ast);
    [[nodiscard]] bool canPerformBinary(TokenKind op, const TypeRoot* left, const TypeRoot* right) const noexcept;

    Context& m_context;
    AstModule* m_astRootModule = nullptr;
    AstFuncDecl* m_function = nullptr;
    SymbolTable* m_table = nullptr;
    Sem::ConstantFoldingPass m_constantFolder;
    Sem::TypePass m_typePass;

    ControlFlowStack<> m_controlStack;
};

} // namespace lbc
