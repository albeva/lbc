#include "pch.hpp"
#include <gtest/gtest.h>
#include "Driver/Context.hpp"
#include "Type/Aggregate.hpp"
#include "Type/Compound.hpp"
#include "Type/Numeric.hpp"
using namespace lbc;

// ------------------------------------
// Singleton / basic types
// ------------------------------------

TEST(TypeFactoryTests, SentinelTypes) {
    Context context;
    auto& tf = context.getTypeFactory();

    EXPECT_TRUE(tf.getVoid()->isVoid());
    EXPECT_TRUE(tf.getNull()->isNull());
    EXPECT_TRUE(tf.getAny()->isAny());

    EXPECT_TRUE(tf.getVoid()->isSentinel());
    EXPECT_TRUE(tf.getNull()->isSentinel());
    EXPECT_TRUE(tf.getAny()->isSentinel());
}

TEST(TypeFactoryTests, PrimitiveTypes) {
    Context context;
    auto& tf = context.getTypeFactory();

    EXPECT_TRUE(tf.getBool()->isBool());
    EXPECT_TRUE(tf.getZString()->isZString());
    EXPECT_TRUE(tf.getBool()->isPrimitive());
    EXPECT_TRUE(tf.getZString()->isPrimitive());
}

TEST(TypeFactoryTests, IntegralTypes) {
    Context context;
    auto& tf = context.getTypeFactory();

    // Signed
    EXPECT_TRUE(tf.getByte()->isSigned());
    EXPECT_TRUE(tf.getShort()->isSigned());
    EXPECT_TRUE(tf.getInteger()->isSigned());
    EXPECT_TRUE(tf.getLong()->isSigned());

    // Unsigned
    EXPECT_FALSE(tf.getUByte()->isSigned());
    EXPECT_FALSE(tf.getUShort()->isSigned());
    EXPECT_FALSE(tf.getUInteger()->isSigned());
    EXPECT_FALSE(tf.getULong()->isSigned());

    // Sizes
    EXPECT_EQ(tf.getByte()->getBytes(), 1);
    EXPECT_EQ(tf.getShort()->getBytes(), 2);
    EXPECT_EQ(tf.getInteger()->getBytes(), 4);
    EXPECT_EQ(tf.getLong()->getBytes(), 8);
    EXPECT_EQ(tf.getUByte()->getBytes(), 1);
    EXPECT_EQ(tf.getUShort()->getBytes(), 2);
    EXPECT_EQ(tf.getUInteger()->getBytes(), 4);
    EXPECT_EQ(tf.getULong()->getBytes(), 8);
}

TEST(TypeFactoryTests, FloatingPointTypes) {
    Context context;
    auto& tf = context.getTypeFactory();

    EXPECT_TRUE(tf.getSingle()->isFloatingPoint());
    EXPECT_TRUE(tf.getDouble()->isFloatingPoint());
    EXPECT_EQ(tf.getSingle()->getBytes(), sizeof(float));
    EXPECT_EQ(tf.getDouble()->getBytes(), sizeof(double));
}

TEST(TypeFactoryTests, SingletonStability) {
    Context context;
    auto& tf = context.getTypeFactory();

    EXPECT_EQ(tf.getInteger(), tf.getInteger());
    EXPECT_EQ(tf.getVoid(), tf.getVoid());
    EXPECT_EQ(tf.getBool(), tf.getBool());
    EXPECT_EQ(tf.getDouble(), tf.getDouble());
}

// ------------------------------------
// Compound types
// ------------------------------------

TEST(TypeFactoryTests, PointerType) {
    Context context;
    auto& tf = context.getTypeFactory();

    const auto* intPtr = tf.getPointer(tf.getInteger());
    EXPECT_TRUE(intPtr->isPointer());
    EXPECT_EQ(intPtr->getBaseType(), tf.getInteger());
}

TEST(TypeFactoryTests, PointerStability) {
    Context context;
    auto& tf = context.getTypeFactory();

    const auto* p1 = tf.getPointer(tf.getInteger());
    const auto* p2 = tf.getPointer(tf.getInteger());
    EXPECT_EQ(p1, p2);
}

TEST(TypeFactoryTests, DistinctPointerTypes) {
    Context context;
    auto& tf = context.getTypeFactory();

    EXPECT_NE(tf.getPointer(tf.getInteger()), tf.getPointer(tf.getBool()));
}

TEST(TypeFactoryTests, AnyPtr) {
    Context context;
    auto& tf = context.getTypeFactory();

    const auto* anyPtr = tf.getAnyPtr();
    EXPECT_TRUE(anyPtr->isPointer());
    EXPECT_EQ(anyPtr->getBaseType(), tf.getAny());
    EXPECT_EQ(anyPtr, tf.getPointer(tf.getAny()));
}

TEST(TypeFactoryTests, ReferenceType) {
    Context context;
    auto& tf = context.getTypeFactory();

    const auto* intRef = tf.getReference(tf.getInteger());
    EXPECT_TRUE(intRef->isReference());
    EXPECT_EQ(intRef->getBaseType(), tf.getInteger());
}

TEST(TypeFactoryTests, ReferenceStability) {
    Context context;
    auto& tf = context.getTypeFactory();

    EXPECT_EQ(tf.getReference(tf.getInteger()), tf.getReference(tf.getInteger()));
}

// ------------------------------------
// Function types
// ------------------------------------

TEST(TypeFactoryTests, FunctionType) {
    Context context;
    auto& tf = context.getTypeFactory();

    std::array<const Type*, 2> params { tf.getInteger(), tf.getBool() };
    const auto* fn = tf.getFunction(params, tf.getVoid());

    EXPECT_TRUE(fn->isFunction());
    EXPECT_EQ(fn->getReturnType(), tf.getVoid());
    ASSERT_EQ(fn->getParams().size(), 2);
    EXPECT_EQ(fn->getParams()[0], tf.getInteger());
    EXPECT_EQ(fn->getParams()[1], tf.getBool());
}

TEST(TypeFactoryTests, FunctionNoParams) {
    Context context;
    auto& tf = context.getTypeFactory();

    const auto* fn = tf.getFunction({}, tf.getInteger());
    EXPECT_TRUE(fn->isFunction());
    EXPECT_EQ(fn->getReturnType(), tf.getInteger());
    EXPECT_TRUE(fn->getParams().empty());
}

TEST(TypeFactoryTests, FunctionStability) {
    Context context;
    auto& tf = context.getTypeFactory();

    std::array<const Type*, 2> params { tf.getInteger(), tf.getBool() };
    const auto* fn1 = tf.getFunction(params, tf.getVoid());
    const auto* fn2 = tf.getFunction(params, tf.getVoid());
    EXPECT_EQ(fn1, fn2);
}

TEST(TypeFactoryTests, FunctionDistinctReturnType) {
    Context context;
    auto& tf = context.getTypeFactory();

    std::array<const Type*, 1> params { tf.getInteger() };
    EXPECT_NE(tf.getFunction(params, tf.getVoid()), tf.getFunction(params, tf.getBool()));
}

TEST(TypeFactoryTests, FunctionDistinctParams) {
    Context context;
    auto& tf = context.getTypeFactory();

    std::array<const Type*, 1> p1 { tf.getInteger() };
    std::array<const Type*, 1> p2 { tf.getBool() };
    EXPECT_NE(tf.getFunction(p1, tf.getVoid()), tf.getFunction(p2, tf.getVoid()));
}
