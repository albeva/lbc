//
//  IrBuilder.cpp
//  LightBASIC
//
//  Created by Albert Varaksin on 03/03/2012.
//  Copyright (c) 2012 LightBASIC development team. All rights reserved.
//
#include "IrBuilder.h"
#include "Token.h"
#include "Ast.h"
#include "Type.h"
#include "Symbol.h"
#include "SymbolTable.h"
#include "stdint.h"

// llvm
#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/IRBuilder.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Bitcode/ReaderWriter.h>


using namespace lbc;

//
// create
IrBuilder::IrBuilder()
:   m_module(nullptr),
    m_table(nullptr),
    m_function(nullptr),
    m_block(nullptr),
    m_value(nullptr)
{
}


//
// AstDeclList
void IrBuilder::visit(AstProgram * ast)
{
    // reset the sate
	m_module = nullptr;
	m_function = nullptr;
	m_block = nullptr;
	m_value = nullptr;
	m_table = nullptr;
    
    // the module
    m_module = new llvm::Module(ast->name, llvm::getGlobalContext());
    
    // symbol table
    m_table = ast->symbolTable.get();
    
    // process declarations
    for (auto & decl : ast->decls) decl->accept(this);
    
    // verify module integrity
    m_module->dump();
    if (llvm::verifyModule(*m_module, llvm::PrintMessageAction)) {
        // there were errors
        delete m_module;
        m_module = nullptr;
    }
}



//
// generate llvm type from local type
llvm::Type * getType(lbc::Type * local, llvm::LLVMContext & context)
{
    if (local->llvmType) return local->llvmType;
    
    llvm::Type * llvmType = nullptr;
    // pointer
    if (local->isPointer()) {
        auto ptr = static_cast<PtrType *>(local);
        auto base = ptr->getBaseType();
        if (base->isPointer()) {
            THROW_EXCEPTION("IS pointer");
        }
        auto llType = llvm::Type::getIntNPtrTy(context, base->getSizeInBits());
        int level = ptr->indirection() - 1;
        while(level--) {
            llType = llType->getPointerTo();
        }
        llvmType = llType;
    }
    // basic type
    else if (local->isPrimitive()) {
        if (local->isIntegral()) {
            llvmType = llvm::Type::getIntNTy(context, local->getSizeInBits());
        } else if (local->isFloatingPoint()) {
            if (local->getSizeInBits() == 32)
                llvmType = llvm::Type::getFloatTy(context);
            else
                llvmType = llvm::Type::getDoubleTy(context);
        }
    }
    // function type
    else if (local->isFunction()) {
        std::vector<llvm::Type*> params;
        auto fnType = static_cast<FunctionType *>(local);
        for (auto p : fnType->params) {
            params.push_back(getType(p, context));
        }
        llvmType = llvm::FunctionType::get(
            getType(fnType->result(), context),
            params,
            fnType->vararg
        );
    }
    // nothing
    local->llvmType = llvmType;
    return llvmType;
}


//
// AstFuncSignature
void IrBuilder::visit(AstFuncSignature * ast)
{
    // the id
    const string & id = ast->id->token->lexeme();
    Symbol * sym = m_table->get(id);
    
    // get the function
    if (!sym->value) {
        assert(!m_module->getFunction(id));
        // get the symbol
        auto sym = m_table->get(id);
        auto llvmType = getType(sym->type(), m_module->getContext());
        string alias = sym->alias();
        if (id == "MAIN") alias = "main";
        // create llvm function
        m_function = llvm::Function::Create(
             llvm::cast<llvm::FunctionType>(llvmType),
             llvm::GlobalValue::ExternalLinkage,
             alias,
             m_module
        );
        m_function->setCallingConv(llvm::CallingConv::C);
        sym->value = m_function;
    }
    
    // bind function params
    if (ast->params) {
        auto llp = m_function->arg_begin();
        for (auto & p : ast->params->params) {
            llp->setName(p->id->token->lexeme());
            if (p->symbol) {
                p->symbol->value = llp;
            }
            llp++;
        }
    }
}


