//
// Created by Albert Varaksin on 05/07/2020.
//
#include "CodeGen.hpp"
#include "Builders/BinaryExprBuilder.hpp"
#include "Builders/DoLoopBuilder.hpp"
#include "Builders/ForStmtBuilder.hpp"
#include "Builders/IfStmtBuilder.hpp"
#include "Driver/Context.hpp"
#include "Helpers.hpp"
#include "Type/Type.hpp"
#include "ValueHandler.hpp"
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
using namespace lbc;
using namespace Gen;

CodeGen::CodeGen(Context& context)
: m_context{ context },
  m_llvmContext{ context.getLlvmContext() },
  m_builder{ m_llvmContext },
  m_constantTrue{ m_builder.getTrue() },
  m_constantFalse{ m_builder.getFalse() } {
}

bool CodeGen::validate() const noexcept {
    assert(m_module != nullptr); // NOLINT
    return !llvm::verifyModule(*m_module, &llvm::outs());
}

void CodeGen::addBlock() {
    auto* func = m_builder.GetInsertBlock()->getParent();
    auto* block = llvm::BasicBlock::Create(m_llvmContext, "", func);
    m_builder.SetInsertPoint(block);
}

void CodeGen::terminateBlock(llvm::BasicBlock* dest) {
    if (m_builder.GetInsertBlock()->getTerminator() == nullptr) {
        m_builder.CreateBr(dest);
    }
}

void CodeGen::switchBlock(llvm::BasicBlock* block) {
    terminateBlock(block);
    if (block->getParent() != nullptr) {
        block->moveAfter(m_builder.GetInsertBlock());
    } else {
        block->insertInto(m_builder.GetInsertBlock()->getParent());
    }
    m_builder.SetInsertPoint(block);
}

void CodeGen::visit(AstModule& ast) {
    m_astRootModule = &ast;
    m_fileId = ast.fileId;
    m_scope = Scope::Root;
    auto file = m_context.getSourceMrg().getMemoryBuffer(m_fileId)->getBufferIdentifier();

    m_module = make_unique<llvm::Module>(file, m_llvmContext);
    m_module->setTargetTriple(m_context.getTriple().str());

    declareFuncs(*ast.stmtList);

    if (m_context.getTriple().isOSWindows()) {
        auto* chkstk = llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(m_llvmContext), false),
            llvm::Function::InternalLinkage,
            "__chkstk",
            *m_module);
        chkstk->setCallingConv(llvm::CallingConv::C);
        chkstk->setDSOLocal(true);
        chkstk->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Local);
        auto* block = llvm::BasicBlock::Create(m_llvmContext, "entry", chkstk);
        m_builder.SetInsertPoint(block);
        m_builder.CreateRetVoid();
    }

    bool hasMainDefined = false;
    if (auto* main = ast.symbolTable->find("MAIN")) {
        if (main->alias() == "main") {
            hasMainDefined = true;
        }
    }
    bool generateMain = !hasMainDefined && ast.hasImplicitMain;

    llvm::Function* mainFn = nullptr;
    if (generateMain) {
        mainFn = llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getInt32Ty(m_llvmContext), false),
            llvm::Function::ExternalLinkage,
            "main",
            *m_module);
        mainFn->setCallingConv(llvm::CallingConv::C);
        mainFn->setDSOLocal(true);
        mainFn->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Local);
        auto* block = llvm::BasicBlock::Create(m_llvmContext, "entry", mainFn);
        m_builder.SetInsertPoint(block);
    } else {
        auto* block = llvm::BasicBlock::Create(m_llvmContext, "entry");
        m_builder.SetInsertPoint(block);
    }

    // parse statements
    visit(*ast.stmtList);

    // close main
    if (mainFn != nullptr) {
        auto* retValue = llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(m_llvmContext));
        auto& lastBlock = mainFn->getBasicBlockList().back();
        llvm::ReturnInst::Create(m_llvmContext, retValue, &lastBlock);
    }

    if (m_globalCtorFunc != nullptr) {
        auto* block = getGlobalCtorBlock();
        if (block->getTerminator() == nullptr) {
            llvm::ReturnInst::Create(m_llvmContext, nullptr, block);
        }
    }
}

