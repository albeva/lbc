//
// Created by Albert Varaksin on 08/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstVisitor.hpp"
#include "Ast/ControlFlowStack.hpp"
#include "Ast/ValueFlags.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Passes/ConstantFoldingPass.hpp"
#include "Passes/DeclPass.hpp"
#include "Passes/TypePass.hpp"

namespace lbc {
class Token;
class Symbol;
class SymbolTable;
class TypeRoot;
class Context;

class SemanticAnalyzer final : public AstVisitor<SemanticAnalyzer, Result<void>>
, public ErrorLogger {
public:
    struct StateFlags final {
        bool allowUseBeforDefiniation : 1;
        bool allowRecursiveSymbolLookup : 1;
    };

    explicit SemanticAnalyzer(Context& context);

    /// declareMembers the expression, optionally coerce result to given type
    Result<void> expression(AstExpr*& ast, const TypeRoot* type = nullptr);

    /// Checks types and if they are convertible, create CAST expression
    Result<void> coerce(AstExpr*& ast, const TypeRoot* type);

    /// Cast expression and fold the value
    Result<void> convert(AstExpr*& ast, const TypeRoot* type);

    /// Creates a CAST expression, without folding
    Result<void> cast(AstExpr*& ast, const TypeRoot* type);

    [[nodiscard]] inline Context& getContext() { return m_context; }
    [[nodiscard]] inline SymbolTable* getSymbolTable() { return m_table; }
    [[nodiscard]] inline Sem::TypePass& getTypePass() { return m_typePass; }
    [[nodiscard]] inline Sem::ConstantFoldingPass& getConstantFoldingPass() { return m_constantFolder; }
    [[nodiscard]] inline Sem::DeclPass& getDeclPass() { return m_declPass; }
    [[nodiscard]] inline bool hasImplicitMain() const { return m_module->hasImplicitMain; }

    template<std::invocable Func>
    inline auto with(Func&& handler) -> std::invoke_result_t<Func> {
        return handler();
    }

    template<typename... Args, std::invocable Func = LastType<Args...>>
    inline auto with(SymbolTable* table, Args&&... rest) -> std::invoke_result_t<Func> {
        RESTORE_ON_EXIT(m_table);
        m_table = table;
        return with(std::forward<Args>(rest)...);
    }

    template<typename... Args, std::invocable Func = LastType<Args...>>
    inline auto with(AstModule* module, Args&&... rest) -> std::invoke_result_t<Func> {
        RESTORE_ON_EXIT(m_module);
        m_module = module;
        return with(std::forward<Args>(rest)...);
    }

    template<typename... Args, std::invocable Func = LastType<Args...>>
    inline auto with(AstFuncDecl* function, Args&&... rest) -> std::invoke_result_t<Func> {
        RESTORE_ON_EXIT(m_function);
        m_function = function;
        return with(std::forward<Args>(rest)...);
    }

    template<typename... Args, std::invocable Func = LastType<Args...>>
    inline auto with(StateFlags flags, Args&&... rest) -> std::invoke_result_t<Func> {
        RESTORE_ON_EXIT(m_flags);
        m_flags = flags;
        return with(std::forward<Args>(rest)...);
    }

    AST_VISITOR_DECLARE_CONTENT_FUNCS()
private:
    Result<void> arithmetic(AstBinaryExpr& ast);
    Result<void> logical(AstBinaryExpr& ast);
    Result<void> comparison(AstBinaryExpr& ast);

    [[nodiscard]] bool canPerformBinary(TokenKind op, const TypeRoot* left, const TypeRoot* right) const;
    [[nodiscard]] bool isVariableAccessible(Symbol* symbol) const;

    Context& m_context;

    AstModule* m_module = nullptr;
    AstFuncDecl* m_function = nullptr;
    SymbolTable* m_table = nullptr;
    StateFlags m_flags{};

    Sem::ConstantFoldingPass m_constantFolder;
    Sem::TypePass m_typePass;
    Sem::DeclPass m_declPass;
};

} // namespace lbc
