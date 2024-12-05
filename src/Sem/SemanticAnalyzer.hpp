//
// Created by Albert Varaksin on 08/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstVisitor.hpp"
#include "Ast/ControlFlowStack.hpp"
#include "Ast/ValueFlags.hpp"
#include "ConstantFolder.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Passes/DeclPass.hpp"
#include "Passes/TypePass.hpp"

namespace lbc {
class Token;
class Symbol;
class SymbolTable;
class TypeRoot;
class Context;

class SemanticAnalyzer final : AstVisitor<SemanticAnalyzer, Result<void>>, public ErrorLogger {
    friend AstVisitor;

public:
    struct StateFlags final {
        bool allowUseBeforDefiniation : 1;
        bool allowRecursiveSymbolLookup : 1;
    };

    explicit SemanticAnalyzer(Context& context);

    /// declareMembers the expression, optionally coerce result to given type
    auto expression(AstExpr*& ast, const TypeRoot* type = nullptr) -> Result<void>;

    /// Checks types and if they are convertible, create CAST expression
    auto coerce(AstExpr*& ast, const TypeRoot* type) -> Result<void>;

    /// Cast expression and fold the value
    auto convert(AstExpr*& ast, const TypeRoot* type) -> Result<void>;

    /// Creates a CAST expression, without folding
    auto cast(AstExpr*& ast, const TypeRoot* type) -> Result<void>;

    [[nodiscard]] auto getContext() -> Context& { return m_context; }
    [[nodiscard]] auto getSymbolTable() -> SymbolTable* { return m_table; }
    [[nodiscard]] auto getTypePass() -> Sem::TypePass& { return m_typePass; }
    [[nodiscard]] auto getDeclPass() -> Sem::DeclPass& { return m_declPass; }
    [[nodiscard]] auto getExprEvaluator() -> ConstantFolder& { return m_constantFolder; }
    [[nodiscard]] auto hasImplicitMain() const -> bool { return m_module->hasImplicitMain; }

    static auto with(std::invocable auto&& handler) {
        return handler();
    }

    template <typename... Args, std::invocable = LastType<Args...>>
    auto with(SymbolTable* table, Args&&... rest) {
        RESTORE_ON_EXIT(m_table);
        m_table = table;
        return with(std::forward<Args>(rest)...);
    }

    template <typename... Args, std::invocable = LastType<Args...>>
    auto with(AstModule* module, Args&&... rest) {
        RESTORE_ON_EXIT(m_module);
        m_module = module;
        return with(std::forward<Args>(rest)...);
    }

    template <typename... Args, std::invocable = LastType<Args...>>
    auto with(AstFuncDecl* function, Args&&... rest) {
        RESTORE_ON_EXIT(m_function);
        m_function = function;
        return with(std::forward<Args>(rest)...);
    }

    template <typename... Args, std::invocable = LastType<Args...>>
    auto with(StateFlags flags, Args&&... rest) {
        RESTORE_ON_EXIT(m_flags);
        m_flags = flags;
        return with(std::forward<Args>(rest)...);
    }

    AST_VISITOR_DECLARE_CONTENT_FUNCS()
private:
    auto arithmetic(AstBinaryExpr& ast) -> Result<void>;
    auto logical(AstBinaryExpr& ast) -> Result<void>;
    auto comparison(AstBinaryExpr& ast) -> Result<void>;

    [[nodiscard]] static auto canPerformBinary(TokenKind op, const TypeRoot* left, const TypeRoot* right) -> bool;
    [[nodiscard]] auto isVariableAccessible(Symbol* symbol) const -> bool;

    Context& m_context;

    AstModule* m_module = nullptr;
    AstFuncDecl* m_function = nullptr;
    SymbolTable* m_table = nullptr;
    StateFlags m_flags {};

    Sem::TypePass m_typePass;
    Sem::DeclPass m_declPass;

    ConstantFolder m_constantFolder;
};

} // namespace lbc
