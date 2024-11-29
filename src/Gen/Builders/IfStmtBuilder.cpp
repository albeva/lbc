//
// Created by Albert on 28/05/2021.
//
#include "IfStmtBuilder.hpp"
#include "Driver/Context.hpp"
using namespace lbc;
using namespace Gen;

IfStmtBuilder::IfStmtBuilder(CodeGen& gen, AstIfStmt& ast)
: Builder { gen, ast } {
    build();
}

void IfStmtBuilder::build() {
    auto* func = m_builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* endBlock = nullptr;
    const auto makeEndBlock = [&] {
        if (endBlock == nullptr) {
            endBlock = llvm::BasicBlock::Create(m_llvmContext, "if.end", func);
        }
        return endBlock;
    };

    const auto count = m_ast.blocks.size();
    for (size_t idx = 0; idx < count; idx++) {
        const auto& block = m_ast.blocks[idx];
        llvm::BasicBlock* elseBlock = nullptr;

        for (const auto& decl : block->decls) {
            m_gen.visit(*decl);
        }

        if (block->expr != nullptr) {
            if (const auto value = block->expr->constantValue) {
                if (std::get<bool>(value.value())) {
                    m_gen.visit(*block->stmt);
                    m_gen.switchBlock(makeEndBlock());
                    return;
                }
                continue;
            }

            auto* condition = m_gen.visit(*block->expr).load();

            auto* thenBlock = llvm::BasicBlock::Create(m_llvmContext, "if.then", func);
            if (idx == count - 1) {
                elseBlock = makeEndBlock();
            } else {
                elseBlock = llvm::BasicBlock::Create(m_llvmContext, "if.else", func);
            }
            m_builder.CreateCondBr(condition, thenBlock, elseBlock);

            m_gen.switchBlock(thenBlock);
        } else {
            elseBlock = makeEndBlock();
        }

        m_gen.visit(*block->stmt);
        m_gen.terminateBlock(makeEndBlock());

        m_gen.switchBlock(elseBlock);
    }
}
