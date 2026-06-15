//
// Created by Albert Varaksin on 15/06/2026.
//
#include "Generator.hpp"
#include "IR/lib/Function.hpp"
#include "IR/lib/Instructions.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace lbc::gen;
using namespace lbc::ir::lib;

void Generator::lowerInstruction(const Instruction& instr) {
    switch (instr.getKind()) {
    case IrKind::Var: {
        const auto& i = llvm::cast<VarInstr>(instr);
        i.getResult()->setLlvm(m_builder.CreateAlloca(lowerType(i.getType()), nullptr, i.getResult()->getName()));
        break;
    }
    case IrKind::Store: {
        const auto& i = llvm::cast<StoreInstr>(instr);
        m_builder.CreateStore(value(i.getSrc()), address(i.getDest()));
        break;
    }
    case IrKind::Load: {
        const auto& i = llvm::cast<LoadInstr>(instr);
        i.getResult()->setLlvm(m_builder.CreateLoad(lowerType(i.getResult()->getType()), value(i.getSource())));
        break;
    }
    case IrKind::AddrOf: {
        // Taking the address of a named value yields its storage pointer.
        const auto& i = llvm::cast<AddrOfInstr>(instr);
        i.getResult()->setLlvm(address(llvm::cast<NamedValue>(i.getOperand())));
        break;
    }
    case IrKind::Cast: {
        const auto& i = llvm::cast<CastInstr>(instr);
        i.getResult()->setLlvm(lowerCast(value(i.getOperand()), i.getOperand()->getType(), i.getTargetType()));
        break;
    }
    case IrKind::Call: {
        const auto& i = llvm::cast<CallInstr>(instr);
        auto* callee = function(*i.getCallee());
        llvm::SmallVector<llvm::Value*> args;
        args.reserve(i.getArgs().size());
        for (const auto* arg : i.getArgs()) {
            args.push_back(value(arg));
        }
        i.getResult()->setLlvm(m_builder.CreateCall(callee->getFunctionType(), callee, args));
        break;
    }
    case IrKind::Neg: {
        const auto& i = llvm::cast<NegInstr>(instr);
        auto* operand = value(i.getOperand());
        i.getResult()->setLlvm(i.getOperand()->getType()->isFloatingPoint() ? m_builder.CreateFNeg(operand) : m_builder.CreateNeg(operand));
        break;
    }
    case IrKind::Not: {
        const auto& i = llvm::cast<NotInstr>(instr);
        i.getResult()->setLlvm(m_builder.CreateNot(value(i.getOperand())));
        break;
    }
    case IrKind::Add:
    case IrKind::Sub:
    case IrKind::Mul:
    case IrKind::Div:
    case IrKind::Mod:
    case IrKind::And:
    case IrKind::Or:
    case IrKind::Eq:
    case IrKind::Ne:
    case IrKind::Lt:
    case IrKind::Le:
    case IrKind::Gt:
    case IrKind::Ge: {
        const auto& bin = llvm::cast<IrBinary>(instr);
        auto* lhs = value(bin.getLhs());
        auto* rhs = value(bin.getRhs());
        const bool fp = bin.getLhs()->getType()->isFloatingPoint();
        const bool sign = isSigned(bin.getLhs()->getType());
        llvm::Value* result = nullptr;
        switch (instr.getKind()) {
        case IrKind::Add:
            result = fp ? m_builder.CreateFAdd(lhs, rhs) : m_builder.CreateAdd(lhs, rhs);
            break;
        case IrKind::Sub:
            result = fp ? m_builder.CreateFSub(lhs, rhs) : m_builder.CreateSub(lhs, rhs);
            break;
        case IrKind::Mul:
            result = fp ? m_builder.CreateFMul(lhs, rhs) : m_builder.CreateMul(lhs, rhs);
            break;
        case IrKind::Div:
            result = fp ? m_builder.CreateFDiv(lhs, rhs) : (sign ? m_builder.CreateSDiv(lhs, rhs) : m_builder.CreateUDiv(lhs, rhs));
            break;
        case IrKind::Mod:
            result = fp ? m_builder.CreateFRem(lhs, rhs) : (sign ? m_builder.CreateSRem(lhs, rhs) : m_builder.CreateURem(lhs, rhs));
            break;
        case IrKind::And:
            result = m_builder.CreateAnd(lhs, rhs);
            break;
        case IrKind::Or:
            result = m_builder.CreateOr(lhs, rhs);
            break;
        case IrKind::Eq:
            result = fp ? m_builder.CreateFCmpOEQ(lhs, rhs) : m_builder.CreateICmpEQ(lhs, rhs);
            break;
        case IrKind::Ne:
            result = fp ? m_builder.CreateFCmpONE(lhs, rhs) : m_builder.CreateICmpNE(lhs, rhs);
            break;
        case IrKind::Lt:
            result = fp ? m_builder.CreateFCmpOLT(lhs, rhs) : (sign ? m_builder.CreateICmpSLT(lhs, rhs) : m_builder.CreateICmpULT(lhs, rhs));
            break;
        case IrKind::Le:
            result = fp ? m_builder.CreateFCmpOLE(lhs, rhs) : (sign ? m_builder.CreateICmpSLE(lhs, rhs) : m_builder.CreateICmpULE(lhs, rhs));
            break;
        case IrKind::Gt:
            result = fp ? m_builder.CreateFCmpOGT(lhs, rhs) : (sign ? m_builder.CreateICmpSGT(lhs, rhs) : m_builder.CreateICmpUGT(lhs, rhs));
            break;
        case IrKind::Ge:
            result = fp ? m_builder.CreateFCmpOGE(lhs, rhs) : (sign ? m_builder.CreateICmpSGE(lhs, rhs) : m_builder.CreateICmpUGE(lhs, rhs));
            break;
        default:
            std::unreachable();
        }
        bin.getResult()->setLlvm(result);
        break;
    }
    case IrKind::Jmp: {
        const auto& i = llvm::cast<JmpInstr>(instr);
        m_builder.CreateBr(llvm::cast<llvm::BasicBlock>(i.getDestination()->getLlvm()));
        break;
    }
    case IrKind::CondJmp: {
        const auto& i = llvm::cast<CondJmpInstr>(instr);
        m_builder.CreateCondBr(value(i.getCondition()), llvm::cast<llvm::BasicBlock>(i.getTrueBlock()->getLlvm()), llvm::cast<llvm::BasicBlock>(i.getFalseBlock()->getLlvm()));
        break;
    }
    case IrKind::Ret: {
        const auto& i = llvm::cast<RetInstr>(instr);
        if (auto* val = i.getValue()) {
            m_builder.CreateRet(value(val));
        } else {
            m_builder.CreateRetVoid();
        }
        break;
    }
    }
}
