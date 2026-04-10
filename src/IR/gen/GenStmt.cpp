//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IR/lib/BasicBlock.hpp"
#include "IR/lib/Function.hpp"
#include "IR/lib/Module.hpp"
#include "IrGenerator.hpp"
#include "Symbol/Symbol.hpp"
using namespace lbc::ir::gen;

auto IrGenerator::accept(const AstStmtList& ast) -> Result {
    for (auto* decl : ast.getDecls()) {
        TRY(visit(*decl));
    }
    for (auto* stmt : ast.getStmts()) {
        TRY(visit(*stmt));
    }
    return {};
}

auto IrGenerator::accept(const AstExprStmt& ast) -> Result {
    TRY(visit(*ast.getExpr()));
    return {};
}

auto IrGenerator::accept(const AstDeclareStmt& /*ast*/) -> Result {
    // No-op: forward declarations handled during StmtList processing
    return {};
}

auto IrGenerator::accept(const AstFuncStmt& ast) -> Result {
    const ValueRestorer restor { m_function, m_block, m_tempCounter, m_ifCounter };

    // Create function and add to module
    const auto* symbol = ast.getDecl()->getSymbol();

    // Set up function state
    m_function = llvm::cast<lib::Function>(symbol->getOperand());
    m_module->getFunctions().push_back(m_function);
    m_tempCounter = 0;
    m_ifCounter = 0;
    setBlock(createBlock("entry"));

    // Process parameters
    for (auto* param : ast.getDecl()->getParams()) {
        TRY(visit(*param));
    }

    // Process body
    TRY(accept(*ast.getStmtList()));

    // Ensure function is terminated
    terminate(nullptr);

    return {};
}

auto IrGenerator::accept(const AstReturnStmt& ast) -> Result {
    lib::Value* value = nullptr;
    if (auto* expr = ast.getExpr()) {
        TRY(visit(*expr));
        value = expr->getOperand();
    }
    emit(makeRet(value));
    return {};
}

auto IrGenerator::accept(const AstDimStmt& ast) -> Result {
    for (auto* decl : ast.getDecls()) {
        TRY(visit(*decl));
    }
    return {};
}

auto IrGenerator::accept(const AstAssignStmt& ast) -> Result {
    auto& dst = *ast.getAssignee();
    const auto& expr = *ast.getExpr();

    TRY(visit(dst));
    TRY(visit(*ast.getExpr()));

    emit(makeStore(llvm::cast<lib::NamedValue>(dst.getOperand()), expr.getOperand()));
    return {};
}

auto IrGenerator::accept(const AstIfStmt& ast) -> Result {
    auto counter = m_ifCounter++;
    auto elseCounter = 1;

    auto* thenBlock = createBlock(std::format("if.{}.then", counter));
    auto* endBlock = createBlock(std::format("if.{}.end", counter));

    auto* ifStmt = &ast;
    while (true) {
        auto* elseStmt = ifStmt->getElseStmt();

        // Determine the false target for this condition
        lib::BasicBlock* falseBlock = nullptr;
        if (elseStmt == nullptr) {
            falseBlock = endBlock;
        } else if (llvm::isa<AstIfStmt>(elseStmt)) {
            auto elseCount = elseCounter++;
            falseBlock = createBlock(std::format("if.{}.else.if.{}", counter, elseCount));
        } else {
            falseBlock = createBlock(std::format("if.{}.else", counter));
        }

        // Emit conditional jump with both targets
        auto* cond = ifStmt->getCondition();
        TRY(visit(*cond));
        emit(makeJmp(cond->getOperand(), thenBlock, falseBlock));

        // Emit THEN block
        m_block = thenBlock;
        TRY(visit(*ifStmt->getThenStmt()));
        terminate(endBlock);

        // Handle ELSE
        if (elseStmt != nullptr) {
            if (const auto* elseIf = llvm::dyn_cast<AstIfStmt>(elseStmt)) {
                ifStmt = elseIf;
                setBlock(falseBlock);
                thenBlock = createBlock(std::format("if.{}.else.if.{}.then", counter, elseCounter - 1));
                continue;
            }

            setBlock(falseBlock);
            TRY(visit(*elseStmt));
            terminate(endBlock);
        }
        break;
    }

    setBlock(endBlock);
    return {};
}
