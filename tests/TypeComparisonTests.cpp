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

    // Convenience aliases
    const Type* voidTy = tf.getVoid();
    const Type* nullTy = tf.getNull();
    const Type* boolTy = tf.getBool();

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
};

// =============================================================================
// Identity
// =============================================================================

TEST_F(TypeComparisonTests, IdenticalTypes) {
    EXPECT_EQ(byteTy->compare(byteTy).result, R::Identical);
    EXPECT_EQ(shortTy->compare(shortTy).result, R::Identical);
    EXPECT_EQ(intTy->compare(intTy).result, R::Identical);
    EXPECT_EQ(longTy->compare(longTy).result, R::Identical);
    EXPECT_EQ(ubyteTy->compare(ubyteTy).result, R::Identical);
    EXPECT_EQ(ushortTy->compare(ushortTy).result, R::Identical);
    EXPECT_EQ(uintTy->compare(uintTy).result, R::Identical);
    EXPECT_EQ(ulongTy->compare(ulongTy).result, R::Identical);
    EXPECT_EQ(singleTy->compare(singleTy).result, R::Identical);
    EXPECT_EQ(doubleTy->compare(doubleTy).result, R::Identical);
    EXPECT_EQ(boolTy->compare(boolTy).result, R::Identical);
}

// =============================================================================
// Signed integral widening: BYTE -> SHORT -> INTEGER -> LONG
// =============================================================================

