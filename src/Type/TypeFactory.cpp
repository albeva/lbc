//
// Created by Albert Varaksin on 21/02/2026.
//
#include "TypeFactory.hpp"

#include "Driver/Context.hpp"
using namespace lbc;

TypeFactory::TypeFactory(Context& context)
: m_context(context) {
    initializeTypes();
}

void TypeFactory::initializeTypes() {
    for (const auto kind : kSingletonKinds) {
        switch (kind) {
        case TypeKind::Void:
        case TypeKind::Null:
        case TypeKind::Any:
        case TypeKind::Bool:
        case TypeKind::ZString:
            setSingleton(m_context.create<Type>(kind));
            break;
        case TypeKind::UByte:
            setSingleton(m_context.create<TypeIntegral>(kind, sizeof(std::uint8_t), false));
            break;
        case TypeKind::UShort:
            setSingleton(m_context.create<TypeIntegral>(kind, sizeof(std::uint16_t), false));
            break;
        case TypeKind::UInteger:
            setSingleton(m_context.create<TypeIntegral>(kind, sizeof(std::uint32_t), false));
            break;
        case TypeKind::ULong:
            setSingleton(m_context.create<TypeIntegral>(kind, sizeof(std::uint64_t), false));
            break;
        case TypeKind::Byte:
            setSingleton(m_context.create<TypeIntegral>(kind, sizeof(std::int8_t), true));
            break;
        case TypeKind::Short:
            setSingleton(m_context.create<TypeIntegral>(kind, sizeof(std::int16_t), true));
            break;
        case TypeKind::Integer:
            setSingleton(m_context.create<TypeIntegral>(kind, sizeof(std::int32_t), true));
            break;
        case TypeKind::Long:
            setSingleton(m_context.create<TypeIntegral>(kind, sizeof(std::int64_t), true));
            break;
        case TypeKind::Single:
            setSingleton(m_context.create<TypeFloatingPoint>(kind, sizeof(float)));
            break;
        case TypeKind::Double:
            setSingleton(m_context.create<TypeFloatingPoint>(kind, sizeof(double)));
            break;
        case TypeKind::Pointer:
        case TypeKind::Reference:
        case TypeKind::Qualified:
        case TypeKind::Function:
            std::unreachable();
        }
    }
}
