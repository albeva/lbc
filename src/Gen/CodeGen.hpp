//
// Created by Albert Varaksin on 05/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstVisitor.hpp"
#include "Ast/ControlFlowStack.hpp"
#include "Type/Type.hpp"
#include "ValueHandler.hpp"

namespace lbc {
class Context;

class CodeGen final : public AstVisitor<CodeGen, void, void, Gen::ValueHandler> {
public:
    explicit CodeGen(Context& context);

    [[nodiscard]] auto getModule() -> std::unique_ptr<llvm::Module>;

    [[nodiscard]] auto validate() const -> bool;

    [[nodiscard]] auto getContext() const -> Context& { return m_context; }
    [[nodiscard]] auto getBuilder() -> llvm::IRBuilder<>& { return m_builder; }
    [[nodiscard]] auto getTrue() const -> llvm::ConstantInt* { return m_constantTrue; }
    [[nodiscard]] auto getFalse() const -> llvm::ConstantInt* { return m_constantFalse; }
    [[nodiscard]] auto getControlStack() -> auto& { return m_controlStack; }

    void addBlock();
    void terminateBlock(llvm::BasicBlock* dest);
    void switchBlock(llvm::BasicBlock* block);

    AST_VISITOR_DECLARE_CONTENT_FUNCS()
private:
    enum class Scope : std::uint8_t {
        Root,
        Function
    };

    auto getGlobalCtorBlock() -> llvm::BasicBlock*;

    void declareFuncs(AstStmtList& ast);
    void declareFunc(AstFuncDecl& ast);
    void declareGlobalVar(const AstVarDecl& ast);
    void declareLocalVar(AstVarDecl& ast);
    auto expr(AstExpr& ast) -> Gen::ValueHandler;
    auto getConstantValue(const TypeRoot* type, const TokenValue& value) -> Gen::ValueHandler;
    auto getStringConstant(llvm::StringRef str) -> llvm::Constant*;

    Context& m_context;
    llvm::LLVMContext& m_llvmContext;
    unsigned int m_fileId = ~0U;
    Scope m_scope = Scope::Root;
    std::unique_ptr<llvm::Module> m_module;
    llvm::Function* m_globalCtorFunc = nullptr;
    llvm::IRBuilder<> m_builder;
    llvm::StringMap<llvm::Constant*> m_stringLiterals;

    llvm::ConstantInt* m_constantTrue;
    llvm::ConstantInt* m_constantFalse;

    bool m_hasImplicitMain = false;
    bool m_declareAsGlobals = true;

    struct ControlEntry final {
        llvm::BasicBlock* continueBlock;
        llvm::BasicBlock* exitBlock;
    };
    ControlFlowStack<ControlEntry> m_controlStack;
};

} // namespace lbc
