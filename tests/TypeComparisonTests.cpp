#include "pch.hpp"
#include <gtest/gtest.h>
#include "Driver/Context.hpp"
#include "Type/Aggregate.hpp"
#include "Type/Compound.hpp"
#include "Type/Numeric.hpp"
using namespace lbc;

class TypeComparisonTests : public ::testing::Test {
protected:
    Context context;
    TypeFactory& tf = context.getTypeFactory();

    const Type* voidTy = tf.getVoid();
    const Type* nullTy = tf.getNull();
    const Type* anyTy = tf.getAny();
    const Type* boolTy = tf.getBool();
    const Type* zstringTy = tf.getZString();

    const TypeIntegral* byteTy = tf.getByte();
    const TypeIntegral* shortTy = tf.getShort();
    const TypeIntegral* intTy = tf.getInteger();
    const TypeIntegral* longTy = tf.getLong();

    const TypeIntegral* ubyteTy = tf.getUByte();
    const TypeIntegral* ushortTy = tf.getUShort();
    const TypeIntegral* uintTy = tf.getUInteger();
    const TypeIntegral* ulongTy = tf.getULong();

    const TypeFloatingPoint* singleTy = tf.getSingle();
    const TypeFloatingPoint* doubleTy = tf.getDouble();

    using R = TypeComparisonResult::Result;
    using F = TypeComparisonResult::Flags;

    void expectConvertible(const Type* to, const Type* from, F size, F sign) {
        auto res = to->compare(from);
        EXPECT_EQ(res.result, R::Convertible) << "expected Convertible";
        EXPECT_EQ(res.size, size) << "unexpected size flag";
        EXPECT_EQ(res.sign, sign) << "unexpected sign flag";
    }

    void expectIncompatible(const Type* to, const Type* from) {
        EXPECT_EQ(to->compare(from).result, R::Incompatible) << "expected Incompatible";
    }
};

// =============================================================================
// Identity
// =============================================================================

TEST_F(TypeComparisonTests, IdenticalTypes) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* intRef = tf.getReference(intTy);
    const Type* types[] = {
        voidTy, nullTy, anyTy, boolTy, zstringTy,
        byteTy, shortTy, intTy, longTy,
        ubyteTy, ushortTy, uintTy, ulongTy,
        singleTy, doubleTy,
        intPtr, intRef
    };
    for (const auto* ty : types) {
        EXPECT_EQ(ty->compare(ty).result, R::Identical);
    }
}

// =============================================================================
// Signed integral: widening accepted, narrowing rejected
// =============================================================================

TEST_F(TypeComparisonTests, SignedIntegralConversions) {
    const TypeIntegral* chain[] = { byteTy, shortTy, intTy, longTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            expectConvertible(chain[j], chain[i], F::Added, F::Unchanged);
            expectIncompatible(chain[i], chain[j]);
        }
    }
}

// =============================================================================
// Unsigned integral: widening accepted, narrowing rejected
// =============================================================================

TEST_F(TypeComparisonTests, UnsignedIntegralConversions) {
    const TypeIntegral* chain[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            expectConvertible(chain[j], chain[i], F::Added, F::Unchanged);
            expectIncompatible(chain[i], chain[j]);
        }
    }
}

// =============================================================================
// Floating-point: SINGLE -> DOUBLE accepted, reverse rejected
// =============================================================================

TEST_F(TypeComparisonTests, FloatingPointConversions) {
    expectConvertible(doubleTy, singleTy, F::Added, F::Unchanged);
    expectIncompatible(singleTy, doubleTy);
}

// =============================================================================
// Cross-sign: unsigned -> larger signed ok, everything else rejected
// =============================================================================

