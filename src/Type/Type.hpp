//
// Created by Albert Varaksin on 07/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Type.def.hpp"

namespace lbc {
// clang-format off
enum class TypeFamily : std::uint8_t {
    Void,           // Void, lack of typeExpr
    Any,            // any ptr, null
    Pointer,        // Ptr to another typeExpr
    Reference,      // Reference to another type
    Boolean,        // true / false
    Integral,       // signed / unsigned integer 8, 16, 32, ... bits
    FloatingPoint,  // single, double
    Function,       // Functions and SUBs and other user callable routines
    ZString,        // nil terminated string, byte ptr / char*
    UDT,            // User defined Type (C struct)
};
// clang-format on

// clang-format off
enum class TypeKind : std::uint8_t {
    #define TYPE(ID, ...) ID,
    ALL_TYPES(TYPE)
    #undef TYPE
    ComplexType
};
// clang-format on

class TypePointer;
class TypeReference;
class TypeNumeric;
class TypeBoolean;
class TypeIntegral;
class TypeFloatingPoint;
class TypeFunction;
class TypeZString;
class Context;
enum class TokenKind : std::uint8_t;

/**
 * Enum representing the result of a type comparison.
 */
enum class TypeComparison : std::uint8_t {
    Incompatible, ///< No viable conversion possible
    Equal,        ///< Types are same and require no conversion
    Downcast,     ///< Smaller to larger. E.g. integer to short
    Upcast        ///< Smaller to Larger. E.g. short to integer
};

/**
 * Enum representing type qualifiers.
 * These qualifiers can be used to specify additional properties of types.
 */
enum class TypeQualifier : std::uint8_t {
    None     = 0b00000000, ///< No qualifiers
    Const    = 0b00000001, ///< Constant type
    Volatile = 0b00000010, ///< Volatile type
    Mutable  = 0b00000011, ///< Mutable type
    All      = 0b11111111  ///< All qualifiers
};
MARK_AS_FLAGS_ENUM(TypeQualifier);

/**
 * Base typeExpr is root for all lb types
 *
 * It uses llvm custom rtti system
 */
class TypeRoot {
public:
    NO_COPY_AND_MOVE(TypeRoot)

    [[nodiscard]] constexpr auto getFamily() const -> TypeFamily { return m_family; }
    [[nodiscard]] constexpr auto getKind() const -> TypeKind { return m_kind; }

    [[nodiscard]] auto getLlvmType(Context& context) const -> llvm::Type* {
        if (m_llvmType == nullptr) {
            m_llvmType = genLlvmType(context);
        }
        return m_llvmType;
    }

    virtual constexpr ~TypeRoot() = default;

    [[nodiscard]] static auto fromTokenKind(TokenKind kind) -> const TypeRoot*;
    [[nodiscard]] virtual auto asString() const -> std::string = 0;

    [[nodiscard]] auto getPointer(Context& context) const -> const TypePointer*;
    [[nodiscard]] auto getReference(Context& context) const -> const TypeReference*;
    [[nodiscard]] constexpr virtual auto getBase() const -> const TypeRoot* { return this; }

    // Type queries
    [[nodiscard]] constexpr auto isVoid() const -> bool { return m_family == TypeFamily::Void; }
    [[nodiscard]] constexpr auto isAny() const -> bool { return m_family == TypeFamily::Any; }
    [[nodiscard]] constexpr auto isPointer() const -> bool { return m_family == TypeFamily::Pointer; }
    [[nodiscard]] constexpr auto isReference() const -> bool { return m_family == TypeFamily::Reference; }
    [[nodiscard]] constexpr auto isBoolean() const -> bool { return m_family == TypeFamily::Boolean; }
    [[nodiscard]] constexpr auto isNumeric() const -> bool { return isIntegral() || isFloatingPoint(); }
    [[nodiscard]] constexpr auto isIntegral() const -> bool { return m_family == TypeFamily::Integral; }
    [[nodiscard]] constexpr auto isFloatingPoint() const -> bool { return m_family == TypeFamily::FloatingPoint; }
    [[nodiscard]] constexpr auto isFunction() const -> bool { return m_family == TypeFamily::Function; }
    [[nodiscard]] constexpr auto isZString() const -> bool { return m_family == TypeFamily::ZString; }
    [[nodiscard]] constexpr auto isUDT() const -> bool { return m_family == TypeFamily::UDT; }
    [[nodiscard]] auto isAnyPointer() const -> bool;
    [[nodiscard]] auto isSignedIntegral() const -> bool;
    [[nodiscard]] auto isUnsignedIntegral() const -> bool;

