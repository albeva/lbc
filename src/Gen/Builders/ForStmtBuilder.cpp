//
// Created by Albert Varaksin on 28/05/2021.
//
#include "ForStmtBuilder.hpp"
#include "Ast/Ast.hpp"
#include "Gen/CodeGen.hpp"
#include "Gen/Helpers.hpp"
#include "Gen/ValueHandler.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Gen;

ForStmtBuilder::ForStmtBuilder(CodeGen& codeGen, AstForStmt& ast)
: Builder { codeGen, ast }
, m_direction { ast.direction } {
    if (m_direction == AstForStmt::Direction::Skip) {
        return;
    }

    createBlocks();
    declareVars();
    checkDirection();
    configureStep();
    build();
}

void ForStmtBuilder::declareVars() {
    for (const auto& decl : m_ast.decls) {
        m_gen.visit(*decl);
    }
    m_gen.visit(*m_ast.iterator);

    m_type = m_ast.iterator->symbol->getType();
    m_llvmType = m_type->getLlvmType(m_gen.getContext());

    m_iterator = ValueHandler { &m_gen, m_ast.iterator->symbol };
    m_limit = ValueHandler::createTempOrConstant(m_gen, *m_ast.limit, "for.limit");
}

void ForStmtBuilder::checkDirection() {
    if (m_direction == AstForStmt::Direction::Unknown) {
        auto* limitValue = m_limit.load();
        auto* iterValue = m_iterator.load();
        m_isDecr = m_builder.CreateCmp(
            getCmpPred(m_type, TokenKind::LessThan),
            limitValue,
            iterValue,
            "for.isdecr"
        );
    }
}

void ForStmtBuilder::createBlocks() {
    m_condBlock = llvm::BasicBlock::Create(m_llvmContext, "for.cond");
    m_bodyBlock = llvm::BasicBlock::Create(m_llvmContext, "for.body");
    m_iterBlock = llvm::BasicBlock::Create(m_llvmContext, "for.iter");
    m_exitBlock = llvm::BasicBlock::Create(m_llvmContext, "for.end");
}

void ForStmtBuilder::configureStep() {
    // No step
    if (m_ast.step == nullptr) {
        llvm::Constant* stepVal = nullptr;
        if (const auto* integral = llvm::dyn_cast<TypeIntegral>(m_type)) {
            stepVal = llvm::ConstantInt::get(m_llvmType, 1, integral->isSigned());
        } else if (llvm::isa<TypeFloatingPoint>(m_type)) {
            stepVal = llvm::ConstantFP::get(m_llvmType, 1);
        } else {
            llvm_unreachable("Unknown type");
        }
        m_step = ValueHandler { &m_gen, m_type, stepVal };
        return;
    }

    // Literal value
    if (auto* literal = llvm::dyn_cast<AstLiteralExpr>(m_ast.step)) {
        const auto* stepTy = literal->type;
        llvm::Constant* stepVal = nullptr;
        if (const auto* integral = llvm::dyn_cast<TypeIntegral>(stepTy)) {
            auto stepLit = literal->getValue().getIntegral();
            if (integral->isSigned()) {
                auto sstepLit = static_cast<int64_t>(stepLit);
                if (sstepLit < 0) {
                    stepLit = static_cast<uint64_t>(-sstepLit);
                }
            }
            stepVal = llvm::ConstantInt::get(m_llvmType, stepLit, false);
        } else if (llvm::isa<TypeFloatingPoint>(stepTy)) {
            auto stepLit = literal->getValue().getFloatingPoint();
            if (stepLit < 0) {
                stepLit = -stepLit;
            }
            stepVal = llvm::ConstantFP::get(m_llvmType, stepLit);
        } else {
            llvm_unreachable("Unkown type");
        }
        m_step = ValueHandler { &m_gen, stepTy, stepVal };
        return;
    }

    // Unknown value
    m_step = ValueHandler::createTemp(m_gen, *m_ast.step, "for.step");
    auto* stepValue = m_step.load();

    auto* isStepNeg = m_builder.CreateCmp(
        getCmpPred(m_ast.step->type, TokenKind::LessThan),
        stepValue,
        llvm::Constant::getNullValue(m_llvmType),
        "for.isStepNeg"
    );

    auto* negateBlock = llvm::BasicBlock::Create(m_llvmContext, "for.step.negate");

    switch (m_direction) {
    case AstForStmt::Direction::Unknown: {
        auto* isDecrBlock = llvm::BasicBlock::Create(m_llvmContext, "for.step.decr");
        auto* isIncrBlock = llvm::BasicBlock::Create(m_llvmContext, "for.step.incr");
        m_builder.CreateCondBr(m_isDecr, isDecrBlock, isIncrBlock);

        m_gen.switchBlock(isDecrBlock);
        m_builder.CreateCondBr(isStepNeg, negateBlock, m_exitBlock);

        m_gen.switchBlock(isIncrBlock);
        m_builder.CreateCondBr(isStepNeg, m_exitBlock, m_condBlock);
        break;
    }
    case AstForStmt::Direction::Skip:
        break;
    case AstForStmt::Direction::Increment:
        m_builder.CreateCondBr(isStepNeg, m_exitBlock, m_condBlock);
        break;
    case AstForStmt::Direction::Decrement:
        m_builder.CreateCondBr(isStepNeg, negateBlock, m_exitBlock);
        break;
    }

    m_gen.switchBlock(negateBlock);
    stepValue = m_builder.CreateNeg(stepValue);
    m_step.store(stepValue);
    m_builder.CreateBr(m_condBlock);
}

