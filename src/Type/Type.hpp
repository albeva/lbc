//
// Created by Albert Varaksin on 07/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Type.def.hpp"

namespace lbc {

enum class TypeFamily : std::uint8_t {
    Void, // Void, lack of typeExpr
    Any, // any ptr, null
    Pointer, // Ptr to another typeExpr
    Boolean, // true / false

    Integral, // signed / unsigned integer 8, 16, 32, ... bits
    FloatingPoint, // single, double

    Function, // function
    ZString, // nil terminated string, byte ptr / char*

    UDT, // User defined Type (C struct)
};

class TypePointer;
class TypeNumeric;
class TypeBoolean;
class TypeIntegral;
class TypeFloatingPoint;
class TypeFunction;
class TypeZString;
class Context;
enum class TokenKind : std::uint8_t;

enum class TypeComparison : std::uint8_t {
    Incompatible,
    Downcast,
    Equal,
    Upcast
};

/**
 * Base typeExpr is root for all lb types
 *
 * It uses llvm custom rtti system
 */
class TypeRoot {
public:
    NO_COPY_AND_MOVE(TypeRoot)

    [[nodiscard]] constexpr auto getKind() const -> TypeFamily { return m_kind; }

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

    // Type queries
    [[nodiscard]] constexpr auto isVoid() const -> bool { return m_kind == TypeFamily::Void; }
    [[nodiscard]] constexpr auto isAny() const -> bool { return m_kind == TypeFamily::Any; }
    [[nodiscard]] constexpr auto isPointer() const -> bool { return m_kind == TypeFamily::Pointer; }
    [[nodiscard]] constexpr auto isBoolean() const -> bool { return m_kind == TypeFamily::Boolean; }
    [[nodiscard]] constexpr auto isNumeric() const -> bool { return isIntegral() || isFloatingPoint(); }
    [[nodiscard]] constexpr auto isIntegral() const -> bool { return m_kind == TypeFamily::Integral; }
    [[nodiscard]] constexpr auto isFloatingPoint() const -> bool { return m_kind == TypeFamily::FloatingPoint; }
    [[nodiscard]] constexpr auto isFunction() const -> bool { return m_kind == TypeFamily::Function; }
    [[nodiscard]] constexpr auto isZString() const -> bool { return m_kind == TypeFamily::ZString; }
    [[nodiscard]] constexpr auto isUDT() const -> bool { return m_kind == TypeFamily::UDT; }
    [[nodiscard]] auto isAnyPointer() const -> bool;
    [[nodiscard]] auto isSignedIntegral() const -> bool;
    [[nodiscard]] auto isUnsignedIntegral() const -> bool;

    // Handy shorthands
    [[nodiscard]] auto getUnderlyingFunctionType() const -> const TypeFunction*;

    // Comparison
    [[nodiscard]] auto compare(const TypeRoot* other) const -> TypeComparison;

    // clang-format off
    #define CHECK_TYPE_METHOD(ID, ...) [[nodiscard]] auto is##ID() const -> bool;
    INTEGRAL_TYPES(CHECK_TYPE_METHOD)
    FLOATINGPOINT_TYPES(CHECK_TYPE_METHOD)
    #undef CHECK_TYPE_METHOD
    // clang-format on

protected:
    constexpr explicit TypeRoot(TypeFamily kind)
    : m_kind { kind } {
    }

    [[nodiscard]] virtual auto genLlvmType(Context& context) const -> llvm::Type* = 0;

private:
    void reset() {
        m_llvmType = nullptr;
    }

    const TypeFamily m_kind;
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
    : TypeRoot { TypeFamily::Void } {
    }
    static auto get() -> const TypeVoid*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getKind() == TypeFamily::Void;
    }

    [[nodiscard]] auto asString() const -> std::string final;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* final;
};

/**
 * Use `Any Ptr` for this
 */
class TypeAny final : public TypeRoot {
public:
    constexpr TypeAny()
    : TypeRoot { TypeFamily::Any } {
    }
    [[nodiscard]] static auto get() -> const TypeAny*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getKind() == TypeFamily::Any;
    }

    [[nodiscard]] auto asString() const -> std::string final;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* final;
};

/**
 * Pointer to another typeExpr
 */