TEST_F(TypeComparisonTests, CrossSignConversions) {
    const TypeIntegral* unsigned_[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    const TypeIntegral* signed_[] = { byteTy, shortTy, intTy, longTy };

    for (int u = 0; u < 4; u++) {
        for (int s = 0; s < 4; s++) {
            // signed -> unsigned: always rejected
            expectIncompatible(unsigned_[u], signed_[s]);

            // unsigned -> signed: only if signed is strictly larger
            if (signed_[s]->getBytes() > unsigned_[u]->getBytes()) {
                expectConvertible(signed_[s], unsigned_[u], F::Added, F::Added);
            } else {
                expectIncompatible(signed_[s], unsigned_[u]);
            }
        }
    }
}

// =============================================================================
// Integer <-> floating-point: always rejected
// =============================================================================

TEST_F(TypeComparisonTests, IntegerFloatIncompatible) {
    const Type* integrals[] = { byteTy, intTy, longTy, ubyteTy, uintTy };
    const Type* floats[] = { singleTy, doubleTy };

    for (const auto* i : integrals) {
        for (const auto* f : floats) {
            expectIncompatible(f, i);
            expectIncompatible(i, f);
        }
    }
}

// =============================================================================
// Incompatible type families (bool, void, zstring vs numerics)
// =============================================================================

TEST_F(TypeComparisonTests, IncompatibleFamilies) {
    const Type* isolated[] = { boolTy, voidTy, zstringTy };
    const Type* numerics[] = { byteTy, intTy, doubleTy };

    for (const auto* iso : isolated) {
        for (const auto* num : numerics) {
            expectIncompatible(iso, num);
            expectIncompatible(num, iso);
        }
    }
    expectIncompatible(voidTy, nullTy);
    expectIncompatible(nullTy, voidTy);
}

// =============================================================================
// Pointer conversions
// =============================================================================

TEST_F(TypeComparisonTests, PointerConversions) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* bytePtr = tf.getPointer(byteTy);
    const auto* anyPtr = tf.getAnyPtr();
    const auto* intPtrPtr = tf.getPointer(intPtr);

    // null -> any pointer type
    EXPECT_EQ(intPtr->compare(nullTy).result, R::Convertible);
    EXPECT_EQ(anyPtr->compare(nullTy).result, R::Convertible);
    expectIncompatible(intTy, nullTy);

    // any pointer -> AnyPtr
    EXPECT_EQ(anyPtr->compare(intPtr).result, R::Convertible);
    EXPECT_EQ(anyPtr->compare(bytePtr).result, R::Convertible);
    expectIncompatible(anyPtr, intTy);

    // mismatched pointee types
    expectIncompatible(intPtr, bytePtr);
    expectIncompatible(bytePtr, intPtr);

    // pointer vs non-pointer
    expectIncompatible(intPtr, intTy);
    expectIncompatible(intTy, intPtr);

    // nested pointer mismatch
    expectIncompatible(intPtrPtr, intPtr);
    expectIncompatible(intPtr, intPtrPtr);
}

// =============================================================================
// Reference conversions
// =============================================================================

TEST_F(TypeComparisonTests, ReferenceConversions) {
    const auto* intRef = tf.getReference(intTy);

    // value -> reference (add ref)
    auto toRef = intRef->compare(intTy);
    EXPECT_EQ(toRef.result, R::Convertible);
    EXPECT_EQ(toRef.reference, F::Added);

    // reference -> value (remove ref)
    auto fromRef = intTy->compare(intRef);
    EXPECT_EQ(fromRef.result, R::Convertible);
    EXPECT_EQ(fromRef.reference, F::Removed);

    // value -> wider reference (add ref + widen)
    const auto* shortRef = tf.getReference(shortTy);
    auto wider = shortRef->compare(byteTy);
    EXPECT_EQ(wider.result, R::Convertible);
    EXPECT_EQ(wider.reference, F::Added);
    EXPECT_EQ(wider.size, F::Added);

    // reference -> wider value (remove ref + widen)
    const auto* refByte = tf.getReference(byteTy);
    auto derefWider = intTy->compare(refByte);
    EXPECT_EQ(derefWider.result, R::Convertible);
    EXPECT_EQ(derefWider.reference, F::Removed);
    EXPECT_EQ(derefWider.size, F::Added);

    // incompatible reference types
    const auto* dblRef = tf.getReference(doubleTy);
    expectIncompatible(intRef, dblRef);
    expectIncompatible(dblRef, intRef);
}