llvm::BasicBlock* CodeGen::getGlobalCtorBlock() {
    if (m_globalCtorFunc == nullptr) {
        m_globalCtorFunc = llvm::Function::Create(
            llvm::FunctionType::get(llvm::Type::getVoidTy(m_llvmContext), false),
            llvm::Function::InternalLinkage,
            "__lbc_global_var_init",
            *m_module);
        if (m_context.getTriple().isOSBinFormatMachO()) {
            m_globalCtorFunc->setSection("__TEXT,__StaticInit,regular,pure_instructions");
        } else if (m_context.getTriple().isOSBinFormatELF()) {
            m_globalCtorFunc->setSection(".text.startup");
        }
        llvm::appendToGlobalCtors(*m_module, m_globalCtorFunc, 0, nullptr);
        llvm::BasicBlock::Create(m_llvmContext, "entry", m_globalCtorFunc);
    }
    return &m_globalCtorFunc->getBasicBlockList().back();
}

void CodeGen::visit(AstStmtList& ast) {
    for (const auto& stmt : ast.stmts) {
        visit(*stmt);
    }
}

void CodeGen::visit(AstImport& ast) {
    if (ast.module == nullptr) {
        return;
    }
    RESTORE_ON_EXIT(m_fileId);
    m_fileId = ast.module->fileId;
    visit(*ast.module->stmtList);
}

void CodeGen::visit(AstExprList& /*ast*/) {
    llvm_unreachable("Unhandled AstExprList&");
}

ValueHandler CodeGen::visit(AstAssignExpr& ast) {
    auto ptr = visit(*ast.lhs);
    auto value = visit(*ast.rhs);
    ptr.store(value);
    // TODO should ptr be loaded or result of store?
    return ptr;
}

void CodeGen::visit(AstExprStmt& ast) {
    visit(*ast.expr);
}

// Variables

void CodeGen::visit(AstVarDecl& ast) {
    if (m_declareAsGlobals) {
        declareGlobalVar(ast);
    } else {
        declareLocalVar(ast);
    }
}

void CodeGen::declareGlobalVar(AstVarDecl& ast) {
    auto* sym = ast.symbol;
    llvm::Constant* constant = nullptr;
    llvm::Type* exprType = sym->type()->getLlvmType(m_context);
    bool generateStoreInCtror = false;

    // has an init expr?
    if (ast.expr != nullptr) {
        if (auto* litExpr = dyn_cast<AstLiteralExpr>(ast.expr)) {
            auto rvalue = visit(*litExpr);
            constant = llvm::cast<llvm::Constant>(rvalue.load());
        } else {
            generateStoreInCtror = true;
        }
    }

    if (constant == nullptr) {
        constant = llvm::Constant::getNullValue(exprType);
    }
    llvm::Value* lvalue = new llvm::GlobalVariable(
        *m_module,
        exprType,
        false,
        sym->getLlvmLinkage(),
        constant,
        ast.symbol->identifier());

    if (generateStoreInCtror) {
        auto* block = m_builder.GetInsertBlock();
        m_builder.SetInsertPoint(getGlobalCtorBlock());
        auto rvalue = visit(*ast.expr);
        m_builder.CreateStore(rvalue.load(), lvalue);
        m_builder.SetInsertPoint(block);
    }

    sym->setLlvmValue(lvalue);
}

void CodeGen::declareLocalVar(AstVarDecl& ast) {
    llvm::Type* exprType = ast.symbol->type()->getLlvmType(m_context);
    auto* lvalue = m_builder.CreateAlloca(exprType, nullptr, ast.symbol->identifier());

    // has an init expr?
    if (ast.expr != nullptr) {
        auto rvalue = visit(*ast.expr);
        m_builder.CreateStore(rvalue.load(), lvalue);
    }

    ast.symbol->setLlvmValue(lvalue);
}

// Functions

void CodeGen::visit(AstFuncDecl& /*ast*/) {
    // NOOP
}

void CodeGen::declareFuncs(AstStmtList& ast) {
    for (const auto& stmt : ast.stmts) {
        switch (stmt->kind) {
        case AstKind::FuncDecl:
            declareFunc(static_cast<AstFuncDecl&>(*stmt));
            break;
        case AstKind::FuncStmt:
            declareFunc(*static_cast<AstFuncStmt&>(*stmt).decl);
            break;
        case AstKind::Import: {
            auto& import = static_cast<AstImport&>(*stmt);
            if (import.module != nullptr) {
                declareFuncs(*import.module->stmtList);
            }
            break;
        }
        default:
            break;
        }
    }
}

