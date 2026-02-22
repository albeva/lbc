//
// Created by Albert Varaksin on 15/02/2026.
//
#include "Type.hpp"
#include "Aggregate.hpp"
#include "Compound.hpp"
#include "Numeric.hpp"
using namespace lbc;

namespace {

auto removeQualRef(const Type* type) -> std::pair<const Type*, TypeComparisonResult> {
    TypeComparisonResult res = TypeComparisonResult::Identical;
    while (true) {
        if (const auto* qual = llvm::dyn_cast<TypeQualified>(type)) {
            if (qual->isConst()) {
                res = TypeComparisonResult::Convertible;
                res.constness = TypeComparisonResult::Flags::Removed;
            }
            type = type->getBaseType();
            continue;
        }
        if (const auto* ref = llvm::dyn_cast<TypeReference>(type)) {
            res = TypeComparisonResult::Convertible;
            res.reference = TypeComparisonResult::Flags::Removed;
            type = ref->getBaseType();
            continue;
        }
        break;
    }
    return std::make_pair(type, res);
}

auto removeRef(const Type* type) -> std::pair<const Type*, TypeComparisonResult> {
    if (const auto* ref = llvm::dyn_cast<TypeReference>(type)) {
        TypeComparisonResult res = TypeComparisonResult::Convertible;
        res.reference = TypeComparisonResult::Flags::Removed;
        return std::make_pair(ref->getBaseType(), res);
    }
    return std::make_pair(type, TypeComparisonResult::Identical);
}

// auto removeQual(const Type* type) -> std::pair<const Type*, TypeComparison> {
//     if (const auto* ref = llvm::dyn_cast<TypeQualified>(type)) {
//         TypeComparison res = TypeComparison::Convertible;
//         if (ref->isConst()) {
//             res.constness = TypeComparison::Flags::Removed;
//         }
//         return std::make_pair(ref->getBaseType(), res);
//     }
//     return std::make_pair(type, TypeComparison::Identical);
// }

// target <- from
auto toIntegral(const TypeIntegral* target, const Type* from, const Type::ComparisonFlags /* flags */) -> TypeComparisonResult {
    auto [src, res] = removeQualRef(from);

    if (const auto* rhs = llvm::dyn_cast<TypeIntegral>(src)) {
        if (target == src) {
            return res;
        }
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
auto toFloatingPoint(const TypeFloatingPoint* target, const Type* from, const Type::ComparisonFlags /* flags */) -> TypeComparisonResult {
    auto [src, res] = removeQualRef(from);
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
auto toPointer(const TypePointer* target, const Type* from, const Type::ComparisonFlags flags) -> TypeComparisonResult {
    using namespace ::flags;
    auto [src, res] = removeQualRef(from);

    if (target == src) {
        if (has(flags, Type::ComparisonFlags::AllowRemovingConstFromPtr) || res.constness != TypeComparisonResult::Flags::Removed) {
            return res;
        }
    }

    if (target->isAnyPtr() && src->isPointer()) {
        return res;
    }

    if (src->isNull()) {
        res.result = TypeComparisonResult::FromNullPtr;
        return res;
    }

    if (const auto* qual = llvm::dyn_cast<TypeQualified>(target->getBaseType())) {
        if (const auto* rhs = llvm::dyn_cast<TypePointer>(src)) {
            if (auto result = qual->compare(rhs->getBaseType(), Type::ComparisonFlags::AllowRemovingConstFromPtr); result.result != TypeComparisonResult::Incompatible) {
                if (result.constness == TypeComparisonResult::Flags::Removed || res.constness == TypeComparisonResult::Flags::Removed) {
                    result.constness = TypeComparisonResult::Flags::Unchanged;
                } else {
                    result.constness = TypeComparisonResult::Flags::Added;
                }
                if (res.reference != TypeComparisonResult::Flags::Unchanged) {
                    result.reference = res.reference;
                }
                return result;
            }
        }
    }

    return TypeComparisonResult::Incompatible;
}

// target <- from
auto toReference(const Type* target, const Type* from, const Type::ComparisonFlags /* flags */) -> TypeComparisonResult {
    if (auto res = target->getBaseType()->compare(from); res.result != TypeComparisonResult::Incompatible) {
        if (res.result == TypeComparisonResult::Identical) {
            res.result = TypeComparisonResult::Convertible;
        }
        res.reference = TypeComparisonResult::Flags::Added;
        return res;
    }
    return TypeComparisonResult::Incompatible;
}

// target <- from
auto toQualified(const TypeQualified* target, const Type* from, const Type::ComparisonFlags /* flags */) -> TypeComparisonResult {
    if (target->isConst()) {
        auto [src, res] = removeRef(from);
        if (const auto* rhs = llvm::dyn_cast<TypeQualified>(src); rhs != nullptr && rhs->isConst()) {
            auto result = target->getBaseType()->compare(rhs->getBaseType(), Type::ComparisonFlags::AllowRemovingConstFromPtr);
            if (res.reference == TypeComparisonResult::Flags::Removed) {
                result.reference = TypeComparisonResult::Flags::Removed;
            }
            result.constness = TypeComparisonResult::Flags::Unchanged;
            return result;
        }

        if (auto result = target->getBaseType()->compare(from); result.result != TypeComparisonResult::Incompatible) {
            result.result = TypeComparisonResult::Convertible;
            result.constness = TypeComparisonResult::Flags::Added;
            return result;
        }

        return TypeComparisonResult::Incompatible;
    }
    return target->getBaseType()->compare(from);
}

} // namespace

// Comparison direction is compare to(this) from(other)
// so the TypeComparison describes changes from other to this.
//
// dim i as long = 0 as const uinteger
//
// longTy->compare(constUIntTy) -> TypeComparison {
//     .result = Convertible,
//     .sign = Added,
//     .size = Added,
//     .constness = Removed
// }
auto Type::compare(const Type* from, ComparisonFlags flags) const -> TypeComparisonResult {
    using namespace ::flags;

    // same type?
    if (this == from) {
        return TypeComparisonResult::Identical;
    }
    // integral
    if (const auto* to = llvm::dyn_cast<TypeIntegral>(this)) {
        return toIntegral(to, from, flags);
    }
    // floating point
    if (const auto* to = llvm::dyn_cast<TypeFloatingPoint>(this)) {
        return toFloatingPoint(to, from, flags);
    }
    // pointer
    if (const auto* to = llvm::dyn_cast<TypePointer>(this)) {
        return toPointer(to, from, flags);
    }
    // reference
    if (const auto* to = llvm::dyn_cast<TypeReference>(this)) {
        return toReference(to, from, flags);
    }
    // qualified
    if (const auto* to = llvm::dyn_cast<TypeQualified>(this)) {
        return toQualified(to, from, flags);
    }

    // no match
    return TypeComparisonResult::Incompatible;
}