TEST_F(TypeComparisonTests, SignedWideningByteToShort) {
    auto res = shortTy->compare(byteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, SignedWideningByteToInteger) {
    auto res = intTy->compare(byteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, SignedWideningByteToLong) {
    auto res = longTy->compare(byteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, SignedWideningShortToInteger) {
    auto res = intTy->compare(shortTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, SignedWideningShortToLong) {
    auto res = longTy->compare(shortTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, SignedWideningIntegerToLong) {
    auto res = longTy->compare(intTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

// =============================================================================
// Unsigned integral widening: UBYTE -> USHORT -> UINTEGER -> ULONG
// =============================================================================

TEST_F(TypeComparisonTests, UnsignedWideningUByteToUShort) {
    auto res = ushortTy->compare(ubyteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, UnsignedWideningUByteToUInteger) {
    auto res = uintTy->compare(ubyteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, UnsignedWideningUByteToULong) {
    auto res = ulongTy->compare(ubyteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, UnsignedWideningUShortToUInteger) {
    auto res = uintTy->compare(ushortTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, UnsignedWideningUShortToULong) {
    auto res = ulongTy->compare(ushortTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

TEST_F(TypeComparisonTests, UnsignedWideningUIntegerToULong) {
    auto res = ulongTy->compare(uintTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Unchanged);
}

// =============================================================================
// Floating-point widening: SINGLE -> DOUBLE
// =============================================================================

TEST_F(TypeComparisonTests, FloatWideningSingleToDouble) {
    auto res = doubleTy->compare(singleTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
}

// =============================================================================
// Safe sign change: unsigned -> larger signed
// =============================================================================

TEST_F(TypeComparisonTests, UByteToShort) {
    auto res = shortTy->compare(ubyteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Added);
}

TEST_F(TypeComparisonTests, UByteToInteger) {
    auto res = intTy->compare(ubyteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Added);
}

TEST_F(TypeComparisonTests, UByteToLong) {
    auto res = longTy->compare(ubyteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Added);
}

TEST_F(TypeComparisonTests, UShortToInteger) {
    auto res = intTy->compare(ushortTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Added);
}

TEST_F(TypeComparisonTests, UShortToLong) {
    auto res = longTy->compare(ushortTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Added);
}

TEST_F(TypeComparisonTests, UIntegerToLong) {
    auto res = longTy->compare(uintTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.sign, F::Added);
}

// =============================================================================
// Narrowing (REJECTED): larger -> smaller
// =============================================================================

TEST_F(TypeComparisonTests, NarrowingShortToByte) {
    EXPECT_EQ(byteTy->compare(shortTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NarrowingIntegerToByte) {
    EXPECT_EQ(byteTy->compare(intTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NarrowingIntegerToShort) {
    EXPECT_EQ(shortTy->compare(intTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NarrowingLongToInteger) {
    EXPECT_EQ(intTy->compare(longTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NarrowingLongToByte) {
    EXPECT_EQ(byteTy->compare(longTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NarrowingUShortToUByte) {
    EXPECT_EQ(ubyteTy->compare(ushortTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NarrowingUIntegerToUShort) {
    EXPECT_EQ(ushortTy->compare(uintTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NarrowingULongToUInteger) {
    EXPECT_EQ(uintTy->compare(ulongTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NarrowingDoubleToSingle) {
    EXPECT_EQ(singleTy->compare(doubleTy).result, R::Incompatible);
}

// =============================================================================
// Signed -> unsigned (REJECTED): always illegal regardless of size
// =============================================================================

TEST_F(TypeComparisonTests, SignedToUnsignedSameSize) {
    EXPECT_EQ(ubyteTy->compare(byteTy).result, R::Incompatible);
    EXPECT_EQ(ushortTy->compare(shortTy).result, R::Incompatible);
    EXPECT_EQ(uintTy->compare(intTy).result, R::Incompatible);
    EXPECT_EQ(ulongTy->compare(longTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, SignedToUnsignedLarger) {
    EXPECT_EQ(ushortTy->compare(byteTy).result, R::Incompatible);
    EXPECT_EQ(uintTy->compare(shortTy).result, R::Incompatible);
    EXPECT_EQ(ulongTy->compare(intTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, SignedToUnsignedSmaller) {
    EXPECT_EQ(ubyteTy->compare(shortTy).result, R::Incompatible);
    EXPECT_EQ(ubyteTy->compare(intTy).result, R::Incompatible);
    EXPECT_EQ(ushortTy->compare(intTy).result, R::Incompatible);
}

// =============================================================================
// Unsigned -> signed same size (REJECTED): UBYTE -> BYTE, etc.
// =============================================================================

TEST_F(TypeComparisonTests, UnsignedToSignedSameSize) {
    EXPECT_EQ(byteTy->compare(ubyteTy).result, R::Incompatible);
    EXPECT_EQ(shortTy->compare(ushortTy).result, R::Incompatible);
    EXPECT_EQ(intTy->compare(uintTy).result, R::Incompatible);
    EXPECT_EQ(longTy->compare(ulongTy).result, R::Incompatible);
}

// =============================================================================
// Integer <-> floating-point (REJECTED)
// =============================================================================

TEST_F(TypeComparisonTests, IntegerToFloat) {
    EXPECT_EQ(singleTy->compare(byteTy).result, R::Incompatible);
    EXPECT_EQ(singleTy->compare(intTy).result, R::Incompatible);
    EXPECT_EQ(doubleTy->compare(intTy).result, R::Incompatible);
    EXPECT_EQ(doubleTy->compare(longTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, FloatToInteger) {
    EXPECT_EQ(byteTy->compare(singleTy).result, R::Incompatible);
    EXPECT_EQ(intTy->compare(singleTy).result, R::Incompatible);
    EXPECT_EQ(intTy->compare(doubleTy).result, R::Incompatible);
    EXPECT_EQ(longTy->compare(doubleTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, UnsignedIntegerToFloat) {
    EXPECT_EQ(singleTy->compare(ubyteTy).result, R::Incompatible);
    EXPECT_EQ(doubleTy->compare(uintTy).result, R::Incompatible);
}

// =============================================================================
// Incompatible type families
// =============================================================================

TEST_F(TypeComparisonTests, BoolToNumeric) {
    EXPECT_EQ(intTy->compare(boolTy).result, R::Incompatible);
    EXPECT_EQ(doubleTy->compare(boolTy).result, R::Incompatible);
    EXPECT_EQ(byteTy->compare(boolTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NumericToBool) {
    EXPECT_EQ(boolTy->compare(intTy).result, R::Incompatible);
    EXPECT_EQ(boolTy->compare(doubleTy).result, R::Incompatible);
    EXPECT_EQ(boolTy->compare(byteTy).result, R::Incompatible);
}

// =============================================================================
// Pointer conversions
// =============================================================================

TEST_F(TypeComparisonTests, PointerIdentical) {
    const auto* intPtr = tf.getPointer(intTy);
    EXPECT_EQ(intPtr->compare(intPtr).result, R::Identical);
}

TEST_F(TypeComparisonTests, PointerFromNull) {
    const auto* intPtr = tf.getPointer(intTy);
    auto res = intPtr->compare(nullTy);
    EXPECT_EQ(res.result, R::FromNullPtr);
}

TEST_F(TypeComparisonTests, PointerMismatch) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* bytePtr = tf.getPointer(byteTy);
    EXPECT_EQ(intPtr->compare(bytePtr).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, PointerToNonPointer) {
    const auto* intPtr = tf.getPointer(intTy);
    EXPECT_EQ(intPtr->compare(intTy).result, R::Incompatible);
}

TEST_F(TypeComparisonTests, NonPointerToPointer) {
    const auto* intPtr = tf.getPointer(intTy);
    EXPECT_EQ(intTy->compare(intPtr).result, R::Incompatible);
}

// =============================================================================
// Reference conversions
// =============================================================================

TEST_F(TypeComparisonTests, ReferenceIdentical) {
    const auto* intRef = tf.getReference(intTy);
    EXPECT_EQ(intRef->compare(intRef).result, R::Identical);
}

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

TEST_F(TypeComparisonTests, ReferenceWidening) {
    const auto* shortRef = tf.getReference(shortTy);
    auto res = shortRef->compare(byteTy);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.reference, F::Added);
    EXPECT_EQ(res.size, F::Added);
}

TEST_F(TypeComparisonTests, ReferenceMismatch) {
    const auto* intRef = tf.getReference(intTy);
    const auto* dblRef = tf.getReference(doubleTy);
    EXPECT_EQ(intRef->compare(dblRef).result, R::Incompatible);
}

// =============================================================================
// Const qualification
// =============================================================================

TEST_F(TypeComparisonTests, ConstFromNonConst) {
    const auto* constInt = tf.getConst(intTy);
    auto res = constInt->compare(intTy);
    EXPECT_EQ(res.result, R::Convertible);
}

TEST_F(TypeComparisonTests, ConstIdentical) {
    const auto* constInt = tf.getConst(intTy);
    EXPECT_EQ(constInt->compare(constInt).result, R::Identical);
}

TEST_F(TypeComparisonTests, ConstWithWidening) {
    const auto* constShort = tf.getConst(shortTy);
    auto res = constShort->compare(tf.getConst(byteTy));
    EXPECT_NE(res.result, R::Incompatible);
    EXPECT_EQ(res.size, F::Added);
}

// =============================================================================
// Stripping const/ref from source
// =============================================================================

TEST_F(TypeComparisonTests, IntegerFromConstByte) {
    const auto* constByte = tf.getConst(byteTy);
    auto res = intTy->compare(constByte);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
}

TEST_F(TypeComparisonTests, IntegerFromRefByte) {
    const auto* refByte = tf.getReference(byteTy);
    auto res = intTy->compare(refByte);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
    EXPECT_EQ(res.reference, F::Removed);
}

TEST_F(TypeComparisonTests, ShortFromConstRefByte) {
    const auto* constRefByte = tf.getConst(tf.getReference(byteTy));
    auto res = shortTy->compare(constRefByte);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.size, F::Added);
}

// =============================================================================
// Null pointer conversions
// =============================================================================

TEST_F(TypeComparisonTests, NullToAnyPointer) {
    const auto* bytePtr = tf.getPointer(byteTy);
    const auto* intPtr = tf.getPointer(intTy);
    const auto* anyPtr = tf.getAnyPtr();
    EXPECT_EQ(bytePtr->compare(nullTy).result, R::FromNullPtr);
    EXPECT_EQ(intPtr->compare(nullTy).result, R::FromNullPtr);
    EXPECT_EQ(anyPtr->compare(nullTy).result, R::FromNullPtr);
}

TEST_F(TypeComparisonTests, NullToNonPointer) {
    EXPECT_EQ(intTy->compare(nullTy).result, R::Incompatible);
    EXPECT_EQ(byteTy->compare(nullTy).result, R::Incompatible);
    EXPECT_EQ(doubleTy->compare(nullTy).result, R::Incompatible);
}

// =============================================================================
// Const pointer / reference safety
// =============================================================================

// dim cip as const integer ptr, ip as integer ptr = cip ' illegal: drops const from pointee
TEST_F(TypeComparisonTests, ConstPtrToNonConstPtr) {
    const auto* constIntPtr = tf.getPointer(tf.getConst(intTy));
    const auto* intPtr = tf.getPointer(intTy);
    EXPECT_EQ(intPtr->compare(constIntPtr).result, R::Incompatible);
}

// dim ip as integer ptr, cip as const integer ptr = ip ' legal: adds const to pointee
TEST_F(TypeComparisonTests, NonConstPtrToConstPtr) {
    const auto* intPtr = tf.getPointer(intTy);
    const auto* constIntPtr = tf.getPointer(tf.getConst(intTy));
    auto res = constIntPtr->compare(intPtr);
    EXPECT_EQ(res.result, R::Convertible);
    EXPECT_EQ(res.constness, F::Added);
}

// dim cir as const integer ref, ir as integer ref = cir ' illegal: drops const from referent
TEST_F(TypeComparisonTests, ConstRefToNonConstRef) {
    const auto* constIntRef = tf.getReference(tf.getConst(intTy));
    const auto* intRef = tf.getReference(intTy);
    EXPECT_EQ(intRef->compare(constIntRef).result, R::Incompatible);
}

// dim ir as integer ref, cir as const integer ref = ir ' legal: adds const to referent
TEST_F(TypeComparisonTests, NonConstRefToConstRef) {
    const auto* intRef = tf.getReference(intTy);
    const auto* constIntRef = tf.getReference(tf.getConst(intTy));
    auto res = constIntRef->compare(intRef);
    EXPECT_EQ(res.result, R::Convertible);
}
