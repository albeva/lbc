//
// Created by Albert on 31/05/2021.
//
#include "ValueHandler.hpp"
#include "Ast/Ast.hpp"
#include "CodeGen.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"
#include "Type/TypeUdt.hpp"
#include <llvm/ADT/TypeSwitch.h>
using namespace lbc;
using namespace Gen;

ValueHandler ValueHandler::createTemp(CodeGen& gen, AstExpr& expr, StringRef name) noexcept {
    auto* value = gen.visit(expr).load();
    auto* var = gen.getBuilder().CreateAlloca(
        expr.type->getLlvmType(gen.getContext()),
        nullptr,
        name);
    gen.getBuilder().CreateStore(value, var);
    return { &gen, { var, true } };
}

ValueHandler ValueHandler::createTempOrConstant(CodeGen& gen, AstExpr& expr, StringRef name) noexcept {
    auto* value = gen.visit(expr).load();
    if (isa<llvm::Constant>(value)) {
        return { &gen, { value, false } };
    }

    auto* var = gen.getBuilder().CreateAlloca(
        expr.type->getLlvmType(gen.getContext()),
        nullptr,
        name);
    gen.getBuilder().CreateStore(value, var);
    return { &gen, { var, true } };
}

ValueHandler::ValueHandler(CodeGen* gen, ValuePtr ptr) noexcept
: PointerUnion{ ptr }, m_gen{ gen } {}

ValueHandler::ValueHandler(CodeGen* gen, Symbol* symbol) noexcept
: PointerUnion{ symbol }, m_gen{ gen } {}

ValueHandler::ValueHandler(CodeGen* gen, llvm::Value* value) noexcept
: PointerUnion{ ValuePtr{ value, false } }, m_gen{ gen } {}

ValueHandler::ValueHandler(CodeGen* gen, AstIdentExpr& ast) noexcept
: PointerUnion{ ast.symbol }, m_gen{ gen } {}

ValueHandler::ValueHandler(CodeGen* gen, AstMemberAccess& ast) noexcept
: PointerUnion{ &ast }, m_gen{ gen } {}

ValueHandler::ValueHandler(CodeGen* gen, AstDereference& ast) noexcept
: PointerUnion{ &ast }, m_gen{ gen } {}

ValueHandler::ValueHandler(CodeGen* gen, AstAddressOf& ast) noexcept
: PointerUnion{ &ast }, m_gen{ gen } {}

llvm::Value* ValueHandler::getAddress() const noexcept {
    if (is<ValuePtr>()) {
        return PointerUnion::get<ValuePtr>().getPointer();
    }

    if (auto* symbol = dyn_cast<Symbol*>()) {
        return symbol->getLlvmValue();
    }

    auto* ast = dyn_cast<AstExpr*>();

    if (auto* deref = llvm::dyn_cast<AstDereference>(ast)) {
        return m_gen->visit(*deref->expr).load();
    }

    if (auto* addrOf = llvm::dyn_cast<AstAddressOf>(ast)) {
        return m_gen->visit(*addrOf->expr).getAddress();
    }

    // a.b.c.d = { lhs a, { lhs b, { lhs c, rhs d }}}
    // FIXME: when member is a pointer access, we need to emit GEP and then recursively call getAddress()
    if (auto* member = llvm::dyn_cast<AstMemberAccess>(ast)) {
        auto& builder = m_gen->getBuilder();
        const auto* type = member->lhs->type;

        auto* lhs = m_gen->visit(*member->lhs).getAddress();
        if (const auto* ptr = llvm::dyn_cast<TypePointer>(type)) {
            lhs = builder.CreateLoad(
                ptr->getLlvmType(m_gen->getContext()),
                lhs);
            type = ptr->getBase();
        }

        llvm::SmallVector<llvm::Value*, 4> idxs;
        idxs.push_back(builder.getInt64(0));
        auto* addr = m_gen->visit(*member->rhs).getAggregateAddress(lhs, idxs);

        return builder.CreateGEP(
            type->getLlvmType(m_gen->getContext()),
            addr,
            idxs);
    }

    llvm_unreachable("Unknown ValueHandler type");
}

llvm::Type* ValueHandler::getLlvmType() const noexcept {
    if (is<ValuePtr>()) {
        auto* value = PointerUnion::get<ValuePtr>().getPointer();
        auto* type = value->getType();
        if (!PointerUnion::get<ValuePtr>().getInt()) {
            return type;
        }
        return type->getPointerElementType();
    }

    if (auto* symbol = dyn_cast<Symbol*>()) {
        return symbol->type()->getLlvmType(m_gen->getContext());
    }

    if (auto* ast = dyn_cast<AstExpr*>()) {
        return ast->type->getLlvmType(m_gen->getContext());
    }

    llvm_unreachable("Unknown type in getLlvmType");
}

llvm::Value* ValueHandler::getAggregateAddress(llvm::Value* base, IndexArray& idxs) const noexcept {
    // end of the member access chain
    if (auto* symbol = dyn_cast<Symbol*>()) {
        auto& builder = m_gen->getBuilder();
        idxs.push_back(builder.getInt32(symbol->getIndex()));
        // TODO: if symbol is a pointer, emit pending GEP and recurse to getAddress?
        // if (terminal && symbol->type()->isPointer()) {
        //     base = builder.CreateGEP(base, idxs);
        //     base = builder.CreateLoad(base);
        //     idxs.pop_back_n(idxs.size() - 1);
        // }
        return base;
    }

    auto* ast = dyn_cast<AstExpr*>();

    // middle of the chain
    if (auto* member = llvm::dyn_cast<AstMemberAccess>(ast)) {
        base = m_gen->visit(*member->lhs).getAggregateAddress(base, idxs);
        return m_gen->visit(*member->rhs).getAggregateAddress(base, idxs);
    }

    llvm_unreachable("Unknown aggregate member access type");
}

llvm::Value* ValueHandler::load() const noexcept {
    auto* addr = getAddress();
    if (isa<llvm::Function>(addr)) {
        return addr;
    }

    if (is<ValuePtr>() && !PointerUnion::get<ValuePtr>().getInt()) {
        return addr;
    }

    if (auto* ast = dyn_cast<AstExpr*>()) {
        if (auto* addrOf = llvm::dyn_cast<AstAddressOf>(ast)) {
            return getAddress();
        }
    }

    return m_gen->getBuilder().CreateLoad(getLlvmType(), addr);
}

void ValueHandler::store(llvm::Value* val) const noexcept {
    auto* addr = getAddress();
    m_gen->getBuilder().CreateStore(val, addr);
}
