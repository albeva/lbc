//
// Created by Albert Varaksin on 28/05/2021.
//
#include "Helpers.hpp"
#include "Lexer/Token.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Gen;

auto lbc::Gen::getCmpPred(const TypeRoot* type, TokenKind op) -> llvm::CmpInst::Predicate {
    type = type->removeReference();

    if (const auto* integral = llvm::dyn_cast<TypeIntegral>(type)) {
        const bool isSigned = integral->isSigned();
        switch (op) {
        case TokenKind::Equal:
            return llvm::CmpInst::Predicate::ICMP_EQ;
        case TokenKind::NotEqual:
            return llvm::CmpInst::Predicate::ICMP_NE;
        case TokenKind::LessThan:
            return isSigned ? llvm::CmpInst::Predicate::ICMP_SLT : llvm::CmpInst::Predicate::ICMP_ULT;
        case TokenKind::LessOrEqual:
            return isSigned ? llvm::CmpInst::Predicate::ICMP_SLE : llvm::CmpInst::Predicate::ICMP_ULE;
        case TokenKind::GreaterOrEqual:
            return isSigned ? llvm::CmpInst::Predicate::ICMP_SGE : llvm::CmpInst::Predicate::ICMP_UGE;
        case TokenKind::GreaterThan:
            return isSigned ? llvm::CmpInst::Predicate::ICMP_SGT : llvm::CmpInst::Predicate::ICMP_UGT;
        default:
            llvm_unreachable("Unkown comparison op");
        }
    }

    if (type->isFloatingPoint()) {
        switch (op) {
        case TokenKind::Equal:
            return llvm::CmpInst::Predicate::FCMP_OEQ;
        case TokenKind::NotEqual:
            return llvm::CmpInst::Predicate::FCMP_UNE;
        case TokenKind::LessThan:
            return llvm::CmpInst::Predicate::FCMP_OLT;
        case TokenKind::LessOrEqual:
            return llvm::CmpInst::Predicate::FCMP_OLE;
        case TokenKind::GreaterOrEqual:
            return llvm::CmpInst::Predicate::FCMP_OGE;
        case TokenKind::GreaterThan:
            return llvm::CmpInst::Predicate::FCMP_OGT;
        default:
            llvm_unreachable("Unkown comparison op");
        }
    }

    if (type->isBoolean() || type->isPointer()) {
        switch (op) {
        case TokenKind::Equal:
            return llvm::CmpInst::Predicate::ICMP_EQ;
        case TokenKind::NotEqual:
            return llvm::CmpInst::Predicate::ICMP_NE;
        default:
            llvm_unreachable("Unkown comparison op");
        }
    }

    llvm_unreachable("Unsupported type");
}

auto lbc::Gen::getBinOpPred(const TypeRoot* type, TokenKind op) -> llvm::Instruction::BinaryOps {
    type = type->removeReference();

    if (type->isIntegral()) {
        const auto sign = type->isSignedIntegral();
        switch (op) {
        case TokenKind::Multiply:
            return llvm::Instruction::Mul;
        case TokenKind::Divide:
            return sign ? llvm::Instruction::SDiv : llvm::Instruction::UDiv;
        case TokenKind::Modulus:
            return sign ? llvm::Instruction::SRem : llvm::Instruction::URem;
        case TokenKind::Plus:
            return llvm::Instruction::Add;
        case TokenKind::Minus:
            return llvm::Instruction::Sub;
        default:
            llvm_unreachable("Unknown binary op");
        }
    }

    if (type->isFloatingPoint()) {
        switch (op) {
        case TokenKind::Multiply:
            return llvm::Instruction::FMul;
        case TokenKind::Divide:
            return llvm::Instruction::FDiv;
        case TokenKind::Modulus:
            return llvm::Instruction::FRem;
        case TokenKind::Plus:
            return llvm::Instruction::FAdd;
        case TokenKind::Minus:
            return llvm::Instruction::FSub;
        default:
            llvm_unreachable("Unknown binary op");
        }
    }

    llvm_unreachable("Unsupported binary op type");
}
