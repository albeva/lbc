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
    const Type* types[] = {
        voidTy, nullTy, anyTy, boolTy, zstringTy,
        byteTy, shortTy, intTy, longTy,
        ubyteTy, ushortTy, uintTy, ulongTy,
        singleTy, doubleTy
    };
    for (const auto* ty : types) {
        EXPECT_EQ(ty->compare(ty).result, R::Identical);
    }
}

TEST_F(TypeComparisonTests, PointerAndReferenceIdentity) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* intRef = tf.getReference(intTy);
    EXPECT_EQ(intPtr->compare(intPtr).result, R::Identical);
    EXPECT_EQ(intRef->compare(intRef).result, R::Identical);
}

// =============================================================================
// Signed integral widening: BYTE -> SHORT -> INTEGER -> LONG
// =============================================================================

TEST_F(TypeComparisonTests, SignedWidening) {
    const TypeIntegral* chain[] = { byteTy, shortTy, intTy, longTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            expectConvertible(chain[j], chain[i], F::Added, F::Unchanged);
        }
    }
}

// =============================================================================
// Unsigned integral widening: UBYTE -> USHORT -> UINTEGER -> ULONG
// =============================================================================

TEST_F(TypeComparisonTests, UnsignedWidening) {
    const TypeIntegral* chain[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            expectConvertible(chain[j], chain[i], F::Added, F::Unchanged);
        }
    }
}

// =============================================================================
// Floating-point widening: SINGLE -> DOUBLE
// =============================================================================

TEST_F(TypeComparisonTests, FloatWidening) {
    expectConvertible(doubleTy, singleTy, F::Added, F::Unchanged);
}

// =============================================================================
// Safe sign change: unsigned -> larger signed
// =============================================================================

TEST_F(TypeComparisonTests, UnsignedToLargerSigned) {
    const TypeIntegral* unsigned_[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    const TypeIntegral* signed_[] = { byteTy, shortTy, intTy, longTy };

    // Each unsigned type can convert to any signed type with more bytes
    for (int u = 0; u < 4; u++) {
        for (int s = 0; s < 4; s++) {
            if (signed_[s]->getBytes() > unsigned_[u]->getBytes()) {
                expectConvertible(signed_[s], unsigned_[u], F::Added, F::Added);
            }
        }
    }
}

// =============================================================================
// Narrowing (REJECTED): larger -> smaller, same family
// =============================================================================

TEST_F(TypeComparisonTests, SignedNarrowing) {
    const TypeIntegral* chain[] = { byteTy, shortTy, intTy, longTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            expectIncompatible(chain[i], chain[j]);
        }
    }
}

TEST_F(TypeComparisonTests, UnsignedNarrowing) {
    const TypeIntegral* chain[] = { ubyteTy, ushortTy, uintTy, ulongTy };
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            expectIncompatible(chain[i], chain[j]);
        }
    }
}

TEST_F(TypeComparisonTests, FloatNarrowing) {
    expectIncompatible(singleTy, doubleTy);
}

// =============================================================================
// Signed -> unsigned (REJECTED): always illegal regardless of size
// =============================================================================

TEST_F(TypeComparisonTests, SignedToUnsigned) {
    const TypeIntegral* signed_[] = { byteTy, shortTy, intTy, longTy };
    const TypeIntegral* unsigned_[] = { ubyteTy, ushortTy, uintTy, ulongTy };

    for (const auto* s : signed_) {
        for (const auto* u : unsigned_) {
            expectIncompatible(u, s);
        }
    }
}

// =============================================================================
// Unsigned -> signed same size (REJECTED)
// =============================================================================

TEST_F(TypeComparisonTests, UnsignedToSignedSameSize) {
    expectIncompatible(byteTy, ubyteTy);
    expectIncompatible(shortTy, ushortTy);
    expectIncompatible(intTy, uintTy);
    expectIncompatible(longTy, ulongTy);
}

// =============================================================================
// Integer <-> floating-point (REJECTED)
// =============================================================================

TEST_F(TypeComparisonTests, IntegerToFloat) {
    const Type* integrals[] = { byteTy, shortTy, intTy, longTy, ubyteTy, ushortTy, uintTy, ulongTy };
    const Type* floats[] = { singleTy, doubleTy };

    for (const auto* i : integrals) {
        for (const auto* f : floats) {
            expectIncompatible(f, i);
            expectIncompatible(i, f);
        }
    }
}

// =============================================================================
// Incompatible type families
// =============================================================================

TEST_F(TypeComparisonTests, BoolIncompatibleWithNumeric) {
    const Type* numerics[] = { byteTy, intTy, longTy, ubyteTy, uintTy, singleTy, doubleTy };
    for (const auto* n : numerics) {
        expectIncompatible(n, boolTy);
        expectIncompatible(boolTy, n);
    }
}

TEST_F(TypeComparisonTests, VoidIncompatibleWithEverything) {
    const Type* types[] = { boolTy, byteTy, intTy, doubleTy, nullTy };
    for (const auto* ty : types) {
        expectIncompatible(voidTy, ty);
        expectIncompatible(ty, voidTy);
    }
}

TEST_F(TypeComparisonTests, ZStringIncompatibleWithNumeric) {
    const Type* numerics[] = { byteTy, intTy, doubleTy };
    for (const auto* n : numerics) {
        expectIncompatible(zstringTy, n);
        expectIncompatible(n, zstringTy);
    }
}

