//
// Created by Albert on 29/05/2021.
//
#include "TypeUdt.hpp"
#include "Driver/Context.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/TypeProxy.hpp"
using namespace lbc;

TypeUDT::TypeUDT(Symbol& symbol, SymbolTable& symbolTable, bool packed)
: TypeRoot{ TypeFamily::UDT },
  m_symbol{ symbol },
  m_symbolTable{ symbolTable },
  m_packed(packed) {
    setProxy(symbol.getTypeProxy());
    symbol.getFlags().isType = true;
}

const TypeUDT* TypeUDT::get(Context& context, Symbol& symbol, SymbolTable& symbolTable, bool packed) {
    if (const auto* type = symbol.getType()) {
        if (const auto* udt = llvm::dyn_cast<TypeUDT>(type)) {
            return udt;
        }
        fatalError("Symbol should hold UDT type pointer!");
    }

    return context.create<TypeUDT>(symbol, symbolTable, packed);
}

std::string TypeUDT::asString() const {
    return m_symbol.name().str();
}

llvm::Type* TypeUDT::genLlvmType(Context& context) const {
    llvm::SmallVector<llvm::Type*> elems;
    elems.reserve(m_symbolTable.size());
    for (auto* symbol : m_symbolTable.getSymbols()) {
        auto* ty = symbol->getType()->getLlvmType(context);
        elems.emplace_back(ty);
    }
    return llvm::StructType::create(
        context.getLlvmContext(),
        elems,
        m_symbol.identifier(),
        m_packed);
}