// =============================================================================
// Function type comparisons
// =============================================================================

TEST_F(TypeComparisonTests, FunctionTypeComparisons) {
    std::array<const Type*, 2> params { intTy, byteTy };
    const auto* fn = tf.getFunction(params, voidTy);

    // identity
    EXPECT_EQ(fn->compare(fn).result, R::Identical);

    // different return type
    const auto* fnInt = tf.getFunction({}, intTy);
    const auto* fnVoid = tf.getFunction({}, voidTy);
    expectIncompatible(fnInt, fnVoid);

    // different param types
    std::array<const Type*, 1> p1 { intTy };
    std::array<const Type*, 1> p2 { byteTy };
    expectIncompatible(tf.getFunction(p1, voidTy), tf.getFunction(p2, voidTy));

    // different param count
    std::array<const Type*, 2> p3 { intTy, intTy };
    expectIncompatible(tf.getFunction(p1, voidTy), tf.getFunction(p3, voidTy));

    // function vs non-function
    expectIncompatible(fn, intTy);
    expectIncompatible(intTy, fn);
}

// =============================================================================
// Common type: same family
// =============================================================================

TEST_F(TypeComparisonTests, CommonSameFamily) {
    // Signed: common is the wider type, symmetric
    const TypeIntegral* signedChain[] = { byteTy, shortTy, intTy, longTy };
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(signedChain[i]->common(signedChain[i]), signedChain[i]);
        for (int j = i + 1; j < 4; j++) {
            EXPECT_EQ(signedChain[i]->common(signedChain[j]), signedChain[j]);
            EXPECT_EQ(signedChain[j]->common(signedChain[i]), signedChain[j]);
        }
    }

    // Unsigned: common is the wider type
    const TypeIntegral* unsignedChain[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            EXPECT_EQ(unsignedChain[i]->common(unsignedChain[j]), unsignedChain[j]);
            EXPECT_EQ(unsignedChain[j]->common(unsignedChain[i]), unsignedChain[j]);
        }
    }

    // Float: SINGLE + DOUBLE -> DOUBLE
    EXPECT_EQ(singleTy->common(doubleTy), doubleTy);
    EXPECT_EQ(doubleTy->common(singleTy), doubleTy);
}

// =============================================================================
// Common type: mixed sign
// =============================================================================

TEST_F(TypeComparisonTests, CommonMixedSign) {
    const TypeIntegral* unsigned_[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    const TypeIntegral* signed_[] = { byteTy, shortTy, intTy, longTy };

    for (int u = 0; u < 4; u++) {
        // Same size: no common type
        EXPECT_EQ(signed_[u]->common(unsigned_[u]), nullptr);
        EXPECT_EQ(unsigned_[u]->common(signed_[u]), nullptr);

        // Unsigned -> larger signed: common is the signed type
        for (int s = 0; s < 4; s++) {
            if (signed_[s]->getBytes() > unsigned_[u]->getBytes()) {
                EXPECT_EQ(unsigned_[u]->common(signed_[s]), signed_[s]);
                EXPECT_EQ(signed_[s]->common(unsigned_[u]), signed_[s]);
            }
        }
    }
}

// =============================================================================
// Common type: incompatible
// =============================================================================

TEST_F(TypeComparisonTests, CommonIncompatible) {
    // Integer <-> float
    EXPECT_EQ(intTy->common(singleTy), nullptr);
    EXPECT_EQ(doubleTy->common(longTy), nullptr);

    // Different families
    EXPECT_EQ(boolTy->common(intTy), nullptr);
    EXPECT_EQ(voidTy->common(intTy), nullptr);
    EXPECT_EQ(zstringTy->common(intTy), nullptr);

    // Pointers: only identical pointers share a common type
    const auto* intPtr = tf.getPointer(intTy);
    const auto* bytePtr = tf.getPointer(byteTy);
    EXPECT_EQ(intPtr->common(intPtr), intPtr);
    EXPECT_EQ(intPtr->common(bytePtr), nullptr);
}
