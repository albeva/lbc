//
// Created by Albert Varaksin on 07/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Type.def.hpp"

namespace lbc {

enum class TypeFamily {
    Void,    // Void, lack of typeExpr
    Any,     // any ptr, null
    Pointer, // Ptr to another typeExpr
    Boolean, // true / false

    Integral,      // signed / unsigned integer 8, 16, 32, ... bits
    FloatingPoint, // single, double

    Function, // function
    ZString,  // nil terminated string, byte ptr / char*

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

enum class TypeComparison {
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

    [[nodiscard]] constexpr TypeFamily getKind() const { return m_kind; }

    [[nodiscard]] llvm::Type* getLlvmType(Context& context) const {
        if (m_llvmType == nullptr) {
            m_llvmType = genLlvmType(context);
        }
        return m_llvmType;
    }

    virtual constexpr ~TypeRoot() = default;

    [[nodiscard]] static const TypeRoot* fromTokenKind(TokenKind kind);
    [[nodiscard]] virtual std::string asString() const = 0;

    [[nodiscard]] const TypePointer* getPointer(Context& context) const;

    // Type queries
    [[nodiscard]] constexpr bool isVoid() const { return m_kind == TypeFamily::Void; }
    [[nodiscard]] constexpr bool isAny() const { return m_kind == TypeFamily::Any; }
    [[nodiscard]] constexpr bool isPointer() const { return m_kind == TypeFamily::Pointer; }
    [[nodiscard]] constexpr bool isBoolean() const { return m_kind == TypeFamily::Boolean; }
    [[nodiscard]] constexpr bool isNumeric() const { return isIntegral() || isFloatingPoint(); }
    [[nodiscard]] constexpr bool isIntegral() const { return m_kind == TypeFamily::Integral; }
    [[nodiscard]] constexpr bool isFloatingPoint() const { return m_kind == TypeFamily::FloatingPoint; }
    [[nodiscard]] constexpr bool isFunction() const { return m_kind == TypeFamily::Function; }
    [[nodiscard]] constexpr bool isZString() const { return m_kind == TypeFamily::ZString; }
    [[nodiscard]] constexpr bool isUDT() const { return m_kind == TypeFamily::UDT; }
    [[nodiscard]] bool isAnyPointer() const;
    [[nodiscard]] bool isSignedIntegral() const;
    [[nodiscard]] bool isUnsignedIntegral() const;

    // Handy shorthands
    [[nodiscard]] const TypeFunction* getUnderlyingFunctionType() const;

    // Comparison
    [[nodiscard]] TypeComparison compare(const TypeRoot* other) const;

    // clang-format off

    #define CHECK_TYPE_METHOD(ID, ...) [[nodiscard]] bool is##ID() const ;
    INTEGRAL_TYPES(CHECK_TYPE_METHOD)
    FLOATINGPOINT_TYPES(CHECK_TYPE_METHOD)
    #undef CHECK_TYPE_METHOD

    // clang-format on

protected:
    constexpr explicit TypeRoot(TypeFamily kind)
    : m_kind{ kind } {}

    [[nodiscard]] virtual llvm::Type* genLlvmType(Context& context) const = 0;

private:
    void reset() {
        m_llvmType = nullptr;
    }

    mutable llvm::Type* m_llvmType = nullptr;
    const TypeFamily m_kind;
    friend class Context;
};

/**
 * Void, lack typeExpr. Cannot be used for C style `void*`
 * Use `Any Ptr` for this
 */
class TypeVoid final : public TypeRoot {
public:
    constexpr TypeVoid() : TypeRoot{ TypeFamily::Void } {}
    static const TypeVoid* get();

    constexpr static bool classof(const TypeRoot* type) {
        return type->getKind() == TypeFamily::Void;
    }

    [[nodiscard]] std::string asString() const final;

protected:
    [[nodiscard]] llvm::Type* genLlvmType(Context& context) const final;
};

/**
 * Use `Any Ptr` for this
 */
class TypeAny final : public TypeRoot {
public:
    constexpr TypeAny() : TypeRoot{ TypeFamily::Any } {}
    [[nodiscard]] static const TypeAny* get();

    constexpr static bool classof(const TypeRoot* type) {
        return type->getKind() == TypeFamily::Any;
    }

    [[nodiscard]] std::string asString() const final;

protected:
    [[nodiscard]] llvm::Type* genLlvmType(Context& context) const final;
};

/**
 * Pointer to another typeExpr
 */
class TypePointer final : public TypeRoot {
public:
    constexpr explicit TypePointer(const TypeRoot* base)
    : TypeRoot{ TypeFamily::Pointer }, m_base{ base } {}

    [[nodiscard]] static const TypePointer* get(Context& context, const TypeRoot* base);

