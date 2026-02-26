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

    using C = Type::Conversion;

    void expectImplicit(const Type* to, const Type* from) {
        EXPECT_TRUE(to->convertible(from, C::Implicit)) << "expected implicitly convertible";
    }

    void expectNotImplicit(const Type* to, const Type* from) {
        EXPECT_FALSE(to->convertible(from, C::Implicit)) << "expected not implicitly convertible";
    }

    void expectCast(const Type* to, const Type* from) {
        EXPECT_TRUE(to->convertible(from, C::Cast)) << "expected cast convertible";
    }

    void expectNotCast(const Type* to, const Type* from) {
        EXPECT_FALSE(to->convertible(from, C::Cast)) << "expected not cast convertible";
    }
};

// =============================================================================
// Identity
// =============================================================================

TEST_F(TypeComparisonTests, IdenticalTypes) {
    const auto* intPtr = tf.getPointer(intTy);
    const Type* types[] = {
        voidTy, nullTy, anyTy, boolTy, zstringTy,
        byteTy, shortTy, intTy, longTy,
        ubyteTy, ushortTy, uintTy, ulongTy,
        singleTy, doubleTy,
        intPtr
    };
    for (const auto* ty : types) {
        EXPECT_TRUE(ty->convertible(ty, C::Implicit));
        EXPECT_TRUE(ty->convertible(ty, C::Cast));
    }
}

// =============================================================================
// Signed integral: widening accepted, narrowing rejected (implicit)
// =============================================================================

TEST_F(TypeComparisonTests, SignedIntegralImplicit) {
    const TypeIntegral* chain[] = { byteTy, shortTy, intTy, longTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            expectImplicit(chain[j], chain[i]);
            expectNotImplicit(chain[i], chain[j]);
        }
    }
}

// =============================================================================
// Unsigned integral: widening accepted, narrowing rejected (implicit)
// =============================================================================

TEST_F(TypeComparisonTests, UnsignedIntegralImplicit) {
    const TypeIntegral* chain[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            expectImplicit(chain[j], chain[i]);
            expectNotImplicit(chain[i], chain[j]);
        }
    }
}

// =============================================================================
// Floating-point: SINGLE -> DOUBLE accepted, reverse rejected (implicit)
// =============================================================================

TEST_F(TypeComparisonTests, FloatingPointImplicit) {
    expectImplicit(doubleTy, singleTy);
    expectNotImplicit(singleTy, doubleTy);
}

// =============================================================================
// Cross-sign implicit: unsigned -> larger signed ok, everything else rejected
// =============================================================================

