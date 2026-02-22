//
// Created by Albert Varaksin on 21/02/2026.
//
#include "TypeFactory.hpp"
#include "Driver/Context.hpp"
#include "llvm/ADT/Hashing.h"
using namespace lbc;

TypeFactory::TypeFactory(Context& context)
: m_context(context) {
    createSingletonTypes();
    m_anyPtr = getPointer(getAny()); // NOLINT(*-prefer-member-initializer)
}

auto TypeFactory::getPointer(const Type* type) -> const TypePointer* {
    if (const auto iter = m_pointers.find(type); iter != m_pointers.end()) {
        return iter->second;
    }
    return m_pointers.insert(std::make_pair(type, create<TypePointer>(type))).first->second;
}

auto TypeFactory::getReference(const Type* type) -> const TypeReference* {
    if (const auto iter = m_references.find(type); iter != m_references.end()) {
        return iter->second;
    }
    return m_references.insert(std::make_pair(type, create<TypeReference>(type))).first->second;
}

auto TypeFactory::getFunction(std::span<const Type*> params, const Type* returnType) -> const TypeFunction* {
    const auto hash = llvm::hash_combine(returnType, llvm::hash_combine_range(params.begin(), params.end()));
    auto& arr = m_functions[hash];
    for (const auto* func : arr) {
        if (func->getReturnType() == returnType && std::ranges::equal(params, func->getParams())) {
            return func;
        }
    }
    return arr.emplace_back(create<TypeFunction>(params, returnType));
}

auto TypeFactory::allocate(const std::size_t size, const std::size_t alignment) const -> void* {
    return m_context.allocate(size, alignment);
}

void TypeFactory::createSingletonTypes() {
    for (const auto kind : kSingletonKinds) {
        switch (kind) {
        case TypeKind::Void:
        case TypeKind::Null:
        case TypeKind::Any:
        case TypeKind::Bool:
        case TypeKind::ZString:
            setSingleton(create<Type>(kind));
            break;
        case TypeKind::UByte:
            setSingleton(create<TypeIntegral>(kind, static_cast<std::uint8_t>(sizeof(std::uint8_t)), false));
            break;
        case TypeKind::UShort:
            setSingleton(create<TypeIntegral>(kind, static_cast<std::uint8_t>(sizeof(std::uint16_t)), false));
            break;
        case TypeKind::UInteger:
            setSingleton(create<TypeIntegral>(kind, static_cast<std::uint8_t>(sizeof(std::uint32_t)), false));
            break;
        case TypeKind::ULong:
            setSingleton(create<TypeIntegral>(kind, static_cast<std::uint8_t>(sizeof(std::uint64_t)), false));
            break;
        case TypeKind::Byte:
            setSingleton(create<TypeIntegral>(kind, static_cast<std::uint8_t>(sizeof(std::int8_t)), true));
            break;
        case TypeKind::Short:
            setSingleton(create<TypeIntegral>(kind, static_cast<std::uint8_t>(sizeof(std::int16_t)), true));
            break;
        case TypeKind::Integer:
            setSingleton(create<TypeIntegral>(kind, static_cast<std::uint8_t>(sizeof(std::int32_t)), true));
            break;
        case TypeKind::Long:
            setSingleton(create<TypeIntegral>(kind, static_cast<std::uint8_t>(sizeof(std::int64_t)), true));
            break;
        case TypeKind::Single:
            setSingleton(create<TypeFloatingPoint>(kind, static_cast<std::uint8_t>(sizeof(float))));
            break;
        case TypeKind::Double:
            setSingleton(create<TypeFloatingPoint>(kind, static_cast<std::uint8_t>(sizeof(double))));
            break;
        case TypeKind::Pointer:
        case TypeKind::Reference:
        case TypeKind::Function:
            std::unreachable();
        }
    }
}