//
// AstFunctionDecl
void IrBuilder::visit(AstFunctionDecl * ast)
{    
    // process the signature
    ast->signature->accept(this);
}


//
// AstFunctionStmt
void IrBuilder::visit(AstFunctionStmt * ast)
{
    // function signature
    ast->signature->accept(this);
    
    // symbol table
    m_table = ast->stmts->symbolTable;
        
    // create the block
    auto tmp = m_block;
    m_block = llvm::BasicBlock::Create(m_module->getContext(), "", m_function, 0);
    
    // store function pointers locally
    if (ast->signature->params) {
        for (auto & p : ast->signature->params->params) {
            auto sym = p->symbol;
            auto value = sym->value;
            sym->value = new llvm::AllocaInst(sym->type()->llvmType, "", m_block);
            new llvm::StoreInst(value, sym->value, m_block);
        }
    }
    
    ast->stmts->accept(this);
    m_block = tmp;
    
    // restore
    m_table = m_table->parent();
}


//
// AstReturnStmt
void IrBuilder::visit(AstReturnStmt * ast)
{
    m_value = nullptr;
    if (ast->expr) ast->expr->accept(this);
    llvm::ReturnInst::Create(m_module->getContext(), m_value, m_block);
}


//
// AstLiteralExpr
void IrBuilder::visit(AstLiteralExpr * ast)
{
    const string & lexeme = ast->token->lexeme();
    auto & context = m_module->getContext();
    const auto & token = ast->token;
    
    // string literal
    if (token->type() == TokenType::StringLiteral) {
        auto arrType = llvm::ArrayType::get(llvm::IntegerType::get(context, 8), lexeme.length() + 1);
        auto global = new llvm::GlobalVariable(
            *m_module,
            arrType,
            true,
            llvm::GlobalValue::PrivateLinkage,
            nullptr,
            ".str"
        );
        global->setAlignment(1);
        llvm::Constant * const_value = llvm::ConstantArray::get(context, lexeme, true);
        global->setInitializer(const_value);
        
        std::vector<llvm::Constant*> indeces;
        llvm::ConstantInt * const_int64 = llvm::ConstantInt::get(context, llvm::APInt(32, 0, false));
        indeces.push_back(const_int64);
        indeces.push_back(const_int64);
        m_value = llvm::ConstantExpr::getGetElementPtr(global, indeces);
        return;
    }
    
    auto local = ast->type;
    auto type = getType(local, context);
    if (local->isBoolean() || token->type() == TokenType::True || token->type() == TokenType::False) {
        if (token->type() == TokenType::True) {
            m_value = llvm::ConstantInt::get(type, 1);
        } else if (token->type() == TokenType::False) {
            m_value = llvm::ConstantInt::get(type, 0);
        } else {
            if (std::stod(lexeme) == 0.0) {
                m_value = llvm::ConstantInt::get(type, 0);
            } else {
                m_value = llvm::ConstantInt::get(type, 1);
            }
        }
    } else if (local->isIntegral()) {
        m_value = llvm::ConstantInt::get(llvm::cast<llvm::IntegerType>(type), lexeme, 10);
    } else if (ast->type->isFloatingPoint()) {
        m_value = llvm::ConstantFP::get(type, lexeme);
    } else if (local->isPointer()) {
        if (local->IsAnyPtr() || lexeme == "0") {
            m_value = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
        } else {
            auto llvmType = llvm::cast<llvm::IntegerType>(getType(PrimitiveType::get(TokenType::LongInt), context));
            auto constant = llvm::ConstantInt::get(llvmType, lexeme, 10);
            m_value = new llvm::IntToPtrInst(constant, type, "", m_block);
        }
    }
}


