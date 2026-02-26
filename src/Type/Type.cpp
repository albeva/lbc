//
// Created by Albert Varaksin on 15/02/2026.
//
#include "Type.hpp"
#include "Aggregate.hpp"
#include "Compound.hpp"
#include "Numeric.hpp"
using namespace lbc;

namespace {

// target <- from
auto toIntegral(const TypeIntegral* target, const Type* from) -> bool {
    if (const auto* rhs = llvm::dyn_cast<TypeIntegral>(from)) {
        if (target->getBytes() > rhs->getBytes()) {
            return target->isSigned() || !rhs->isSigned();
        }
    }
    return false;
}

// target <- from
auto toFloatingPoint(const TypeFloatingPoint* target, const Type* from) -> bool {
    if (const auto* rhs = llvm::dyn_cast<TypeFloatingPoint>(from)) {
        return target->getBytes() > rhs->getBytes();
    }
    return false;
}

// target <- from
auto toPointer(const TypePointer* target, const Type* from) -> bool {
    if (target->isAnyPtr() && from->isPointer()) {
        return true;
    }
    return from->isNull();
}

} // namespace

auto Type::common(const Type* other) const -> const Type* {
    if (convertible(other, Conversion::Implicit)) {
        return this;
    }
    if (other->convertible(this, Conversion::Implicit)) {
        return other;
    }
    return nullptr;
}

/// Check if `from` type can be converted to `this` type
auto Type::convertible(const Type* from, const Conversion mode) const -> bool {
    if (this == from) {
        return true;
    }

    switch (mode) {
    case Conversion::Implicit:
        // integral
        if (const auto* to = llvm::dyn_cast<TypeIntegral>(this)) {
            return toIntegral(to, from);
        }

        // floating point
        if (const auto* to = llvm::dyn_cast<TypeFloatingPoint>(this)) {
            return toFloatingPoint(to, from);
        }

        // pointer
        if (const auto* to = llvm::dyn_cast<TypePointer>(this)) {
            return toPointer(to, from);
        }
        return false;
    case Conversion::Cast: {
        // number <- number
        if (isNumeric() && from->isNumeric()) {
            return true;
        }
        // pointer <- null | pointer
        if (isPointer() && (from->isNull() || from->isPointer())) {
            return true;
        }
        return false;
    }
    default:
        std::unreachable();
    }
}

auto Type::removeReference() const -> const Type* {
    if (const auto* ref = llvm::dyn_cast<TypeReference>(this)) {
        return ref->getBaseType();
    }
    return this;
}

auto Type::string() const -> std::string {
    switch (getKind()) {
    case TypeKind::Void:
        return "VOID";
    case TypeKind::Null:
        return "NULL";
    case TypeKind::Any:
        return "ANY";
    default:
        if (const auto tkn = getTokenKind()) {
            return tkn->string().str();
        }
        return "<invalid>";
    }
}
