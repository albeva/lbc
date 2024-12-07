//
// Created by Albert on 31/05/2021.
//
#include "ValueHandler.hpp"
#include "Ast/Ast.hpp"
#include "Builders/MemberExprBuilder.hpp"
#include "CodeGen.hpp"
#include "Driver/Context.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace Gen;

auto ValueHandler::createTemp(CodeGen& gen, AstExpr& expr, llvm::StringRef name) -> ValueHandler {
    auto* value = gen.visit(expr).load();
    auto* var = gen.getBuilder().CreateAlloca(
        expr.type->getLlvmType(gen.getContext()),
        nullptr,
        name
    );
    gen.getBuilder().CreateStore(value, var);
    return createOpaqueValue(gen, expr.type, var, name);
}

auto ValueHandler::createTempOrConstant(CodeGen& gen, AstExpr& expr, llvm::StringRef name) -> ValueHandler {
    auto* value = gen.visit(expr).load();
    if (llvm::isa<llvm::Constant>(value)) {
        return { &gen, expr.type, value };
    }

    auto* var = gen.getBuilder().CreateAlloca(
        expr.type->getLlvmType(gen.getContext()),
        nullptr,
        name
    );
    gen.getBuilder().CreateStore(value, var);
    return createOpaqueValue(gen, expr.type, var, name);
}

auto ValueHandler::createOpaqueValue(CodeGen& gen, const TypeRoot* type, llvm::Value* value, llvm::StringRef name) -> ValueHandler {
    auto* symbol = gen.getContext().create<Symbol>(name, /* symbol table */ nullptr, type, /* declaring ast node */ nullptr);
    symbol->setLlvmValue(value);
    return { &gen, symbol };
}

ValueHandler::ValueHandler(CodeGen* gen, const TypeRoot* type, llvm::Value* value)
: PointerUnion { value }
, m_gen { gen }
, m_type { type } {
}

ValueHandler::ValueHandler(CodeGen* gen, Symbol* symbol)
: PointerUnion { symbol }
, m_gen { gen }
, m_type { symbol->getType() } {
}

ValueHandler::ValueHandler(CodeGen* gen, AstIdentExpr& ast)
: ValueHandler { gen, ast.symbol } {
}

ValueHandler::ValueHandler(CodeGen* gen, AstMemberExpr& ast)
: PointerUnion { &ast }
, m_gen { gen }
, m_type { ast.type } {
}

ValueHandler::ValueHandler(CodeGen* gen, AstAddressOf& ast)
: PointerUnion { &ast }
, m_gen { gen }
, m_type { ast.type } {
}

ValueHandler::ValueHandler(CodeGen* gen, AstAlignOfExpr& ast)
: PointerUnion { &ast }
, m_gen { gen }
, m_type { ast.type } {
}

ValueHandler::ValueHandler(CodeGen* gen, AstSizeOfExpr& ast)
: PointerUnion { &ast }
, m_gen { gen }
, m_type { ast.type } {
}

ValueHandler::ValueHandler(CodeGen* gen, AstDereference& ast)
: PointerUnion { &ast }
, m_gen { gen }
, m_type { ast.type } {
}

auto ValueHandler::getAddress() const -> llvm::Value* {
    if (auto* value = dyn_cast<llvm::Value*>()) {
        return value;
    }

    if (const auto* symbol = dyn_cast<Symbol*>()) {
        return symbol->getLlvmValue();
    }

    auto* ast = dyn_cast<AstExpr*>();

    if (const auto* deref = llvm::dyn_cast<AstDereference>(ast)) {
        return m_gen->visit(*deref->expr).load();
    }

    if (const auto* addrOf = llvm::dyn_cast<AstAddressOf>(ast)) {
        return m_gen->visit(*addrOf->expr).getAddress();
    }

    if (auto* member = llvm::dyn_cast<AstMemberExpr>(ast)) {
        return MemberExprBuilder { *m_gen, *member }.build();
    }

    llvm_unreachable("Unknown ValueHandler type");
}

auto ValueHandler::load(const bool asReference) const -> llvm::Value* {
    auto* addr = getAddress();

    // value is an intermediary. It is already loaded and expected type
    if (is<llvm::Value*>()) {
        return addr;
    }

    auto& builder = m_gen->getBuilder();
    auto& ctx = m_gen->getContext();

    if (auto* expr = dyn_cast<AstExpr*>()) {
        // Handle: @expr
        if (const auto* addrof = llvm::dyn_cast<AstAddressOf>(expr)) {
            if (addrof->expr->type->isReference()) {
                return builder.CreateLoad(getLlvmType(), addr);
            }
            return addr;
        }
    }

    // If we are loading reference - return address
    if (asReference) {
        // if we are loading reference of reference - return the address reference holds
        if (const auto& ref = llvm::dyn_cast<TypeReference>(m_type)) {
            return builder.CreateLoad(ref->getLlvmType(ctx), addr);
        }
        return addr;
    }

    // If this is a reference, dereference it
    if (const auto& ref = llvm::dyn_cast<TypeReference>(m_type)) {
        addr = builder.CreateLoad(ref->getLlvmType(ctx), addr);
        return builder.CreateLoad(ref->getBase()->getLlvmType(ctx), addr);
    }

    // Load value
    return builder.CreateLoad(getLlvmType(), addr);
}

auto ValueHandler::getLlvmType() const -> llvm::Type* {
    if (const auto* value = dyn_cast<llvm::Value*>()) {
        return value->getType();
    }
    if (const auto* symbol = dyn_cast<Symbol*>()) {
        return symbol->getType()->getLlvmType(m_gen->getContext());
    }
    if (const auto* ast = dyn_cast<AstExpr*>()) {
        return ast->type->getLlvmType(m_gen->getContext());
    }
    llvm_unreachable("Unknown type in getLlvmType");
}

void ValueHandler::store(llvm::Value* val) const {
    auto* addr = getAddress();
    if (llvm::isa<TypeReference>(m_type) && !is<llvm::Value*>()) {
        addr = m_gen->getBuilder().CreateLoad(getLlvmType(), addr);
    }
    m_gen->getBuilder().CreateStore(val, addr);
}

void ValueHandler::store(const ValueHandler& val) const {
    store(val.load());
}