//
// AstVarDecl
void IrBuilder::visit(AstVarDecl * ast)
{
    const string & id = ast->id->token->lexeme();
    auto sym = m_table->get(id);
    auto llType = getType(sym->type(), m_module->getContext());
    // if m_block is null then is a global variable
    if (m_block == nullptr) {
        llvm::Constant * constant;
        if (llType->isPointerTy()) {
            constant = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(llType));
        } else if (llType->isIntegerTy()) {
            constant = llvm::ConstantInt::get(llType, 0, false);
        } else if (llType->isFloatingPointTy()) {
            constant = llvm::ConstantFP::get(llType, 0);
        }
        sym->value = new llvm::GlobalVariable(
            *m_module,
            llType,
            false,
            llvm::GlobalValue::ExternalLinkage,
            constant,
            id
        );
    } else {
        sym->value = new llvm::AllocaInst(llType, id, m_block);
    }
}


//
// AstAssignStmt
void IrBuilder::visit(AstAssignStmt * ast)
{
    // left value
    llvm::Value * dst = nullptr;
    
    // dereference ?
    if (ast->left->is(Ast::DereferenceExpr)) {
        static_cast<AstDereferenceExpr *>(ast->left.get())->expr->accept(this);
        dst = m_value;
    } else {
        dst = m_table->get(static_cast<AstIdentExpr *>(ast->left.get())->token->lexeme())->value;
    }
    
    // right hand expression
    ast->right->accept(this);
    
    // store nstr
    new llvm::StoreInst(m_value, dst, m_block);
}


//
// AstAddressOfExpr
void IrBuilder::visit(AstAddressOfExpr * ast)
{
    const string & id = ast->id->token->lexeme();
    m_value = m_table->get(id)->value;
    ast->type->llvmType = getType(ast->type, m_module->getContext());
}


//
// AstDereferenceExpr
void IrBuilder::visit(AstDereferenceExpr * ast)
{
    ast->expr->accept(this);
    m_value = new llvm::LoadInst(m_value, "", m_block);
    ast->type->llvmType = getType(ast->type, m_module->getContext());
}


//
// AstCastExpr
void IrBuilder::visit(AstCastExpr * ast)
{
    ast->expr->accept(this);
    auto src = ast->expr->type;
    auto dst = ast->type;
    bool srcSigned = src->isSignedIntegral();
    dst->llvmType = getType(dst, m_module->getContext());
    
    // if target is boolean
    // then comparison is required
    if (dst->isBoolean()) {
        if (src->isIntegral()) {
            auto const_int = llvm::ConstantInt::get(src->llvmType, 0);
            m_value = new llvm::ICmpInst(*m_block, llvm::ICmpInst::ICMP_NE, m_value, const_int, "");
            srcSigned = false;
        } else if (src->isFloatingPoint()) {
            auto const_fp = llvm::ConstantFP::get(src->llvmType, 0);
            m_value = new llvm::FCmpInst(*m_block, llvm::ICmpInst::FCMP_UNE, m_value, const_fp, "");
            srcSigned = false;
        } else if (src->isPointer()) {
            std::cout << src->toString() << '\n';
            auto const_null = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(src->llvmType));
            m_value = new llvm::ICmpInst(*m_block, llvm::ICmpInst::ICMP_NE, m_value, const_null, "");
            srcSigned = false;
        }
    }
    
    // get cast opcode
    auto opcode = llvm::CastInst::getCastOpcode(
        m_value,
        srcSigned,
        dst->llvmType,
        dst->isSignedIntegral()
    );
    
    // create cast instruction
    m_value = llvm::CastInst::Create(opcode, m_value, dst->llvmType, "", m_block);
}


//
// AstCallExpr
void IrBuilder::visit(AstCallExpr * ast)
{
    const string & id = ast->id->token->lexeme();
    auto sym = m_table->get(id);
    
    vector<llvm::Value *> args;
    if (ast->args) {
        for (auto & arg : ast->args->args) {
            arg->accept(this);
            args.push_back(m_value);
        }
    }
    
    m_value = llvm::CallInst::Create(sym->value, args, id, m_block);
}


//
// AstIdentExpr
void IrBuilder::visit(AstIdentExpr * ast)
{
    const string & id = ast->token->lexeme();
    m_value = new llvm::LoadInst(m_table->get(id)->value, "", m_block);
}


//
// AstCallStmt
void IrBuilder::visit(AstCallStmt * ast)
{
    ast->expr->accept(this);
}