void CodeGen::declareFunc(AstFuncDecl& ast) {
    auto* sym = ast.symbol;
    auto* fnTy = llvm::cast<llvm::FunctionType>(ast.symbol->type()->getLlvmType(m_context));
    auto* fn = llvm::Function::Create(
        fnTy,
        sym->getLlvmLinkage(),
        ast.symbol->identifier(),
        *m_module);
    fn->setCallingConv(llvm::CallingConv::C);
    fn->setDSOLocal(true);
    ast.symbol->setLlvmValue(fn);

    if (ast.params != nullptr) {
        auto* iter = fn->arg_begin();
        for (const auto& param : ast.params->params) {
            iter->setName(param->symbol->identifier());
            param->symbol->setLlvmValue(iter);
            iter++; // NOLINT
        }
    }
}

void CodeGen::visit(AstFuncParamDecl& /*ast*/) {
    llvm_unreachable("visit");
}

void CodeGen::visit(AstFuncStmt& ast) {
    RESTORE_ON_EXIT(m_scope);
    m_scope = Scope::Function;

    RESTORE_ON_EXIT(m_declareAsGlobals);
    m_declareAsGlobals = false;

    auto* func = llvm::cast<llvm::Function>(ast.decl->symbol->getLlvmValue());

    auto* current = m_builder.GetInsertBlock();
    auto* block = llvm::BasicBlock::Create(m_llvmContext, "", func);
    m_builder.SetInsertPoint(block);

    if (ast.decl->params != nullptr) {
        for (const auto& param : ast.decl->params->params) {
            auto* sym = param->symbol;
            auto* value = sym->getLlvmValue();
            sym->setLlvmValue(m_builder.CreateAlloca(
                sym->type()->getLlvmType(m_context),
                nullptr,
                sym->identifier() + ".addr"));
            m_builder.CreateStore(value, sym->getLlvmValue());
        }
    }

    visit(*ast.stmtList);

    block = m_builder.GetInsertBlock();
    if (block->getTerminator() == nullptr) {
        auto* retType = func->getReturnType();
        if (!retType->isVoidTy()) {
            fatalError("No RETURN statement");
        }
        m_builder.CreateRetVoid();
    }

    m_builder.SetInsertPoint(current);
}

void CodeGen::visit(AstReturnStmt& ast) {
    if (ast.expr != nullptr) {
        auto value = visit(*ast.expr);
        m_builder.CreateRet(value.load());
    } else {
        auto* func = m_builder.GetInsertBlock()->getParent();
        auto* retTy = func->getReturnType();

        if (retTy->isVoidTy()) {
            m_builder.CreateRetVoid();
        } else {
            auto* constant = llvm::Constant::getNullValue(retTy);
            m_builder.CreateRet(constant);
        }
    }
}

//------------------------------------------------------------------
// Type (user defined)
//------------------------------------------------------------------

void CodeGen::visit(AstUdtDecl& /*ast*/) {
    // NOOP
}

//------------------------------------------------------------------
// Type alias
//------------------------------------------------------------------

void CodeGen::visit(AstTypeAlias& /* ast */) {
    // NO OP
}

//------------------------------------------------------------------
// IF stmt
//------------------------------------------------------------------

void CodeGen::visit(AstIfStmt& ast) {
    RESTORE_ON_EXIT(m_declareAsGlobals);
    m_declareAsGlobals = false;
    Gen::IfStmtBuilder(*this, ast);
}

void CodeGen::visit(AstForStmt& ast) {
    RESTORE_ON_EXIT(m_declareAsGlobals);
    m_declareAsGlobals = false;
    Gen::ForStmtBuilder(*this, ast);
}

void CodeGen::visit(AstDoLoopStmt& ast) {
    RESTORE_ON_EXIT(m_declareAsGlobals);
    m_declareAsGlobals = false;
    Gen::DoLoopBuilder(*this, ast);
}

void CodeGen::visit(AstContinuationStmt& ast) {
    auto target = m_controlStack.find(ast.destination);
    if (target == m_controlStack.cend()) {
        fatalError("control statement not found");
    }

    switch (ast.action) {
    case AstContinuationStmt::Action::Continue:
        m_builder.CreateBr(target->second.continueBlock);
        break;
    case AstContinuationStmt::Action::Exit:
        m_builder.CreateBr(target->second.exitBlock);
        break;
    }

    addBlock();
}

//------------------------------------------------------------------
// Attributes
//------------------------------------------------------------------

void CodeGen::visit(AstAttributeList& /*ast*/) {
    llvm_unreachable("visitAttributeList");
}

void CodeGen::visit(AstAttribute& /*ast*/) {
    llvm_unreachable("visitAttribute");
}

//------------------------------------------------------------------
// Type
//------------------------------------------------------------------

void CodeGen::visit(AstTypeExpr& /*ast*/) {
    // NOOP
}

//------------------------------------------------------------------
// Expressions
//------------------------------------------------------------------