    // Qualifiers
    [[nodiscard]] constexpr auto isConst() const -> bool { return flags::has(m_qualifiers, TypeQualifier::Const); }
    [[nodiscard]] constexpr auto isVolatile() const -> bool { return flags::has(m_qualifiers, TypeQualifier::Volatile); }
    [[nodiscard]] constexpr auto isMutable() const -> bool { return flags::has(m_qualifiers, TypeQualifier::Mutable); }
    [[nodiscard]] constexpr auto getQualifiers() const -> TypeQualifier { return m_qualifiers; }
    [[nodiscard]] auto withQualifiers(Context& context, TypeQualifier qualifiers) const -> const TypeRoot*;
    [[nodiscard]] auto withoutQualifiers(Context& context, TypeQualifier qualifiers = TypeQualifier::All) const -> const TypeRoot*;

    // Handy shorthands
    [[nodiscard]] auto
    getUnderlyingFunctionType() const -> const TypeFunction*;

    // Remove reference, otherwise no effect
    [[nodiscard]] auto removeReference() const -> const TypeRoot* {
        if (isReference()) {
            return getBase();
        }
        return this;
    }

    // Comparison
    [[nodiscard]] auto compare(const TypeRoot* other) const -> TypeComparison;

    // clang-format off
    #define CHECK_TYPE_METHOD(ID, ...) [[nodiscard]] auto is## ID() const -> bool;
    INTEGRAL_TYPES(CHECK_TYPE_METHOD)
    FLOATINGPOINT_TYPES(CHECK_TYPE_METHOD)
    #undef CHECK_TYPE_METHOD
    // clang-format on

    [[nodiscard]] auto getSize(Context& context) const -> std::size_t;
    [[nodiscard]] auto getAlignment(Context& context) const -> std::size_t;

protected:
    constexpr explicit TypeRoot(
        const TypeFamily family,
        const TypeKind kind,
        const TypeQualifier qualifier
    )
    : m_family { family }
    , m_kind { kind }
    , m_qualifiers { qualifier } { }

    [[nodiscard]] virtual auto genLlvmType(Context& context) const -> llvm::Type* = 0;

private:
    void reset() const {
        m_llvmType = nullptr;
    }

    const TypeFamily m_family;
    const TypeKind m_kind;
    const TypeQualifier m_qualifiers;
    mutable llvm::Type* m_llvmType = nullptr;
    friend class Context;
};

/**
 * Void, lack typeExpr. Cannot be used for C style `void*`
 * Use `Any Ptr` for this
 */
class TypeVoid final : public TypeRoot {
public:
    constexpr TypeVoid()
    : TypeRoot { TypeFamily::Void, TypeKind::ComplexType, TypeQualifier::None } {
    }
    static auto get() -> const TypeVoid*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::Void;
    }

    [[nodiscard]] auto asString() const -> std::string override;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;
};

/**
 * Use `Any Ptr` for this
 */
class TypeAny final : public TypeRoot {
public:
    constexpr TypeAny()
    : TypeRoot { TypeFamily::Any, TypeKind::ComplexType, TypeQualifier::None } {
    }
    [[nodiscard]] static auto get() -> const TypeAny*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::Any;
    }

    [[nodiscard]] auto asString() const -> std::string override;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;
};

/**
 * Pointer to another typeExpr
 */
class TypePointer final : public TypeRoot {
public:
    constexpr explicit TypePointer(const TypeRoot* base, const TypeQualifier qualifiers = TypeQualifier::None)
    : TypeRoot { TypeFamily::Pointer, TypeKind::ComplexType, qualifiers }
    , m_base { base } {
    }

    [[nodiscard]] static auto get(Context& context, const TypeRoot* base, TypeQualifier qualifiers = TypeQualifier::None) -> const TypePointer*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::Pointer;
    }

    [[nodiscard]] auto asString() const -> std::string override;

    [[nodiscard]] constexpr auto getBase() const -> const TypeRoot* override { return m_base; }

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;

private:
    const TypeRoot* m_base;
};

/**
 * Pointer to another typeExpr
 */
class TypeReference final : public TypeRoot {
public:
    constexpr explicit TypeReference(const TypeRoot* base, const TypeQualifier qualifiers = TypeQualifier::None)
    : TypeRoot { TypeFamily::Reference, TypeKind::ComplexType, qualifiers }
    , m_base { base } {
    }

    [[nodiscard]] static auto get(Context& context, const TypeRoot* base, TypeQualifier qualifiers = TypeQualifier::None) -> const TypeReference*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::Reference;
    }

    // References are technically pointers, in some cases can be useful to simply convert the type
    [[nodiscard]] auto convertToPointer(Context& context) const -> const TypePointer*;

    [[nodiscard]] auto asString() const -> std::string override;

    [[nodiscard]] constexpr auto getBase() const -> const TypeRoot* override { return m_base; }

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;

private:
    const TypeRoot* m_base;
};

/**
 * Boolean true / false, result of comparison operators
 */
