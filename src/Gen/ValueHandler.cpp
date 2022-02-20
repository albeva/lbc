//
// Created by Albert on 31/05/2021.
//
#include "ValueHandler.hpp"
#include "Ast/Ast.hpp"
#include "CodeGen.hpp"
#include "Driver/Context.hpp"
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

    return createOpaqueIdent(gen, expr.type, var, expr.range, name);
}

ValueHandler ValueHandler::createTempOrConstant(CodeGen& gen, AstExpr& expr, StringRef name) noexcept {
    auto* value = gen.visit(expr).load();
    if (isa<llvm::Constant>(value)) {
        return { &gen, expr.type, value };
    }

    auto* var = gen.getBuilder().CreateAlloca(
        expr.type->getLlvmType(gen.getContext()),
        nullptr,
        name);
    gen.getBuilder().CreateStore(value, var);

    return createOpaqueIdent(gen, expr.type, var, expr.range, name);
}

ValueHandler ValueHandler::createOpaqueIdent(CodeGen& gen, const TypeRoot* type, llvm::Value* value, llvm::SMRange range, StringRef name) noexcept {
    auto* symbol = gen.getContext().create<Symbol>(name, type);
    symbol->setLlvmValue(value);
    return createOpaqueIdent(gen, symbol, range, name);
}

ValueHandler ValueHandler::createOpaqueIdent(CodeGen& gen, Symbol* symbol, llvm::SMRange range, StringRef name) noexcept {
    auto* ident = gen.getContext().create<AstIdentExpr>(range, name);
    ident->symbol = symbol;
    ident->type = symbol->type();
    return { &gen, *ident };
}

ValueHandler::ValueHandler(CodeGen* gen, const TypeRoot* type, llvm::Value* value) noexcept
: PointerUnion{ value }, m_gen{ gen }, m_type{ type } {}

ValueHandler::ValueHandler(CodeGen* gen, AstExpr& ast) noexcept
: PointerUnion{ &ast }, m_gen{ gen }, m_type{ ast.type } {}

llvm::Value* ValueHandler::getAddress() const noexcept {
    if (auto* value = dyn_cast<llvm::Value*>()) {
        return value;
    }

    auto* ast = dyn_cast<AstExpr*>();

    if (auto* ident = llvm::dyn_cast<AstIdentExpr>(ast)) {
        return ident->symbol->getLlvmValue();
    }

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
    if (auto* value = dyn_cast<llvm::Value*>()) {
        return value->getType();
    }

    if (auto* ast = dyn_cast<AstExpr*>()) {
        return ast->type->getLlvmType(m_gen->getContext());
    }

    llvm_unreachable("Unknown type in getLlvmType");
}

llvm::Value* ValueHandler::getAggregateAddress(llvm::Value* base, IndexArray& idxs) const noexcept {
    auto* ast = dyn_cast<AstExpr*>();

    // end of the member access chain
    if (auto* ident = llvm::dyn_cast<AstIdentExpr>(ast)) {
        auto& builder = m_gen->getBuilder();
        idxs.push_back(builder.getInt32(ident->symbol->getIndex()));
        // TODO: if symbol is a pointer, emit pending GEP and recurse to getAddress?
        // if (terminal && symbol->type()->isPointer()) {
        //     base = builder.CreateGEP(base, idxs);
        //     base = builder.CreateLoad(base);
        //     idxs.pop_back_n(idxs.size() - 1);
        // }
        return base;
    }

    // middle of the chain
    if (auto* member = llvm::dyn_cast<AstMemberAccess>(ast)) {
        base = m_gen->visit(*member->lhs).getAggregateAddress(base, idxs);
        return m_gen->visit(*member->rhs).getAggregateAddress(base, idxs);
    }

    llvm_unreachable("Unknown aggregate member access type");
}

llvm::Value* ValueHandler::load() const noexcept {
    auto* addr = getAddress();
    if (isa<llvm::Function>(addr) || is<llvm::Value*>()) {
        return addr;
    }

    if (auto* ast = dyn_cast<AstExpr*>()) {
        if (llvm::isa<AstAddressOf>(ast)) {
            return addr;
        }
    }

    return m_gen->getBuilder().CreateLoad(getLlvmType(), addr);
}

void ValueHandler::store(llvm::Value* val) const noexcept {
    auto* addr = getAddress();
    m_gen->getBuilder().CreateStore(val, addr);
}

void ValueHandler::store(ValueHandler& val) const noexcept {
    store(val.load());
}
