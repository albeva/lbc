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
using namespace lbc;
using namespace Gen;

ValueHandler ValueHandler::createTemp(CodeGen& gen, AstExpr& expr, StringRef name) noexcept {
    auto* value = gen.visit(expr).load();
    auto* var = gen.getBuilder().CreateAlloca(
        expr.type->getLlvmType(gen.getContext()),
        nullptr,
        name);
    gen.getBuilder().CreateStore(value, var);

    return createOpaqueValue(gen, expr.type, var, name);
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

    return createOpaqueValue(gen, expr.type, var, name);
}

ValueHandler ValueHandler::createOpaqueValue(CodeGen& gen, const TypeRoot* type, llvm::Value* value, StringRef name) noexcept {
    auto* symbol = gen.getContext().create<Symbol>(name, type);
    symbol->setLlvmValue(value);
    return { &gen, symbol };
}

ValueHandler::ValueHandler(CodeGen* gen, const TypeRoot* type, llvm::Value* value) noexcept
: PointerUnion{ value }, m_gen{ gen }, m_type{ type } {}

ValueHandler::ValueHandler(CodeGen* gen, Symbol* symbol) noexcept
: PointerUnion{ symbol }, m_gen{ gen }, m_type{ symbol->type() } {}

ValueHandler::ValueHandler(CodeGen* gen, AstIdentExpr& ast) noexcept
: ValueHandler{ gen, ast.symbol } {}

ValueHandler::ValueHandler(CodeGen* gen, AstMemberAccess& ast) noexcept
: PointerUnion{ &ast }, m_gen{ gen }, m_type{ ast.type } {}

ValueHandler::ValueHandler(CodeGen* gen, AstAddressOf& ast) noexcept
: PointerUnion{ &ast }, m_gen{ gen }, m_type{ ast.type } {}

ValueHandler::ValueHandler(CodeGen* gen, AstDereference& ast) noexcept
: PointerUnion{ &ast }, m_gen{ gen }, m_type{ ast.type } {}

llvm::Value* ValueHandler::getAddress() const noexcept {
    if (auto* value = dyn_cast<llvm::Value*>()) {
        return value;
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

    if (auto* member = llvm::dyn_cast<AstMemberAccess>(ast)) {
        return getAggregageAddress(*member);
    }

    llvm_unreachable("Unknown ValueHandler type");
}

llvm::Value* ValueHandler::load() const noexcept {
    auto* addr = getAddress();
    if (is<llvm::Value*>()) {
        return addr;
    }

    if (auto* ast = dyn_cast<AstExpr*>()) {
        if (llvm::isa<AstAddressOf>(ast)) {
            return addr;
        }
    }

    return m_gen->getBuilder().CreateLoad(getLlvmType(), addr);
}

llvm::Type* ValueHandler::getLlvmType() const noexcept {
    if (auto* value = dyn_cast<llvm::Value*>()) {
        return value->getType();
    }

    if (auto* symbol = dyn_cast<Symbol*>()) {
        return symbol->type()->getLlvmType(m_gen->getContext());
    }

    if (auto* ast = dyn_cast<AstExpr*>()) {
        return ast->type->getLlvmType(m_gen->getContext());
    }

    llvm_unreachable("Unknown type in getLlvmType");
}

llvm::Value* ValueHandler::getAggregageAddress(AstMemberAccess& ast) const noexcept {
    auto& builder = m_gen->getBuilder();

    llvm::SmallVector<llvm::Value*> idxs{};
    idxs.push_back(builder.getInt32(0));

    llvm::Value* addr = nullptr;
    llvm::Type* type = nullptr;

    const auto createGEP = [&]() {
        if (idxs.size() < 2) {
            return;
        }
        addr = builder.CreateGEP(type, addr, idxs);
        idxs.pop_back_n(idxs.size() - 1);
    };

    // a.b.c.d = [ a, b, c, d ]
    for (size_t i = 0; i < ast.exprs.size(); i++) {
        bool last = i == (ast.exprs.size() - 1);

        auto* symbol = m_gen->visit(*ast.exprs[i]).dyn_cast<Symbol*>();
        if (symbol == nullptr) {
            fatalError("MemberAccess expressions shoudl be symbols!");
        }

        if (i == 0) {
            type = symbol->type()->getLlvmType(m_gen->getContext());
            addr = symbol->getLlvmValue();
        } else {
            idxs.push_back(builder.getInt32(symbol->getIndex()));
        }

        if (symbol->type()->isPointer() && !last) {
            createGEP();
            type = llvm::cast<TypePointer>(symbol->type())->getBase()->getLlvmType(m_gen->getContext());
            addr = builder.CreateLoad(
                symbol->type()->getLlvmType(m_gen->getContext()),
                addr);
        }
    }
    createGEP();

    return addr;
}

void ValueHandler::store(llvm::Value* val) const noexcept {
    auto* addr = getAddress();
    m_gen->getBuilder().CreateStore(val, addr);
}

void ValueHandler::store(ValueHandler& val) const noexcept {
    store(val.load());
}
