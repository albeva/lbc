//
// Created by Albert on 29/05/2021.
//
#include "ForStmtPass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Sem;

void ForStmtPass::visit(AstForStmt& ast) const {
    auto* current = m_sem.getSymbolTable();
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(current);
    m_sem.with(ast.symbolTable, [&]() {
        declare(ast);
        analyze(ast);
        determineForDirection(ast);
    });
}

void ForStmtPass::declare(AstForStmt& ast) const {
    m_sem.getDeclPass().declareAndDefine(ast.decls);
    m_sem.getDeclPass().declareAndDefine(*ast.iterator);
    auto flags = ast.iterator->symbol->valueFlags();
    flags.assignable = false;
    ast.iterator->symbol->valueFlags() = flags;
}

void ForStmtPass::analyze(AstForStmt& ast) const {
    MUST(m_sem.expression(ast.limit));

    if (ast.step != nullptr) {
        MUST(m_sem.expression(ast.step));
    }

    const auto* type = ast.iterator->symbol->getType();
    if (!type->isNumeric()) {
        fatalError("NEXT iterator must be of numeric type");
    }

    // type TO type check
    switch (type->compare(ast.limit->type)) {
    case TypeComparison::Incompatible:
        fatalError("Incompatible types in FOR");
    case TypeComparison::Downcast:
        MUST(m_sem.convert(ast.limit, type));
        break;
    case TypeComparison::Equal:
        break;
    case TypeComparison::Upcast:
        if (ast.iterator->typeExpr != nullptr) {
            MUST(m_sem.convert(ast.limit, type));
        } else {
            MUST(m_sem.convert(ast.iterator->expr, ast.limit->type));
            ast.iterator->symbol->setType(ast.limit->type);
        }
        break;
    }

    // type STEP type check
    if (ast.step != nullptr) {
        switch (type->compare(ast.step->type)) {
        case TypeComparison::Incompatible:
            fatalError("Incompatible types in STEP");
        case TypeComparison::Downcast:
        case TypeComparison::Upcast: {
            const auto* dstTy = type;
            const auto* iterTy = llvm::dyn_cast<TypeIntegral>(type);
            if (iterTy != nullptr && !iterTy->isSigned()) {
                if (const auto* stepIntTy = llvm::dyn_cast<TypeIntegral>(ast.step->type)) {
                    if (stepIntTy->isSigned()) {
                        if (auto* literal = llvm::dyn_cast<AstLiteralExpr>(ast.step)) {
                            if (static_cast<int64_t>(std::get<uint64_t>(literal->value)) < 0) {
                                dstTy = iterTy->getSigned();
                            }
                        } else {
                            dstTy = iterTy->getSigned();
                        }
                    }
                } else if (llvm::isa<TypeFloatingPoint>(ast.step->type)) {
                    if (auto* literal = llvm::dyn_cast<AstLiteralExpr>(ast.step)) {
                        if (std::get<double>(literal->value) < 0.0) {
                            dstTy = iterTy->getSigned();
                        }
                    } else {
                        dstTy = iterTy->getSigned();
                    }
                }
            }
            MUST(m_sem.convert(ast.step, dstTy));
            break;
        }
        case TypeComparison::Equal:
            break;
        }
    }

    MUST(m_sem.visit(*ast.stmt));

    if (!ast.next.empty()) {
        if (ast.next != ast.iterator->name) {
            fatalError("NEXT iterator names must match");
        }
    }
}

void ForStmtPass::determineForDirection(AstForStmt& ast) const {
    auto* from = llvm::dyn_cast<AstLiteralExpr>(ast.iterator->expr);
    auto* to = llvm::dyn_cast<AstLiteralExpr>(ast.limit);
    const auto* type = ast.iterator->symbol->getType();
    bool equal = false;

    if (from != nullptr && to != nullptr) {
        if (const auto* integral = llvm::dyn_cast<TypeIntegral>(type)) {
            auto lhs = std::get<uint64_t>(from->value);
            auto rhs = std::get<uint64_t>(to->value);
            if (lhs == rhs) {
                ast.direction = AstForStmt::Direction::Increment;
                equal = true;
            } else if (integral->isSigned()) {
                auto slhs = static_cast<int64_t>(lhs);
                auto srhs = static_cast<int64_t>(rhs);
                if (slhs < srhs) {
                    ast.direction = AstForStmt::Direction::Increment;
                } else if (slhs > srhs) {
                    ast.direction = AstForStmt::Direction::Decrement;
                }
            } else {
                if (lhs < rhs) {
                    ast.direction = AstForStmt::Direction::Increment;
                } else {
                    ast.direction = AstForStmt::Direction::Decrement;
                }
            }
        } else if (llvm::isa<TypeFloatingPoint>(type)) {
            auto lhs = std::get<double>(from->value);
            auto rhs = std::get<double>(to->value);
            if (lhs == rhs) {
                ast.direction = AstForStmt::Direction::Increment;
                equal = true;
            } else if (lhs < rhs) {
                ast.direction = AstForStmt::Direction::Increment;
            } else {
                ast.direction = AstForStmt::Direction::Decrement;
            }
        }
    }

    if (ast.step == nullptr) {
        return;
    }

    auto* step = llvm::dyn_cast<AstLiteralExpr>(ast.step);
    if (step == nullptr) {
        return;
    }

    if (step->type->isSignedIntegral()) {
        auto val = static_cast<int64_t>(std::get<uint64_t>(step->value));
        if (val < 0) {
            if (ast.direction == AstForStmt::Direction::Increment) {
                if (equal) {
                    ast.direction = AstForStmt::Direction::Decrement;
                } else {
                    ast.direction = AstForStmt::Direction::Skip;
                }
            } else if (ast.direction == AstForStmt::Direction::Unknown) {
                ast.direction = AstForStmt::Direction::Decrement;
            }
        } else if (val > 0) {
            if (ast.direction == AstForStmt::Direction::Decrement) {
                ast.direction = AstForStmt::Direction::Skip;
            } else if (ast.direction == AstForStmt::Direction::Unknown) {
                ast.direction = AstForStmt::Direction::Increment;
            }
        } else {
            // TODO emit warning
            if (ast.direction == AstForStmt::Direction::Unknown) {
                ast.direction = AstForStmt::Direction::Increment;
            }
        }
    } else if (step->type->isUnsignedIntegral()) {
        auto val = std::get<uint64_t>(step->value);
        if (val == 0 || ast.direction == AstForStmt::Direction::Decrement) {
            ast.direction = AstForStmt::Direction::Skip;
        } else if (ast.direction == AstForStmt::Direction::Unknown) {
            ast.direction = AstForStmt::Direction::Increment;
        }
    } else if (step->type->isFloatingPoint()) {
        auto val = std::get<double>(step->value);
        if (val < 0.0) {
            if (ast.direction == AstForStmt::Direction::Increment) {
                if (equal) {
                    ast.direction = AstForStmt::Direction::Decrement;
                } else {
                    ast.direction = AstForStmt::Direction::Skip;
                }
            } else if (ast.direction == AstForStmt::Direction::Unknown) {
                ast.direction = AstForStmt::Direction::Decrement;
            }
        } else if (val > 0.0) {
            if (ast.direction == AstForStmt::Direction::Decrement) {
                ast.direction = AstForStmt::Direction::Skip;
            } else if (ast.direction == AstForStmt::Direction::Unknown) {
                ast.direction = AstForStmt::Direction::Increment;
            }
        } else {
            // TODO emit warning
            if (ast.direction == AstForStmt::Direction::Unknown) {
                ast.direction = AstForStmt::Direction::Increment;
            }
        }
    }
}