// =============================================================================
// Pointer conversions
// =============================================================================

TEST_F(TypeComparisonTests, PointerFromNull) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* bytePtr = tf.getPointer(byteTy);
    const auto* anyPtr = tf.getAnyPtr();
    EXPECT_EQ(intPtr->compare(nullTy).result, R::Convertible);
    EXPECT_EQ(bytePtr->compare(nullTy).result, R::Convertible);
    EXPECT_EQ(anyPtr->compare(nullTy).result, R::Convertible);
}

TEST_F(TypeComparisonTests, NullToNonPointer) {
    expectIncompatible(intTy, nullTy);
    expectIncompatible(doubleTy, nullTy);
    expectIncompatible(boolTy, nullTy);
}

TEST_F(TypeComparisonTests, AnyPtrFromAnyPointer) {
    const auto* anyPtr = tf.getAnyPtr();
    const auto* intPtr = tf.getPointer(intTy);
    const auto* bytePtr = tf.getPointer(byteTy);
    EXPECT_EQ(anyPtr->compare(intPtr).result, R::Convertible);
    EXPECT_EQ(anyPtr->compare(bytePtr).result, R::Convertible);
}

TEST_F(TypeComparisonTests, AnyPtrFromNonPointer) {
    const auto* anyPtr = tf.getAnyPtr();
    expectIncompatible(anyPtr, intTy);
    expectIncompatible(anyPtr, boolTy);
}

TEST_F(TypeComparisonTests, PointerMismatch) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* bytePtr = tf.getPointer(byteTy);
    expectIncompatible(intPtr, bytePtr);
    expectIncompatible(bytePtr, intPtr);
}

TEST_F(TypeComparisonTests, PointerToNonPointerIncompatible) {
    const auto* intPtr = tf.getPointer(intTy);
    expectIncompatible(intPtr, intTy);
    expectIncompatible(intTy, intPtr);
}

TEST_F(TypeComparisonTests, NestedPointers) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* intPtrPtr = tf.getPointer(intPtr);
    EXPECT_EQ(intPtrPtr->compare(intPtrPtr).result, R::Identical);
    expectIncompatible(intPtrPtr, intPtr);
    expectIncompatible(intPtr, intPtrPtr);
}

// =============================================================================
// Reference conversions
// =============================================================================

TEST_F(TypeComparisonTests, ReferenceFromValue) {
    const auto* intRef = tf.getReference(intTy);
    auto res = intRef->compare(intTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.reference, F::Added);
}

TEST_F(TypeComparisonTests, ValueFromReference) {
    const auto* intRef = tf.getReference(intTy);
    auto res = intTy->compare(intRef);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.reference, F::Removed);
}

TEST_F(TypeComparisonTests, ReferenceWithWidening) {
    const auto* shortRef = tf.getReference(shortTy);
    auto res = shortRef->compare(byteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.reference, F::Added);
    EXPECT_EQ(res.size, F::Added);
}

TEST_F(TypeComparisonTests, ValueFromReferenceWithWidening) {
    const auto* refByte = tf.getReference(byteTy);
    auto res = intTy->compare(refByte);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.reference, F::Removed);
}

TEST_F(TypeComparisonTests, ReferenceMismatch) {
    const auto* intRef = tf.getReference(intTy);
    const auto* dblRef = tf.getReference(doubleTy);
    expectIncompatible(intRef, dblRef);
    expectIncompatible(dblRef, intRef);
}

// =============================================================================
// Function type comparisons
// =============================================================================

TEST_F(TypeComparisonTests, FunctionIdentical) {
    std::array<const Type*, 2> params { intTy, byteTy };
    const auto* fn = tf.getFunction(params, voidTy);
    EXPECT_EQ(fn->compare(fn).result, R::Identical);
}

TEST_F(TypeComparisonTests, FunctionDifferentReturnType) {
    const auto* fn1 = tf.getFunction({}, intTy);
    const auto* fn2 = tf.getFunction({}, voidTy);
    expectIncompatible(fn1, fn2);
    expectIncompatible(fn2, fn1);
}

TEST_F(TypeComparisonTests, FunctionDifferentParams) {
    std::array<const Type*, 1> p1 { intTy };
    std::array<const Type*, 1> p2 { byteTy };
    const auto* fn1 = tf.getFunction(p1, voidTy);
    const auto* fn2 = tf.getFunction(p2, voidTy);
    expectIncompatible(fn1, fn2);
    expectIncompatible(fn2, fn1);
}

TEST_F(TypeComparisonTests, FunctionDifferentParamCount) {
    std::array<const Type*, 1> p1 { intTy };
    std::array<const Type*, 2> p2 { intTy, intTy };
    const auto* fn1 = tf.getFunction(p1, voidTy);
    const auto* fn2 = tf.getFunction(p2, voidTy);
    expectIncompatible(fn1, fn2);
    expectIncompatible(fn2, fn1);
}

TEST_F(TypeComparisonTests, FunctionIncompatibleWithNonFunction) {
    const auto* fn = tf.getFunction({}, voidTy);
    expectIncompatible(fn, intTy);
    expectIncompatible(intTy, fn);
    expectIncompatible(fn, tf.getPointer(intTy));
}