TEST_F(TypeComparisonTests, CrossSignImplicit) {
    const TypeIntegral* unsigned_[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    const TypeIntegral* signed_[] = { byteTy, shortTy, intTy, longTy };

    for (int u = 0; u < 4; u++) {
        for (int s = 0; s < 4; s++) {
            // signed -> unsigned: always rejected
            expectNotImplicit(unsigned_[u], signed_[s]);

            // unsigned -> signed: only if signed is strictly larger
            if (signed_[s]->getBytes() > unsigned_[u]->getBytes()) {
                expectImplicit(signed_[s], unsigned_[u]);
            } else {
                expectNotImplicit(signed_[s], unsigned_[u]);
            }
        }
    }
}

// =============================================================================
// Integer <-> floating-point: rejected implicitly, allowed by cast
// =============================================================================

TEST_F(TypeComparisonTests, IntegerFloatImplicit) {
    const Type* integrals[] = { byteTy, intTy, longTy, ubyteTy, uintTy };
    const Type* floats[] = { singleTy, doubleTy };

    for (const auto* i : integrals) {
        for (const auto* f : floats) {
            expectNotImplicit(f, i);
            expectNotImplicit(i, f);
        }
    }
}

TEST_F(TypeComparisonTests, IntegerFloatCast) {
    const Type* integrals[] = { byteTy, intTy, longTy, ubyteTy, uintTy };
    const Type* floats[] = { singleTy, doubleTy };

    for (const auto* i : integrals) {
        for (const auto* f : floats) {
            expectCast(f, i);
            expectCast(i, f);
        }
    }
}

// =============================================================================
// Cast: numeric conversions (narrowing, cross-sign)
// =============================================================================

TEST_F(TypeComparisonTests, NumericCast) {
    // narrowing is allowed
    expectCast(byteTy, longTy);
    expectCast(singleTy, doubleTy);

    // cross-sign is allowed
    expectCast(ubyteTy, byteTy);
    expectCast(byteTy, ubyteTy);
    expectCast(uintTy, longTy);
    expectCast(longTy, ulongTy);
}

// =============================================================================
// Incompatible type families (bool, void, zstring vs numerics)
// =============================================================================

TEST_F(TypeComparisonTests, IncompatibleFamilies) {
    const Type* isolated[] = { boolTy, voidTy, zstringTy };
    const Type* numerics[] = { byteTy, intTy, doubleTy };

    for (const auto* iso : isolated) {
        for (const auto* num : numerics) {
            expectNotImplicit(iso, num);
            expectNotImplicit(num, iso);
            expectNotCast(iso, num);
            expectNotCast(num, iso);
        }
    }
    expectNotImplicit(voidTy, nullTy);
    expectNotImplicit(nullTy, voidTy);
}

// =============================================================================
// Pointer conversions (implicit)
// =============================================================================

TEST_F(TypeComparisonTests, PointerImplicit) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* bytePtr = tf.getPointer(byteTy);
    const auto* anyPtr = tf.getAnyPtr();
    const auto* intPtrPtr = tf.getPointer(intPtr);

    // null -> any pointer type
    expectImplicit(intPtr, nullTy);
    expectImplicit(anyPtr, nullTy);
    expectNotImplicit(intTy, nullTy);

    // any pointer -> AnyPtr
    expectImplicit(anyPtr, intPtr);
    expectImplicit(anyPtr, bytePtr);
    expectNotImplicit(anyPtr, intTy);

    // mismatched pointee types
    expectNotImplicit(intPtr, bytePtr);
    expectNotImplicit(bytePtr, intPtr);

    // pointer vs non-pointer
    expectNotImplicit(intPtr, intTy);
    expectNotImplicit(intTy, intPtr);

    // nested pointer mismatch
    expectNotImplicit(intPtrPtr, intPtr);
    expectNotImplicit(intPtr, intPtrPtr);
}

// =============================================================================
// Pointer conversions (cast)
// =============================================================================

TEST_F(TypeComparisonTests, PointerCast) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* bytePtr = tf.getPointer(byteTy);
    const auto* anyPtr = tf.getAnyPtr();

    // null -> pointer
    expectCast(intPtr, nullTy);
    expectCast(anyPtr, nullTy);

    // pointer -> pointer (any direction)
    expectCast(intPtr, bytePtr);
    expectCast(bytePtr, intPtr);
    expectCast(anyPtr, intPtr);
    expectCast(intPtr, anyPtr);

    // pointer vs non-pointer: rejected
    expectNotCast(intPtr, intTy);
    expectNotCast(intTy, intPtr);
}

// =============================================================================
// Function type comparisons
// =============================================================================

TEST_F(TypeComparisonTests, FunctionTypeComparisons) {
    std::array<const Type*, 2> params { intTy, byteTy };
    const auto* fn = tf.getFunction(params, voidTy);

    // identity
    EXPECT_TRUE(fn->convertible(fn, C::Implicit));

    // different return type
    const auto* fnInt = tf.getFunction({}, intTy);
    const auto* fnVoid = tf.getFunction({}, voidTy);
    expectNotImplicit(fnInt, fnVoid);

    // different param types
    std::array<const Type*, 1> p1 { intTy };
    std::array<const Type*, 1> p2 { byteTy };
    expectNotImplicit(tf.getFunction(p1, voidTy), tf.getFunction(p2, voidTy));

    // different param count
    std::array<const Type*, 2> p3 { intTy, intTy };
    expectNotImplicit(tf.getFunction(p1, voidTy), tf.getFunction(p3, voidTy));

    // function vs non-function
    expectNotImplicit(fn, intTy);
    expectNotImplicit(intTy, fn);
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