    constexpr static bool classof(const TypeRoot* type) {
        return type->getKind() == TypeFamily::Pointer;
    }

    [[nodiscard]] std::string asString() const final;

    [[nodiscard]] constexpr const TypeRoot* getBase() const { return m_base; }

protected:
    [[nodiscard]] llvm::Type* genLlvmType(Context& context) const final;

private:
    const TypeRoot* m_base;
};

/**
 * Boolean true / false, result of comparison operators
 */
class TypeBoolean final : public TypeRoot {
public:
    constexpr TypeBoolean() : TypeRoot{ TypeFamily::Boolean } {}

    [[nodiscard]] static const TypeBoolean* get();

    constexpr static bool classof(const TypeRoot* type) {
        return type->getKind() == TypeFamily::Boolean;
    }

    [[nodiscard]] std::string asString() const final;

protected:
    [[nodiscard]] llvm::Type* genLlvmType(Context& context) const final;
};

/**
 * Base for all numeric declaredTypes
 * Bool while conforming, is special getKind
 */
class TypeNumeric : public TypeRoot {
public:
    constexpr static bool classof(const TypeRoot* type) {
        auto kind = type->getKind();
        return kind == TypeFamily::Integral || kind == TypeFamily::FloatingPoint;
    }

    [[nodiscard]] constexpr unsigned getBits() const { return m_bits; }

protected:
    constexpr TypeNumeric(TypeFamily kind, unsigned bits)
    : TypeRoot{ kind }, m_bits{ bits } {}

private:
    const unsigned m_bits;
};

/**
 * Fixed width integer declaredTypes.
 */
class TypeIntegral final : public TypeNumeric {
public:
    constexpr TypeIntegral(unsigned bits, bool isSigned)
    : TypeNumeric{ TypeFamily::Integral, bits }, m_signed{ isSigned } {}

    [[nodiscard]] static const TypeIntegral* get(unsigned bits, bool isSigned);

    constexpr static bool classof(const TypeRoot* type) {
        return type->getKind() == TypeFamily::Integral;
    }

    [[nodiscard]] constexpr bool isSigned() const { return m_signed; }

    [[nodiscard]] const TypeIntegral* getSigned() const;
    [[nodiscard]] const TypeIntegral* getUnsigned() const;

    [[nodiscard]] std::string asString() const final;

protected:
    [[nodiscard]] llvm::Type* genLlvmType(Context& context) const final;

private:
    const bool m_signed;
};

/**
 * Floating point declaredTypes
 */
class TypeFloatingPoint final : public TypeNumeric {
public:
    constexpr explicit TypeFloatingPoint(unsigned bits)
    : TypeNumeric{ TypeFamily::FloatingPoint, bits } {}

    [[nodiscard]] static const TypeFloatingPoint* get(unsigned bits);

    constexpr static bool classof(const TypeRoot* type) {
        return type->getKind() == TypeFamily::FloatingPoint;
    }

    [[nodiscard]] std::string asString() const final;

protected:
    [[nodiscard]] llvm::Type* genLlvmType(Context& context) const final;
};

/**
 * Function typeExpr
 */
class TypeFunction final : public TypeRoot {
public:
    TypeFunction(const TypeRoot* retType, llvm::SmallVector<const TypeRoot*>&& paramTypes, bool variadic)
    : TypeRoot{ TypeFamily::Function },
      m_retType{ retType },
      m_paramTypes{ std::move(paramTypes) },
      m_variadic{ variadic } {}

    [[nodiscard]] static const TypeFunction* get(
        Context& context,
        const TypeRoot* retType,
        llvm::SmallVector<const TypeRoot*> paramTypes,
        bool variadic
    );

    constexpr static bool classof(const TypeRoot* type) {
        return type->getKind() == TypeFamily::Function;
    }

    [[nodiscard]] std::string asString() const final;

    [[nodiscard]] const TypeRoot* getReturn() const { return m_retType; }
    [[nodiscard]] const llvm::SmallVector<const TypeRoot*>& getParams() const { return m_paramTypes; }
    [[nodiscard]] bool isVariadic() const { return m_variadic; }

    llvm::FunctionType* getLlvmFunctionType(Context& context) const {
        return static_cast<llvm::FunctionType*>(getLlvmType(context));
    }

protected:
    [[nodiscard]] llvm::Type* genLlvmType(Context& context) const final;

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
    constexpr TypeZString() : TypeRoot{ TypeFamily::ZString } {}

    [[nodiscard]] static const TypeZString* get();

    constexpr static bool classof(const TypeRoot* type) {
        return type->getKind() == TypeFamily::ZString;
    }

    [[nodiscard]] std::string asString() const final;

protected:
    [[nodiscard]] llvm::Type* genLlvmType(Context& context) const final;
};

} // namespace lbc