class TypePointer final : public TypeRoot {
public:
    constexpr explicit TypePointer(const TypeRoot* base)
    : TypeRoot { TypeFamily::Pointer }
    , m_base { base } {
    }

    [[nodiscard]] static auto get(Context& context, const TypeRoot* base) -> const TypePointer*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getKind() == TypeFamily::Pointer;
    }

    [[nodiscard]] auto asString() const -> std::string final;

    [[nodiscard]] constexpr auto getBase() const -> const TypeRoot* { return m_base; }

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* final;

private:
    const TypeRoot* m_base;
};

/**
 * Boolean true / false, result of comparison operators
 */
class TypeBoolean final : public TypeRoot {
public:
    constexpr TypeBoolean()
    : TypeRoot { TypeFamily::Boolean } {
    }

    [[nodiscard]] static auto get() -> const TypeBoolean*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getKind() == TypeFamily::Boolean;
    }

    [[nodiscard]] auto asString() const -> std::string final;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* final;
};

/**
 * Base for all numeric declaredTypes
 * Bool while conforming, is special getKind
 */
class TypeNumeric : public TypeRoot {
public:
    constexpr static auto classof(const TypeRoot* type) -> bool {
        auto kind = type->getKind();
        return kind == TypeFamily::Integral || kind == TypeFamily::FloatingPoint;
    }

    [[nodiscard]] constexpr auto getBits() const -> unsigned { return m_bits; }

protected:
    constexpr TypeNumeric(TypeFamily kind, unsigned bits)
    : TypeRoot { kind }
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
    constexpr TypeIntegral(unsigned bits, bool isSigned)
    : TypeNumeric { TypeFamily::Integral, bits }
    , m_signed { isSigned } {
    }

    [[nodiscard]] static auto get(unsigned bits, bool isSigned) -> const TypeIntegral*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getKind() == TypeFamily::Integral;
    }

    [[nodiscard]] constexpr auto isSigned() const -> bool { return m_signed; }

    [[nodiscard]] auto getSigned() const -> const TypeIntegral*;
    [[nodiscard]] auto getUnsigned() const -> const TypeIntegral*;

    [[nodiscard]] auto asString() const -> std::string final;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* final;

private:
    const bool m_signed;
};

/**
 * Floating point declaredTypes
 */
class TypeFloatingPoint final : public TypeNumeric {
public:
    constexpr explicit TypeFloatingPoint(unsigned bits)
    : TypeNumeric { TypeFamily::FloatingPoint, bits } {
    }

    [[nodiscard]] static auto get(unsigned bits) -> const TypeFloatingPoint*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getKind() == TypeFamily::FloatingPoint;
    }

    [[nodiscard]] auto asString() const -> std::string final;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* final;
};

/**
 * Function typeExpr
 */
class TypeFunction final : public TypeRoot {
public:
    TypeFunction(const TypeRoot* retType, llvm::SmallVector<const TypeRoot*>&& paramTypes, bool variadic)
    : TypeRoot { TypeFamily::Function }
    , m_retType { retType }
    , m_paramTypes { std::move(paramTypes) }
    , m_variadic { variadic } {
    }

    [[nodiscard]] static auto get(
        Context& context,
        const TypeRoot* retType,
        llvm::SmallVector<const TypeRoot*> paramTypes,
        bool variadic
    ) -> const TypeFunction*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getKind() == TypeFamily::Function;
    }

    [[nodiscard]] auto asString() const -> std::string final;

    [[nodiscard]] auto getReturn() const -> const TypeRoot* { return m_retType; }
    [[nodiscard]] auto getParams() const -> const llvm::SmallVector<const TypeRoot*>& { return m_paramTypes; }
    [[nodiscard]] auto isVariadic() const -> bool { return m_variadic; }

    auto getLlvmFunctionType(Context& context) const -> llvm::FunctionType* {
        return llvm::cast<llvm::FunctionType>(getLlvmType(context));
    }

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* final;

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
    constexpr TypeZString()
    : TypeRoot { TypeFamily::ZString } {
    }

    [[nodiscard]] static auto get() -> const TypeZString*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getKind() == TypeFamily::ZString;
    }

    [[nodiscard]] auto asString() const -> std::string final;

protected:
    [[nodiscard]] auto genLlvmType(Context& context) const -> llvm::Type* final;
};

} // namespace lbc
