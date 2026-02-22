//
// Created by Albert Varaksin on 15/02/2026.
//
#include "Type.hpp"
#include "Aggregate.hpp"
#include "Compound.hpp"
#include "Numeric.hpp"
using namespace lbc;

namespace {

auto removeRef(const Type* type) -> std::pair<const Type*, TypeComparisonResult> {
    if (const auto* ref = llvm::dyn_cast<TypeReference>(type)) {
        TypeComparisonResult res = TypeComparisonResult::Convertible;
        res.reference = TypeComparisonResult::Flags::Removed;
        return std::make_pair(ref->getBaseType(), res);
    }
    return std::make_pair(type, TypeComparisonResult::Identical);
}

// target <- from
auto toIntegral(const TypeIntegral* target, const Type* from) -> TypeComparisonResult {
    auto [src, res] = removeRef(from);
    if (target == src) {
        return res;
    }
    if (const auto* rhs = llvm::dyn_cast<TypeIntegral>(src)) {
        if (target->getBytes() > rhs->getBytes()) {
            res.result = TypeComparisonResult::Convertible;
            if (target->isSigned()) {
                if (not rhs->isSigned()) {
                    res.sign = TypeComparisonResult::Flags::Added;
                }
            } else if (rhs->isSigned()) {
                return TypeComparisonResult::Incompatible;
            }
            res.size = TypeComparisonResult::Flags::Added;
            return res;
        }
    }
    return TypeComparisonResult::Incompatible;
}

// target <- from
auto toFloatingPoint(const TypeFloatingPoint* target, const Type* from) -> TypeComparisonResult {
    auto [src, res] = removeRef(from);
    if (target == src) {
        return res;
    }
    if (const auto* rhs = llvm::dyn_cast<TypeFloatingPoint>(src)) {
        if (target->getBytes() > rhs->getBytes()) {
            res.result = TypeComparisonResult::Convertible;
            res.size = TypeComparisonResult::Flags::Added;
            return res;
        }
    }
    return TypeComparisonResult::Incompatible;
}

// target <- from
auto toPointer(const TypePointer* target, const Type* from) -> TypeComparisonResult {
    auto [src, res] = removeRef(from);

    if (target == src) {
        return res;
    }
    if (target->isAnyPtr() && src->isPointer()) {
        res.result = TypeComparisonResult::Convertible;
        return res;
    }
    if (src->isNull()) {
        res.result = TypeComparisonResult::Convertible;
        return res;
    }
    return TypeComparisonResult::Incompatible;
}

// target <- from
auto toReference(const Type* target, const Type* from) -> TypeComparisonResult {
    if (auto res = target->getBaseType()->compare(from); res.result != TypeComparisonResult::Incompatible) {
        if (res.result == TypeComparisonResult::Identical) {
            res.result = TypeComparisonResult::Convertible;
        }
        res.reference = TypeComparisonResult::Flags::Added;
        return res;
    }
    return TypeComparisonResult::Incompatible;
}

} // namespace

// Comparison direction is compare to(this) from(other)
// so the TypeComparison describes changes from other to this.
//
// dim i as long = 0 as uinteger
//
// longTy->compare(uintTy) -> TypeComparison {
//     .result = Convertible,
//     .sign = Added,
//     .size = Added
// }
auto Type::compare(const Type* from) const -> TypeComparisonResult {
    // same type?
    if (this == from) {
        return TypeComparisonResult::Identical;
    }
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
    // reference
    if (const auto* to = llvm::dyn_cast<TypeReference>(this)) {
        return toReference(to, from);
    }
    // no match
    return TypeComparisonResult::Incompatible;
}
