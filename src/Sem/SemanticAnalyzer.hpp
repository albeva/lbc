//
// Created by Albert Varaksin on 08/07/2020.
//
#pragma once
#include "Ast/AstVisitor.h"
#include "Ast/ControlFlowStack.hpp"
#include "Ast/ValueFlags.hpp"
#include "Passes/ConstantFoldingPass.hpp"
#include "Passes/DeclPass.hpp"
#include "Passes/TypePass.hpp"

namespace lbc {
class Token;
class Symbol;
class SymbolTable;
class TypeRoot;
class Context;

class SemanticAnalyzer final : public AstVisitor<SemanticAnalyzer> {
public:
    struct StateFlags final {
        bool allowUseBeforDefiniation : 1;
    };

    explicit SemanticAnalyzer(Context& context);

    /// declareMembers the expression, optionally coerce result to given type
    void expression(AstExpr*& ast, const TypeRoot* type = nullptr);

    /// Checks types and if they are convertible, create CAST expression
    void coerce(AstExpr*& expr, const TypeRoot* type);

    /// Cast expression and fold the value
    void convert(AstExpr*& ast, const TypeRoot* type);

    /// Creates a CAST expression, without folding
    void cast(AstExpr*& ast, const TypeRoot* type);

    [[nodiscard]] inline Context& getContext() noexcept { return m_context; }
    [[nodiscard]] inline SymbolTable* getSymbolTable() noexcept { return m_table; }
    [[nodiscard]] inline auto& getControlStack() noexcept { return m_controlStack; }
    [[nodiscard]] inline Sem::TypePass& getTypePass() noexcept { return m_typePass; }
    [[nodiscard]] inline Sem::ConstantFoldingPass& getConstantFoldingPass() noexcept { return m_constantFolder; }
    [[nodiscard]] inline Sem::DeclPass& getDeclPass() noexcept { return m_declPass; }
    [[nodiscard]] inline bool hasImplicitMain() const noexcept { return m_astRootModule->hasImplicitMain; }

    template<std::invocable Func>
    inline auto with(Func&& handler) -> std::invoke_result_t<Func> {
        return handler();
    }

    template<typename... Args>
    inline auto with(AstFuncDecl* function, Args... rest) -> decltype(with(rest...)) {
        RESTORE_ON_EXIT(m_function);
        m_function = function;
        return with(rest...);
    }

    template<typename... Args>
    inline auto with(SymbolTable* table, Args... rest) -> decltype(with(rest...)) {
        RESTORE_ON_EXIT(m_table);
        m_table = table;
        return with(rest...);
    }

    template<typename... Args>
    inline auto with(StateFlags flags, Args... rest) -> decltype(with(rest...)) {
        RESTORE_ON_EXIT(m_flags);
        m_flags = flags;
        return with(rest...);
    }

    AST_VISITOR_DECLARE_CONTENT_FUNCS()
private:
    void arithmetic(AstBinaryExpr& ast);
    void logical(AstBinaryExpr& ast);
    void comparison(AstBinaryExpr& ast);
    [[nodiscard]] bool canPerformBinary(TokenKind op, const TypeRoot* left, const TypeRoot* right) const noexcept;

    [[nodiscard]] bool isVariableAccessible(Symbol* symbol) const noexcept;

    Context& m_context;
    AstModule* m_astRootModule = nullptr;

    AstFuncDecl* m_function = nullptr;
    SymbolTable* m_table = nullptr;
    StateFlags m_flags{};

    Sem::ConstantFoldingPass m_constantFolder;
    Sem::TypePass m_typePass;
    Sem::DeclPass m_declPass;

    ControlFlowStack<> m_controlStack;
};

} // namespace lbc