void ForStmtBuilder::build() {
    llvm::BasicBlock* incrBlock = nullptr;
    llvm::BasicBlock* decrBlock = nullptr;

    // Condition
    m_gen.switchBlock(m_condBlock);
    switch (m_direction) {
    case AstForStmt::Direction::Unknown: {
        incrBlock = llvm::BasicBlock::Create(m_llvmContext, "for.cond.incr");
        decrBlock = llvm::BasicBlock::Create(m_llvmContext, "for.cond.decr");
        m_builder.CreateCondBr(m_isDecr, decrBlock, incrBlock);

        m_gen.switchBlock(incrBlock);
        makeCondition(true);

        m_gen.switchBlock(decrBlock);
        makeCondition(false);
        break;
    }
    case AstForStmt::Direction::Skip:
        break;
    case AstForStmt::Direction::Increment:
        makeCondition(true);
        break;
    case AstForStmt::Direction::Decrement:
        makeCondition(false);
        break;
    }

    // Body
    m_gen.switchBlock(m_bodyBlock);

    m_gen.getControlStack().with({ ControlFlowStatement::For, { .continueBlock = m_iterBlock, .exitBlock = m_exitBlock } }, [&] {
        m_gen.visit(*m_ast.stmt);
    });

    // Iteration
    m_gen.switchBlock(m_iterBlock);
    switch (m_direction) {
    case AstForStmt::Direction::Unknown: {
        auto* iterIncrBlock = llvm::BasicBlock::Create(m_llvmContext, "for.iter.incr");
        auto* iterDecrBlock = llvm::BasicBlock::Create(m_llvmContext, "for.iter.decr");
        m_builder.CreateCondBr(m_isDecr, iterDecrBlock, iterIncrBlock);

        m_gen.switchBlock(iterIncrBlock);
        makeIteration(true, incrBlock);

        m_gen.switchBlock(iterDecrBlock);
        makeIteration(false, decrBlock);
        break;
    }
    case AstForStmt::Direction::Skip:
        break;
    case AstForStmt::Direction::Increment:
        makeIteration(true, m_condBlock);
        break;
    case AstForStmt::Direction::Decrement:
        makeIteration(false, m_condBlock);
        break;
    }

    // End
    m_gen.switchBlock(m_exitBlock);
}

void ForStmtBuilder::makeCondition(bool incr) {
    auto lessOrEqualPred = getCmpPred(m_type, TokenKind::LessOrEqual);
    auto* iterValue = m_iterator.load();
    auto* limitValue = m_limit.load();
    auto* cmp = incr
                  ? m_builder.CreateCmp(lessOrEqualPred, iterValue, limitValue, "for.incrCond")
                  : m_builder.CreateCmp(lessOrEqualPred, limitValue, iterValue, "for.decrCond");

    m_builder.CreateCondBr(cmp, m_bodyBlock, m_exitBlock);
}

void ForStmtBuilder::makeIteration(bool incr, llvm::BasicBlock* branch) {
    auto* stepValue = m_step.load();
    auto* iterValue = m_iterator.load();
    auto* result = incr
                     ? m_builder.CreateAdd(iterValue, stepValue)
                     : m_builder.CreateSub(iterValue, stepValue);
    m_iterator.store(result);
    m_builder.CreateBr(branch);
}
