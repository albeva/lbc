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

ValueHandler ValueHandler::createTemp(CodeGen& gen, AstExpr& expr, llvm::StringRef name) {
    auto* value = gen.visit(expr).load();
    auto* var = gen.getBuilder().CreateAlloca(
        expr.type->getLlvmType(gen.getContext()),
        nullptr,
        name
    );
    gen.getBuilder().CreateStore(value, var);

    return createOpaqueValue(gen, expr.type, var, name);
}

ValueHandler ValueHandler::createTempOrConstant(CodeGen& gen, AstExpr& expr, llvm::StringRef name) {
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

ValueHandler ValueHandler::createOpaqueValue(CodeGen& gen, const TypeRoot* type, llvm::Value* value, llvm::StringRef name) {
    auto* symbol = gen.getContext().create<Symbol>(name, /* symbol table */ nullptr, type, /* declaring ast node */ nullptr);
    symbol->setLlvmValue(value);
    return { &gen, symbol };
}

ValueHandler::ValueHandler(CodeGen* gen, const TypeRoot* type, llvm::Value* value)
: PointerUnion{ value }, m_gen{ gen }, m_type{ type } {}

ValueHandler::ValueHandler(CodeGen* gen, Symbol* symbol)
: PointerUnion{ symbol }, m_gen{ gen }, m_type{ symbol->getType() } {}

ValueHandler::ValueHandler(CodeGen* gen, AstIdentExpr& ast)
: ValueHandler{ gen, ast.symbol } {}

ValueHandler::ValueHandler(CodeGen* gen, AstBinaryExpr& ast)
: PointerUnion{ &ast }, m_gen{ gen }, m_type{ ast.type } {}

ValueHandler::ValueHandler(CodeGen* gen, AstAddressOf& ast)
: PointerUnion{ &ast }, m_gen{ gen }, m_type{ ast.type } {}

ValueHandler::ValueHandler(CodeGen* gen, AstDereference& ast)
: PointerUnion{ &ast }, m_gen{ gen }, m_type{ ast.type } {}

llvm::Value* ValueHandler::getAddress() const {
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

    if (auto* member = llvm::dyn_cast<AstBinaryExpr>(ast)) {
        return getAggregageAddress(*member);
    }

    llvm_unreachable("Unknown ValueHandler type");
}

llvm::Value* ValueHandler::load() const {
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

llvm::Type* ValueHandler::getLlvmType() const {
    if (auto* value = dyn_cast<llvm::Value*>()) {
        return value->getType();
    }

    if (auto* symbol = dyn_cast<Symbol*>()) {
        return symbol->getType()->getLlvmType(m_gen->getContext());
    }

    if (auto* ast = dyn_cast<AstExpr*>()) {
        return ast->type->getLlvmType(m_gen->getContext());
    }

    llvm_unreachable("Unknown type in getLlvmType");
}

llvm::Value* ValueHandler::getAggregageAddress(AstBinaryExpr& ast) const {
    auto& builder = m_gen->getBuilder();

    // Very dumb code to serialize member access
    // TODO: Refactor this to a builder which generates code recursively
    struct Generator final {
        std::vector<AstExpr*> exprs{};
        // // a.b.c.d = <<<a, b>, c>, d>
        void build(AstBinaryExpr& ast) {
            if (auto* left = llvm::dyn_cast<AstBinaryExpr>(ast.lhs)) {
                build(*left);
            } else {
                exprs.push_back(ast.lhs);
            }
            exprs.push_back(ast.rhs);
        }
    };
    Generator gen{};
    gen.build(ast);
    auto exprs = std::move(gen.exprs);

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

    // a.b.c.d = [a, b, c, d]
    for (size_t i = 0; i < exprs.size(); i++) {
        bool const last = i == (exprs.size() - 1);

        auto* symbol = m_gen->visit(*exprs[i]).dyn_cast<Symbol*>();
        if (symbol == nullptr) {
            fatalError("MemberAccess expressions shoudl be symbols!");
        }

        if (i == 0) {
            type = symbol->getType()->getLlvmType(m_gen->getContext());
            addr = symbol->getLlvmValue();
        } else {
            idxs.push_back(builder.getInt32(symbol->getIndex()));
        }

        if (symbol->getType()->isPointer() && !last) {
            createGEP();
            type = llvm::cast<TypePointer>(symbol->getType())->getBase()->getLlvmType(m_gen->getContext());
            addr = builder.CreateLoad(
                symbol->getType()->getLlvmType(m_gen->getContext()),
                addr
            );
        }
    }
    createGEP();

    return addr;
}

void ValueHandler::store(llvm::Value* val) const {
    auto* addr = getAddress();
    m_gen->getBuilder().CreateStore(val, addr);
}

void ValueHandler::store(ValueHandler& val) const {
    store(val.load());
}
