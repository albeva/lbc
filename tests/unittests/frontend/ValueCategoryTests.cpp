//
// Created by Albert Varaksin on 15/06/2026.
//
#include "pch.hpp"
#include <gtest/gtest.h>
#include "Ast/ValueCategory.hpp"
using namespace lbc;

// Pure unit tests for the ValueCategory smart enum. These pin the value-category
// lattice — in particular hasIdentity() and isMovable() are range checks that
// depend on the Kind enum's ordering, so reordering it must fail these tests
// rather than break callers silently.

TEST(ValueCategoryTests, AddressableLattice) {
    constexpr ValueCategory cat = ValueCategory::Addressable; // C++ lvalue
    EXPECT_TRUE(cat.isAddressable());
    EXPECT_FALSE(cat.isExpiring());
    EXPECT_FALSE(cat.isValue());
    EXPECT_TRUE(cat.hasIdentity()); // glvalue
    EXPECT_FALSE(cat.isMovable());  // not an rvalue
}

TEST(ValueCategoryTests, ExpiringLattice) {
    constexpr ValueCategory cat = ValueCategory::Expiring; // C++ xvalue
    EXPECT_FALSE(cat.isAddressable());
    EXPECT_TRUE(cat.isExpiring());
    EXPECT_FALSE(cat.isValue());
    EXPECT_TRUE(cat.hasIdentity()); // glvalue
    EXPECT_TRUE(cat.isMovable());   // rvalue
}

TEST(ValueCategoryTests, ValueLattice) {
    constexpr ValueCategory cat = ValueCategory::Value; // C++ prvalue
    EXPECT_FALSE(cat.isAddressable());
    EXPECT_FALSE(cat.isExpiring());
    EXPECT_TRUE(cat.isValue());
    EXPECT_FALSE(cat.hasIdentity()); // not a glvalue
    EXPECT_TRUE(cat.isMovable());    // rvalue
}

TEST(ValueCategoryTests, DefaultsToValue) {
    constexpr ValueCategory cat {};
    EXPECT_TRUE(cat.isValue());
}

TEST(ValueCategoryTests, Equality) {
    constexpr ValueCategory cat = ValueCategory::Addressable;
    EXPECT_TRUE(cat == ValueCategory::Addressable);
    EXPECT_FALSE(cat == ValueCategory::Value);
    EXPECT_TRUE(cat.kind() == ValueCategory::Addressable);
}

TEST(ValueCategoryTests, StringNames) {
    EXPECT_EQ((ValueCategory { ValueCategory::Addressable }).string(), "addressable");
    EXPECT_EQ((ValueCategory { ValueCategory::Expiring }).string(), "expiring");
    EXPECT_EQ((ValueCategory { ValueCategory::Value }).string(), "value");
}