class TypeBoolean final : public TypeRoot {
public:
    explicit constexpr TypeBoolean(const TypeQualifier qualifiers = TypeQualifier::None)
    : TypeRoot { TypeFamily::Boolean, TypeKind::Bool, qualifiers } {
    }

    [[nodiscard]] static auto get(TypeQualifier qualifiers = TypeQualifier::None) -> const TypeBoolean*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::Boolean;
    }

    [[nodiscard]] auto asString() const -> std::string override;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;
};

/**
 * Base for all numeric declaredTypes
 * Bool while conforming, is special getKind
 */
class TypeNumeric : public TypeRoot {
public:
    constexpr static auto classof(const TypeRoot* type) -> bool {
        const auto kind = type->getFamily();
        return kind == TypeFamily::Integral || kind == TypeFamily::FloatingPoint;
    }

    [[nodiscard]] constexpr auto getBits() const -> unsigned { return m_bits; }

protected:
    constexpr TypeNumeric(const TypeFamily family, const TypeKind kind, const unsigned bits, const TypeQualifier qualifiers = TypeQualifier::None)
    : TypeRoot { family, kind, qualifiers }
    , m_bits { bits } {
    }

private:
    const unsigned m_bits;
};

/**
 * Fixed width integer declaredTypes.
 */
class TypeIntegral final : public TypeNumeric {
public:
    constexpr TypeIntegral(const TypeKind kind, const unsigned bits, const bool isSigned, const TypeQualifier qualifiers = TypeQualifier::None)
    : TypeNumeric { TypeFamily::Integral, kind, bits, qualifiers }
    , m_signed { isSigned } {
    }

    [[nodiscard]] static auto get(unsigned bits, bool isSigned, TypeQualifier qualifiers = TypeQualifier::None) -> const TypeIntegral*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::Integral;
    }

    [[nodiscard]] constexpr auto isSigned() const -> bool { return m_signed; }

    [[nodiscard]] auto getSigned() const -> const TypeIntegral*;
    [[nodiscard]] auto getUnsigned() const -> const TypeIntegral*;

    [[nodiscard]] auto asString() const -> std::string override;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;

private:
    const bool m_signed;
};

/**
 * Floating point declaredTypes
 */
class TypeFloatingPoint final : public TypeNumeric {
public:
    constexpr explicit TypeFloatingPoint(const TypeKind kind, const unsigned bits, const TypeQualifier qualifiers = TypeQualifier::None)
    : TypeNumeric { TypeFamily::FloatingPoint, kind, bits, qualifiers } {
    }

    [[nodiscard]] static auto get(unsigned bits, TypeQualifier qualifiers = TypeQualifier::None) -> const TypeFloatingPoint*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::FloatingPoint;
    }

    [[nodiscard]] auto asString() const -> std::string override;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;
};

/**
 * Function typeExpr
 */
class TypeFunction final : public TypeRoot {
public:
    TypeFunction(const TypeRoot* retType, llvm::SmallVector<const TypeRoot*>&& paramTypes, const bool variadic, const TypeQualifier qualifiers = TypeQualifier::None)
    : TypeRoot { TypeFamily::Function, TypeKind::ComplexType, qualifiers }
    , m_retType { retType }
    , m_paramTypes { std::move(paramTypes) }
    , m_variadic { variadic } {
    }

    [[nodiscard]] static auto get(
        Context& context,
        const TypeRoot* retType,
        llvm::SmallVector<const TypeRoot*> paramTypes,
        bool variadic,
        TypeQualifier qualifiers = TypeQualifier::None
    ) -> const TypeFunction*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::Function;
    }

    [[nodiscard]] auto asString() const -> std::string override;

    [[nodiscard]] auto getReturn() const -> const TypeRoot* { return m_retType; }
    [[nodiscard]] auto getParams() const -> const llvm::SmallVector<const TypeRoot*>& { return m_paramTypes; }
    [[nodiscard]] auto isVariadic() const -> bool { return m_variadic; }

    auto getLlvmFunctionType(Context& context) const -> llvm::FunctionType* {
        return llvm::cast<llvm::FunctionType>(getLlvmType(context));
    }

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;

private:
    friend class Context;
    const TypeRoot* m_retType;
    const llvm::SmallVector<const TypeRoot*> m_paramTypes;
    const bool m_variadic;
};

/**
 * ZString - a zero terminated string literal.
 * Equivalent to C `const char*`
 */
class TypeZString final : public TypeRoot {
public:
    explicit constexpr TypeZString(const TypeQualifier qualifiers = TypeQualifier::None)
    : TypeRoot { TypeFamily::ZString, TypeKind::ZString, qualifiers } {
    }

    [[nodiscard]] static auto get(TypeQualifier qualifiers = TypeQualifier::None) -> const TypeZString*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::ZString;
    }

    [[nodiscard]] auto asString() const -> std::string override;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* override;
};

} // namespace lbc