ValueHandler CodeGen::visit(AstIdentExpr& ast) {
    return { this, ast };
}

ValueHandler CodeGen::visit(AstMemberAccess& ast) {
    return { this, ast };
}

ValueHandler CodeGen::visit(AstDereference& ast) {
    return { this, ast };
}

ValueHandler CodeGen::visit(AstAddressOf& ast) {
    return { this, ast };
}

ValueHandler CodeGen::visit(AstCallExpr& ast) {
    auto* callable = visit(*ast.callable).getAddress();

    llvm::FunctionType* fnType = nullptr;
    if (auto* func = llvm::dyn_cast<llvm::Function>(callable)) {
        fnType = func->getFunctionType();
    } else if (const auto* ptr = llvm::dyn_cast<llvm::PointerType>(callable->getType())) {
        fnType = llvm::cast<llvm::FunctionType>(ptr->getElementType());
    }

    const auto& args = ast.args->exprs;
    std::vector<llvm::Value*> values;
    values.reserve(args.size());
    for (const auto& arg : args) {
        auto* value = visit(*arg).load();
        values.emplace_back(value);
    }

    auto* call = m_builder.CreateCall(fnType, callable, values, "");
    call->setTailCall(false);
    return { this, ast.type, call };
}

ValueHandler CodeGen::visit(AstLiteralExpr& ast) {
    const auto visitor = Visitor{
        [&](std::monostate /*value*/) -> llvm::Value* {
            return llvm::ConstantPointerNull::get(
                llvm::cast<llvm::PointerType>(ast.type->getLlvmType(m_context)));
        },
        [&](StringRef str) -> llvm::Value* {
            return getStringConstant(str);
        },
        [&](uint64_t value) -> llvm::Value* {
            return llvm::ConstantInt::get(
                ast.type->getLlvmType(m_context),
                value,
                static_cast<const TypeIntegral*>(ast.type)->isSigned());
        },
        [&](double value) -> llvm::Value* {
            return llvm::ConstantFP::get(
                ast.type->getLlvmType(m_context),
                value);
        },
        [&](bool value) -> llvm::Value* {
            return value ? m_constantTrue : m_constantFalse;
        }
    };
    return { this, ast.type, std::visit(visitor, ast.value) };
}

llvm::Constant* CodeGen::getStringConstant(StringRef str) {
    auto [iter, inserted] = m_stringLiterals.try_emplace(str, nullptr);
    if (inserted) {
        iter->second = m_builder.CreateGlobalStringPtr(str);
    }
    return iter->second;
}

ValueHandler CodeGen::visit(AstUnaryExpr& ast) {
    switch (ast.tokenKind) {
    case TokenKind::Negate: {
        auto* value = visit(*ast.expr).load();

        if (value->getType()->isIntegerTy()) {
            return { this, ast.type, m_builder.CreateNeg(value) };
        }

        if (value->getType()->isFloatingPointTy()) {
            return { this, ast.type, m_builder.CreateFNeg(value) };
        }

        llvm_unreachable("Unexpected unary operator");
    }
    case TokenKind::LogicalNot: {
        auto value = visit(*ast.expr);
        return { this, ast.type, m_builder.CreateNot(value.load(), "lnot") };
    }
    default:
        llvm_unreachable("Unexpected unary operator");
    }
}

//------------------------------------------------------------------
// Binary Operation
//------------------------------------------------------------------

ValueHandler CodeGen::visit(AstBinaryExpr& ast) {
    return Gen::BinaryExprBuilder(*this, ast).build();
}

//------------------------------------------------------------------
// Casting
//------------------------------------------------------------------

ValueHandler CodeGen::visit(AstCastExpr& ast) {
    auto* value = visit(*ast.expr).load();

    bool srcIsSigned = ast.expr->type->isSignedIntegral();
    bool dstIsSigned = ast.type->isSignedIntegral();

    auto opcode = llvm::CastInst::getCastOpcode(
        value,
        srcIsSigned,
        ast.type->getLlvmType(m_context),
        dstIsSigned);
    auto* casted = m_builder.CreateCast(opcode, value, ast.type->getLlvmType(m_context));

    return { this, ast.type, casted };
}

ValueHandler CodeGen::visit(AstIfExpr& ast) {
    auto condValue = visit(*ast.expr);
    auto trueValue = visit(*ast.trueExpr);
    auto falseValue = visit(*ast.falseExpr);

    auto* value = m_builder.CreateSelect(condValue.load(), trueValue.load(), falseValue.load());
    return { this, ast.type, value };
}

unique_ptr<llvm::Module> CodeGen::getModule() {
    return std::move(m_module);
}
